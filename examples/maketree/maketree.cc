/*
 * MakeTree
 * Copyright (C) 2010 by Mark-Andre Hopf <mhopf@mark13.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *   
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 * Requirements:
 * - Mesa 2.0 or OpenGL
 * - TOAD
 */

#include <toad/toad.hh>
#include <toad/springlayout.hh>
#include <toad/menubar.hh>
#include <toad/action.hh>
#include <toad/scrollbar.hh>
#include <toad/table.hh>
#include <toad/stl/vector.hh>
#include "glwindow.hh"

// redefine 'exception' to avoid trouble between C++ and SGIs <math.h>
#define exception mexception
#include <math.h>
#undef exception

#include <cstdio>
#include <cstdlib>
#include <vector>

#include "math/matrix4.h"

#define DEBUG

using namespace toad;

static void drawSegment(GLfloat len);

double trandom(double v)
{
  return v * ( rand() / RAND_MAX );
}

enum EShape {
  SHAPE_CONICAL,
  SHAPE_SPHERICAL,
  SHAPE_HEMISPHERICAL,
  SHAPE_CYLINDRICAL,
  SHAPE_TAPERED_CYLINDRICAL,
  SHAPE_FLAME,
  SHAPE_INVERSE_CONICAL,
  SHAPE_TEND_FLAME,
  SHAPE_ENVELOPE
};

double
shapeRatio(EShape shape, double ratio)
{
  switch(shape) {
    case SHAPE_CONICAL:
      return 0.2 + 0.8 * ratio;
    case SHAPE_SPHERICAL:
      return 0.2 + 0.8 * sin(M_PI * ratio);
    case SHAPE_HEMISPHERICAL:
      return 0.2 + 0.8 * sin(0.5 * M_PI * ratio);
    case SHAPE_CYLINDRICAL:
      return 1.0;
    case SHAPE_TAPERED_CYLINDRICAL:
      return 0.5 + 0.5 * ratio;
    case SHAPE_FLAME:
      return ratio <= 0.7 ? ratio / 0.7 : (1.0 - ratio) / 0.3;
    case SHAPE_INVERSE_CONICAL:
      return 1.0 - 0.8 * ratio;
    case SHAPE_TEND_FLAME:
      return ratio <= 0.7 ? 0.5 + 0.5 * ratio / 0.7 : 0.5 + 0.5 * ( 1.0 - ratio ) / 0.3;
    case SHAPE_ENVELOPE:
      return 1.0; // use pruning envelope, not implemented yet;
  }
  return 1.0;
}

struct TStem
{
  double length;
  double lengthv;
  double taper;
  
  double curveres;      // number of segments a stem is divided into
  double curve;
  double curvev;
  double curveback;
  
  double segsplits;
  double splitangle;
  double splitanglev;
  
  double downangle;
  double downanglev;
  double rotate;
  double rotatev;
  double branches;
  double branchesdist;
};

struct TTree
{
  TTree();
  // tree shape
    string species;
    EShape shape;
    double levels;
    double scale;
    double scalev;
    double basesize;
    double basesplits;
    double ratiopower;
    double attractionup;
  // trunk radius
    // ratio
    // flare
    // lobes
    // lobedepth
    // scale
    // scalev
  // leaves
    // leaves
    // leafshape
    // leafscale
    // leafscalex
    // leafbend
    // leafstemlen
    // leafdistrib
  // pruning
    // prune ratio
    // prune width
    // prune width peak
    // prune power low
    // prune power high
  // quality
    // leafquality
    // smooth

  GVector<TStem> stem;
};

void
render(const TTree &tree,
       unsigned lvl=0,
       double length_parent=0.0,
       double offset_child=0.0)
{
  double length_child_max = tree.stem[lvl+1].length + trandom(tree.stem[lvl+1].lengthv);

  // double stemLength()
  double length;
  if (lvl==0) {
    length = tree.stem[0].length + trandom(tree.stem[0].lengthv) * tree.scale; // 122.6
  } else if (lvl==1) {
    double length_base = tree.basesize * tree.scale;
    length = length_parent *                                                   // 121.94
             length_child_max *
             shapeRatio(tree.shape, (length_parent - offset_child)/
                                    (length_parent-length_base) );
  } else {
    length = length_child_max * (length_parent - 0.6 * offset_child);          // 122.1
  }

  // void prepareSubstemParams()
  double children=0; // substem_cnt
  if (lvl==0) {
    children = tree.stem[1].branches; // / ( 1.0 - tree.basesize );                // ?
  } else
  if (lvl==1) {
    children = tree.stem[2].branches *                                         // 121.78
                ( 0.2 + 0.8 * ( length / length_parent ) / length_child_max);
  } else {
    if (lvl+1 < tree.stem.size())
      children = tree.stem[lvl+1].branches *                                   // 121.80
                 ( 1.0 - 0.5 * offset_child / length_parent);
  }

  double segmentLength = length / tree.stem[lvl].curveres;
  double length_base = 0.0;
  if (lvl==0)
    length_base = tree.basesize * length;

  double dist;
  if (lvl==0) {
    dist = (length - length_base) / children;
  } else {
    dist = length / children;
  }
  
  double r=0.0;

unsigned cntr=0;

  for(double segment=0.0; segment<length; segment+=segmentLength) {

    double d = trandom(tree.stem[lvl].curvev)/tree.stem[lvl].curveres;
    if (tree.stem[lvl].curveback==0.0)
      d += tree.stem[lvl].curve/tree.stem[lvl].curveres;
    else if (segment<tree.stem[lvl].curveres/2)
      d += tree.stem[lvl].curve/(tree.stem[lvl].curveres/2);
    else
      d += tree.stem[lvl].curveback/(tree.stem[lvl].curveres/2);
    glRotatef(d, 1.0, 0.0, 0.0);
    drawSegment(segmentLength);

    if (children==0.0)
      goto skipChildren;
{
    double off0 = segment;
    double off1 = segment + segmentLength;
    if (lvl==0) {
      if (off1 <= length_base) {
        goto skipChildren;
      } else
      if (off0 < length_base) {
        off0 = length_base;
      }
    }
    
    off0 -= segment;
    off1 -= segment;

    for(; off0<off1; off0+=dist) {
      double offsetChild = off0 + segment;

      glPushMatrix();
      glTranslated(0.0, off0, 0.0);

      // rotate < 0.0 is a special case
      r += tree.stem[lvl+1].rotate + trandom(tree.stem[lvl+1].rotatev);
      glRotated(r, 0.0, 1.0, 0.0);

      double downangle_child;
      if (tree.stem[lvl+1].downanglev >= 0.0) {
        downangle_child = tree.stem[lvl+1].downangle + trandom(tree.stem[lvl+1].downanglev);
      } else {
        downangle_child = 
          tree.stem[lvl+1].downangle + 
            tree.stem[lvl+1].downanglev *
            ( 1.0 - 2.0 * shapeRatio(SHAPE_CONICAL, (length-offsetChild)/(length-length_base)));
      }
      glRotated(downangle_child, 1.0, 0.0, 0.0);

      double length_child;
      if (lvl==0) {
        double length_base = tree.basesize * tree.scale;
        length_child = length *
                       length_child_max *
                       shapeRatio(tree.shape, (length - offset_child)/(length-length_base));
      } else {
        length_child = length_child_max * ( length /*_parent*/ - 0.6 * offset_child );
      }

//      drawSegment(length_child);
++cntr;
      render(tree, lvl+1, length, offsetChild);
      glPopMatrix();
    }


}

skipChildren:
    glTranslated(0.0, segmentLength, 0.0);
  }

if (lvl==0)
  cout << "drew " << cntr << " children, wanted " << children << endl;

}

#if 0
void
render(const TTree &tree, unsigned lvl=0, double offset_child=0.0)
{
  double scale_tree = tree.scale + trandom(tree.scalev);
  double length_trunk = tree.stem[0].length + trandom(tree.stem[0].lengthv) * tree.scale;

//  if (lvl+1=>stem.size()) return;
/*    
  for(unsigned i=0; i<children; ++i) {
    
  }
*/

  double l = tree.stem[lvl].length + trandom(tree.stem[lvl].lengthv);

  double length_child_max = tree.stem[lvl+1].length + trandom(tree.stem[lvl+1].lengthv);
  
  unsigned children;
  if (lvl==0)
    children = tree.stem[lvl].branches * ( 0.2 + 0.8 * (tree.stem[lvl+1].length / l/*ength_parent*/) / length_child_max );
  else
    children = tree.stem[lvl].branches * ( 1.0 - 0.5 * offset_child / l/*ength_parent*/);


  double sl = l / tree.stem[lvl].curveres; // segment length
  for(double segment=0.0; segment<l; segment+=sl) {
    // 4.1 The Curved Stem
    if (tree.stem[lvl].curvev<0.0) {
      // MISSING: special mode, stem is formed as a helix, declanation angle is curvevary
    }
    double d = trandom(tree.stem[lvl].curvev)/tree.stem[lvl].curveres;
    if (tree.stem[lvl].curveback==0.0)
      d += tree.stem[lvl].curve/tree.stem[lvl].curveres;
    else if (segment<tree.stem[lvl].curveres/2)
      d += tree.stem[lvl].curve/(tree.stem[lvl].curveres/2);
    else
      d += tree.stem[lvl].curveback/(tree.stem[lvl].curveres/2);
    glRotatef(d, 1.0, 0.0, 0.0);
    drawSegment(sl);

    double length_child;
    if (lvl==0) {
      double length_base = tree.basesize * tree.scale;
      length_child = length_trunk *
                     length_child_max *
                     shapeRatio(tree.shape, (length_trunk - offset_child)/(length_trunk-length_base));
    } else {
      length_child = length_child_max * ( l/*ength_parent*/ - 0.6 * offset_child );
    }

    double downangle_child;
    if (tree.stem[lvl+1].downanglev >= 0.0) {
      downangle_child = tree.stem[lvl+1].downangle + tree.stem[lvl+1].trandom(downanglev);
    } else {
      // uh.. distribution not implemented yet
      downangle_child = tree.stem[lvl+1].downangle + trandom(tree.stem[lvl+1].downanglev);
    }
    
    // rotate < 0.0 is a special case
    double r = tree.stem[lvl+1].rotate + trandom(tree.stem[lvl+1].rotatev);

    glTranslated(0.0, sl, 0.0);
  }
}
#endif

TTree tree;

TTree::TTree()
{
  species = "quaking_aspen";
  shape = SHAPE_TEND_FLAME;
  levels = 3;
  scale = 13.0;
  scalev = 3.0;
  basesize = 0.4;
  basesplits = 0.0;
  ratiopower = 1.2;
  attractionup = 0.5;

  stem.push_back(TStem());
  stem.push_back(TStem());
  stem.push_back(TStem());

  stem[0].length  = 1.0;
  stem[0].lengthv = 0.0;
  stem[0].taper   = 1.0;
  stem[0].curveres = 3 ;
  stem[0].curve = 0.0;
  stem[0].curvev = 20.0;
  stem[0].curveback = 0.0;
  stem[0].segsplits = 0.0;
  stem[0].splitangle = 0.0;
  stem[0].splitanglev = 0.0;
  stem[0].downangle = 0.0;
  stem[0].downanglev = 0.0;
  stem[0].rotate = 0.0;
  stem[0].rotatev = 0.0;
  stem[0].branches = 1.0;
  stem[0].branchesdist = 0.0;

  stem[1].length  = 1.0;
  stem[1].lengthv = 0.0;
  stem[1].taper   = 1.0;
  stem[1].curveres = 5 ;
  stem[1].curve = -40.0;
  stem[1].curvev = 50.0;
  stem[1].curveback = 0.0;
  stem[1].segsplits = 0.0;
  stem[1].splitangle = 0.0;
  stem[1].splitanglev = 0.0;
  stem[1].downangle = 60.0;
  stem[1].downanglev = -50.0;
  stem[1].rotate = 140.0;
  stem[1].rotatev = 0.0;
  stem[1].branches = 50.0;
  stem[1].branchesdist = 1.0;

  stem[2].length  = 0.6;
  stem[2].lengthv = 0.0;
  stem[2].taper   = 1.0;
  stem[2].curveres = 3 ;
  stem[2].curve = -40.0;
  stem[2].curvev = 75.0;
  stem[2].curveback = 0.0;
  stem[2].segsplits = 0.0;
  stem[2].splitangle = 0.0;
  stem[2].splitanglev = 0.0;
  stem[2].downangle = 45.0;
  stem[2].downanglev = 10.0;
  stem[2].rotate = 0.0;
  stem[2].rotatev = 0.0;
  stem[2].branches = 30.0;
  stem[2].branchesdist = 1.0;
}

class TTreeAdapter:
  public TSimpleTableAdapter
{
    TTree *container;
  public:
    TTreeAdapter(TTree *container) { this->container = container; }
    virtual size_t getRows() { return 16; }
    virtual size_t getCols() { return container->stem.size(); }
    void tableEvent(TTableEvent &te);
};

void
TTreeAdapter::tableEvent(TTableEvent &te)
{
  switch(te.type) {
    case TTableEvent::GET_COL_SIZE:
      te.w = 64;
      break;
    case TTableEvent::GET_ROW_SIZE: // otherwise we crash. fix this in libtoad!!!
      break;
    default:
      switch(te.row) {
        case  0: handleDouble(te, &container->stem[te.col].length); break;
        case  1: handleDouble(te, &container->stem[te.col].lengthv); break;
        case  2: handleDouble(te, &container->stem[te.col].taper); break;
        case  3: handleDouble(te, &container->stem[te.col].curveres); break;
        case  4: handleDouble(te, &container->stem[te.col].curve); break;
        case  5: handleDouble(te, &container->stem[te.col].curvev); break;
        case  6: handleDouble(te, &container->stem[te.col].curveback); break;
        case  7: handleDouble(te, &container->stem[te.col].segsplits); break;
        case  8: handleDouble(te, &container->stem[te.col].splitangle); break;
        case  9: handleDouble(te, &container->stem[te.col].splitanglev); break;
        case 10: handleDouble(te, &container->stem[te.col].downangle); break;
        case 11: handleDouble(te, &container->stem[te.col].downanglev); break;
        case 12: handleDouble(te, &container->stem[te.col].rotate); break;
        case 13: handleDouble(te, &container->stem[te.col].rotatev); break;
        case 14: handleDouble(te, &container->stem[te.col].branches); break;
        case 15: handleDouble(te, &container->stem[te.col].branchesdist); break;
      }
  }
}

class TViewer:
  public TGLWindow
{
  public:
    Matrix observer;
    TViewer(TWindow *p,const string &t):TGLWindow(p,t)
    {
    };
  protected:
    void glPaint();
    void mouseEvent(const TMouseEvent &);
};

class TMainWindow:
  public TWindow
{
  public:
    TMainWindow(TWindow *p,const string &t)
    :TWindow(p,t){};
  protected:
    TViewer *gl;
    void create();
    void menuQuit();
    void menuInfo();
    void menuCopyright();
};


int
main(int argc, char **argv, char **envv)
{

  toad::initialize(argc, argv, envv); {
    TMainWindow wnd(NULL, "MakeTree");
    toad::mainLoop();
  } toad::terminate();
  return 0;
}

// TMainWindow
//--------------------------------------------------------------------
void TMainWindow::create()
{
  setSize(640,480);
  setBackground(TColor::DIALOG);

  // create menubar
  //----------------
  TMenuBar *mb=new TMenuBar(this, "mb");
  
  TAction *action;
  action = new TAction(this, "file|quit");
  CONNECT(action->sigClicked, this,menuQuit);

  action = new TAction(this, "help|info");
  CONNECT(action->sigClicked, this,menuInfo);

  action = new TAction(this, "help|copyright");
  CONNECT(action->sigClicked, this,menuCopyright);

  // create viewer
  //---------------
  gl = new TViewer(this, "gl");

  // create stem table
  //-------------------
  TTable *tbl = new TTable(this, "tbl");
  tbl->setAdapter(new TTreeAdapter(&tree));
  TDefaultTableHeaderRenderer *hdr = new TDefaultTableHeaderRenderer(false);
  
  static const char *names[16] = {
    "Length", "LengthV", "Taper", 
    "CurveRes", "Curve", "CurveV", "CurveBack",
    "SegSplits", "SplitAngle", "SplitAngleV",
    "DownAngle", "DownAngleV", "Rotate", "RotateV", "Branches", "BranchesDist"
  };
  for(int i=0; i<16; ++i)
    hdr->setText(i, names[i]);
  tbl->setRowHeaderRenderer(hdr);
  tbl->setColHeaderRenderer(new TDefaultTableHeaderRenderer(true));

  // layout
  //------------------------
  TSpringLayout *layout = new TSpringLayout();
  
  layout->attach("mb", TSpringLayout::TOP | TSpringLayout::LEFT | TSpringLayout::RIGHT);

  layout->attach("tbl", TSpringLayout::TOP, "mb");
  layout->attach("tbl", TSpringLayout::LEFT | TSpringLayout::BOTTOM);
  
  layout->attach("gl", TSpringLayout::TOP, "mb");
  layout->attach("gl", TSpringLayout::RIGHT | TSpringLayout::BOTTOM);
  layout->attach("gl", TSpringLayout::LEFT, "tbl");
  
  layout->distance("gl", 10);

  setLayout(layout);
}

void
TMainWindow::menuQuit()
{
  if(messageBox(this,
                getTitle(),
                "Do you really want to quit the program?",
                TMessageBox::ICON_QUESTION | TMessageBox::YESNO
               ) == TMessageBox::YES)
  {
    postQuitMessage(0);
  }
}

void
TMainWindow::menuInfo()
{
  messageBox(this, 
             getTitle(),
            "Create 3D trees. The green plant thingys\n"
            "For further information write to:\n" 
            "mhopf@mark13.org",
            TMessageBox::ICON_INFORMATION | TMessageBox::OK);
}

void
TMainWindow::menuCopyright()
{
  messageBox(this, getTitle(),
    "MakeTree\n\n"
    "The program implements the tree creation algorithm as described 1995 by "
    "Jason Weber (Teletronics International, Inc.) and Joseph Penn (Army "
    "Research Laboratory) in \"Creation and Rendering of Realistic Trees\".\n\n"
    
    "MakeTree\n"
    "Copyright (C) 2010 by Mark-André Hopf <mhopf@mark13.org>\n\n"
    "This program is free software; you can redistribute it and/or modify "
    "it under the terms of the GNU General Public License as published by "
    "the Free Software Foundation; either version 2 of the License, or "
    "(at your option) any later version.\n\n"
    
    "This program is distributed in the hope that it will be useful, "
    "but WITHOUT ANY WARRANTY; without even the implied warranty of "
    "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "
    "GNU General Public License for more details.\n\n"
    
    "You should have received a copy of the GNU General Public License "
    "along with this program; if not, write to the Free Software "
    "Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.\n\n"

    "GLT OpenGL C++ Toolkit\n"
    "Copyright (C) 2000-2002 Nigel Stewart <nigels@nigels.com>\n"
    "http://www.nigels.com/glt/\n\n"
    "This library is free software; you can redistribute it and/or "
    "modify it under the terms of the GNU Lesser General Public "
    "License as published by the Free Software Foundation; either "
    "version 2.1 of the License, or (at your option) any later version.\n\n"
    "This library is distributed in the hope that it will be useful, "
    "but WITHOUT ANY WARRANTY; without even the implied warranty of "
    "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU "
    "Lesser General Public License for more details.\n\n"
    "You should have received a copy of the GNU Lesser General Public "
    "License along with this library; if not, write to the Free Software "
    "Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA"
    ,
    TMessageBox::ICON_INFORMATION | TMessageBox::OK);
}            

// TViewer
//--------------------------------------------------------------------
void 
drawSegment(GLfloat l)
{
  GLfloat r0 = 0.02;
  GLfloat r1 = 0.02;

  unsigned n = 8;
  GLfloat sh = M_PI / n; // half step
  GLfloat s = 2.0 * sh;  // full step

  GLfloat s0, s1=0.0;

  for(unsigned i=0; i<n; ++i) {
    s0 = s1;
    s1 += s;
    GLfloat sn = s0 + sh;
    glBegin(GL_POLYGON);
 
    Vector v[4];
    v[0][0] = sin(s1)*r0;
    v[0][1] = 0.0;
    v[0][2] = cos(s1)*r1;
    v[1][0] = sin(s1)*r0;
    v[1][1] = l;
    v[1][2] = cos(s1)*r1;
    v[2][0] = sin(s0)*r0;
    v[2][1] = l;
    v[2][2] = cos(s0)*r1;
    v[3][0] = sin(s0)*r0;
    v[3][1] = 0.0;
    v[3][2] = cos(s0)*r1;
    
    Vector n = planeNormal(v[0], v[1], v[2]);
    n.normalize();
    n.glNormal();
    
    for(unsigned j=0; j<4; ++j)
      v[j].glVertex();

    glEnd();
  }
}

void
TViewer::glPaint()
{
  glClearColor( 0.0, 0.0, 0.5, 0.0 );
  glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
  
  glMatrixMode( GL_PROJECTION );
  glLoadIdentity();
  glFrustum( -1.0,    // left
              1.0,    // right
              -1.0,   // bottom
              1.0,    // top
              1.0,    // near
              40.0    // far
  );
  
  glMatrixMode( GL_MODELVIEW );
  glLoadIdentity();

  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  
  glEnable(GL_LIGHTING);
  glEnable(GL_COLOR_MATERIAL);
  glEnable(GL_LIGHT0);


  glTranslatef(0.0, -5.0, -10.0);

  observer.glMultMatrix();

  glScaled(10.0,10.0,10.0);

  glColor3f(1.0, 0.0, 0.0);

  render(tree);
}

void
TViewer::mouseEvent(const TMouseEvent &me)
{
  if (me.dblClick) {
    observer.identity();
    return;
  }

  static TCoord x,y;
  switch(me.type) {
    case TMouseEvent::LDOWN:
    case TMouseEvent::RDOWN:
      x=me.x;
      y=me.y;
      break;
    case TMouseEvent::LUP:
      break;
    case TMouseEvent::MOVE:
      if (me.modifier() & MK_LBUTTON) {
        observer *= matrixRotate(me.y-y, x-me.x);
        x=me.x;
        y=me.y;
        invalidateWindow();
      }
      if (me.modifier() & MK_RBUTTON) {
        observer *= matrixTranslate((me.x-x)/10.0, (y-me.y)/10.0, 0.0);
        x=me.x;
        y=me.y;
        invalidateWindow();
      }
      break;
    case TMouseEvent::ROLL_UP:
      observer *= matrixTranslate(0.0, 0.0, 1.0);
      invalidateWindow();
      break;
    case TMouseEvent::ROLL_DOWN:
      observer *= matrixTranslate(0.0, 0.0, -1.0);
      invalidateWindow();
      break;
    
  }
}
