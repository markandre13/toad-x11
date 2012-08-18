/*
 * MakeTree
 * Copyright (C) 2011 by Mark-Andre Hopf <mhopf@mark13.org>
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
 * - OpenGL
 * - TOAD
 */

#define _TOAD_PRIVATE
#define GL_GLEXT_PROTOTYPES 1

#include <toad/toad.hh>
#include <toad/springlayout.hh>
#include <toad/menubar.hh>
#include <toad/filedialog.hh>
#include <toad/action.hh>
#include <toad/undomanager.hh>
#include <toad/textfield.hh>
#include <toad/combobox.hh>
#include <toad/table.hh>
#include <toad/stl/vector.hh>
#include <toad/io/binstream.hh>
#include "glwindow.hh"

#include <GL/glext.h>

#include <cmath>

#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>
#include <vector>
#include <fstream>
#include <map>
#include <libxml/xmlreader.h>

#include "math/matrix4.h"

#define DEBUG

using namespace toad;

static const string programname("MakeTree");

struct Exporter *exporter = 0;
size_t surface_branch, surface_leaf;

unsigned stem_res = 8;
bool recalculate = true;

// branch & leaf
vector<Vector> normals[2];
vector<Vector> points[2];
vector<GLuint> faces[2];

// rendering the tree sometimes takes a while
// strategy was to abort in case we had a new message, but that caused the tree
// not being rendered sometimes. more time required to fix this. so here's a
// hack
bool myPeekMessage()
{
  while(XPending(x11display)) {
    TOADBase::handleMessage();
  }
  return false;
}

struct Exporter
{
#if 0
  Exporter();

  size_t createSurface(const string &name, double r, double g, double b);
  void begin(size_t surface);
  void vertex(float x, float y, float z);
  void vertex(const Vector &v) {
    vertex(v[0], v[1], v[2]);
  }
  void end();
#endif
  virtual void write(ostream *stream) = 0;
#if 0
  size_t srfs_size, pols_size;

  struct point {
    point(float px, float py, float pz):
      x(px), y(py), z(pz) {}
    float x, y, z;
  };
  vector<point> points;
  map<string, size_t> lookup;
  
  struct polygon {
    vector< vector<point>::size_type > points;
  };
  vector<polygon> polygons;
  
  struct surface {
    surface(const string &aName, double cR, double cG, double cB):
      name(aName), r(cR*255), g(cG*255), b(cB*255) {}
    string name;
    vector<polygon> polygons;
    unsigned r, g, b;
  };
  vector<surface> surfaces;
  
  polygon *current;
#endif
};

#if 0
Exporter::Exporter()
{
  srfs_size = pols_size = 0;
  current = NULL;
}

size_t
Exporter::createSurface(const string &name, double r, double g, double b)
{
  surfaces.push_back(surface(name, r, g, b));
  srfs_size += name.size() + 1;
  if (!(name.size() & 1))
    ++srfs_size;
    
  return surfaces.size() - 1;
};

void
Exporter::begin(size_t surface)
{
  pols_size += 2 + 2;
  surfaces[surface].polygons.push_back(polygon());
  current = &surfaces[surface].polygons.back();
}

void
Exporter::vertex(float x, float y, float z)
{
  pols_size += 2;
  size_t index;

  struct {
    float x, y, z;
  } pt;
  pt.x = x;
  pt.y = y;
  pt.z = z;
  string id;
  id.assign((char*)&pt, sizeof(pt));
  map<string, size_t>::iterator i = lookup.find(id);
  if (i==lookup.end()) {
    index = points.size();
    points.push_back(point(x, y, z));
    lookup[id] = index;
  } else {
    index = i->second;
  }
  
#if 0
  index = 0;
  for(vector<point>::const_iterator p = points.begin(); p!=points.end(); ++p) {
    if (p->x==x && p->y==y && p->z==z) {
      break;
    }
    ++index;
  }
#else
//  index = points.size();
#endif
//  if (index==points.size())
//    points.push_back(point(x, y, z));
  current->points.push_back(index);
}

void
Exporter::end()
{
  current = NULL;
}
#endif

struct WFOF:
  public Exporter
{
  void write(ostream *stream);
};

void
WFOF::write(ostream *stream)
{
  ostream &out(*stream);

  out << "mtllib tree.mtl" << endl;
  
  out << "vt 0.000000 0.312500" << endl
      << "vt 0.312500 0.000000" << endl
      << "vt 0.625000 0.000000" << endl
      << "vt 0.937500 0.156250" << endl
      << "vt 1.000000 0.312500" << endl
      << "vt 0.937500 0.468750" << endl
      << "vt 0.625000 0.625000" << endl
      << "vt 0.312500 0.625000" << endl;
  
  for(int i=0; i<2; ++i) {
    for(vector<Vector>::const_iterator p = points[i].begin(); p!=points[i].end(); ++p) {
      out << "v " << p->x() << ' ' << p->y() << ' ' << p->z() << endl;
    }
  }
  
  for(int i=0; i<2; ++i) {
    GLuint o = (i==0) ? 0 : points[0].size();
    ++o;
    out << "usemtl " << (i==0 ? "branches" : "leafs" ) << endl;
    unsigned t=0;
    for(vector<GLuint>::const_iterator j=faces[i].begin(); j!=faces[i].end();) {
//      out << "f " << *(j++)+o << ' ' << *(j++)+o << ' ' << *(j++)+o << ' ' << *(j++)+o << endl;
      out << "f ";
      out << (*j) + o;
      if (i==1) {
        switch(t) {
          case 0: out << "/1"; break;
          case 1: out << "/2"; break;
          case 2: out << "/3"; break;
        }
      }
      ++j;
      out << " ";
      out << (*j) + o;
      if (i==1) {
        switch(t) {
          case 0: out << "/2"; break;
          case 1: out << "/3"; break;
          case 2: out << "/4"; break;
        }
      }
      ++j;
      out << " ";
      out << (*j) + o;
      if (i==1) {
        switch(t) {
          case 0: out << "/7"; break;
          case 1: out << "/6"; break;
          case 2: out << "/5"; break;
        }
      }
      ++j;
      out << " ";
      out << (*j) + o;
      if (i==1) {
        switch(t) {
          case 0: out << "/8"; break;
          case 1: out << "/7"; break;
          case 2: out << "/6"; break;
        }
        ++t;
        if (t>2)
          t=0;
      }
      ++j;
      out << endl;
    }
  }
}

// Lightwave Object Format (version 1.0, a bit too much for the trees)
// LWO2 is introduced with LighWave 6
struct LWOF:
  public Exporter
{
  void write(ostream *stream);
};

void
LWOF::write(ostream *stream)
{
#if 0
  TOutBinStream out(stream);
  out.setEndian(TOutBinStream::BIG);

  size_t form_size = 8*4 + 4 + points.size() * 12 + srfs_size * 2 + pols_size + 26 * surfaces.size();

  out.writeString("FORM");
  out.writeDWord(form_size); // complete file size minus this header (8 octets)
  
  out.writeString("LWOB");
  
  // points
  out.writeString("PNTS");
  out.writeDWord(points.size() * 12);
  
  for(vector<point>::const_iterator p = points.begin(); p!=points.end(); ++p) {
    out.writeFloat(p->x);
    out.writeFloat(p->y);
    out.writeFloat(p->z);
  }
  
  // surfaces
  out.writeString("SRFS");
  out.writeDWord(srfs_size);
  
  for(vector<surface>::const_iterator p = surfaces.begin(); p != surfaces.end(); ++p) {
    out.writeString(p->name);
    out.writeByte(0);
    if (!(p->name.size() & 1))
      out.writeByte(0);
  }
  
  // polygons
  out.writeString("POLS");
  out.writeDWord(pols_size);
  
  size_t cntr = 1;
  for(vector<surface>::const_iterator p = surfaces.begin(); p != surfaces.end(); ++p) {
    for(vector<polygon>::const_iterator q = p->polygons.begin(); q != p->polygons.end(); ++q) {
      out.writeWord(q->points.size());
      for(vector< vector<point>::size_type >::const_iterator r = q->points.begin(); r != q->points.end(); ++r) {
        out.writeWord(*r);
      }
      out.writeWord(cntr++);
    }
  }

  // more information on the surfaces
  for(vector<surface>::const_iterator p = surfaces.begin(); p != surfaces.end(); ++p) {
     out.writeString("SURF");
     out.writeDWord(p->name.size() + ( (p->name.size()&1) ? 1 : 2 ) + 18);
     
     out.writeString(p->name);
     out.writeByte(0);
     if (!(p->name.size() & 1))
       out.writeByte(0);
     
     out.writeString("COLR");
     out.writeWord(4);
     out.writeByte(p->r);
     out.writeByte(p->g);
     out.writeByte(p->b);
     out.writeByte(0);
     
     out.writeString("FLAG");
     out.writeWord(2);
     out.writeWord(0);
  }
#endif
}

template <typename T>
class TEnumSelectionModel:
  public TSingleSelectionModel
{
  public:
    operator T() const { return static_cast<T>(getRow()); }
    T operator=(T v) { select(0, v); return v; }
};

class TCStringTableAdapter:
  public TSimpleTableAdapter
{
    const char **array;
    size_t n;
  public:
    TCStringTableAdapter(const char **);
    size_t getRows() { return n; }
    size_t getCols() { return 1; }
    void tableEvent(TTableEvent &te);
};

TCStringTableAdapter::TCStringTableAdapter(const char **a)
{
  array = a;
  n = 0;
  for(; *a; ++a)
    ++n;
}

void
TCStringTableAdapter::tableEvent(TTableEvent &te)
{
  switch(te.type) {
    case TTableEvent::GET_COL_SIZE: {
      te.w = 0;
      for(size_t i=0; i<n; ++i) {
        TCoord w = getDefaultFont().getTextWidth(array[i]);
        if (w>te.w)
          te.w = w;
      }
    } break;
    case TTableEvent::GET_ROW_SIZE:
      te.h = getDefaultFont().getHeight()+2;
      break;
    case TTableEvent::PAINT:
      renderBackground(te);
      te.pen->drawString(1, 1, array[te.row]);
      renderCursor(te);
      break;
  }
}

class TTree;

static void drawSegment(size_t &ring0, size_t &ring1, const Matrix &m, GLfloat len, GLfloat radius, bool initializeRing0);
static void drawLeaf(const Matrix &m, const TTree &tree);

enum ERandomOption {
  RANDOM_RANDOM,
  RANDOM_MINIMUM,
  RANDOM_MAXIMUM
};

const char* randomnames[] = {
  "random",
  "minimum",
  "maximum"
};

TEnumSelectionModel<ERandomOption> randomopt;


double trandom(double v)
{
  switch(randomopt) {
    case RANDOM_RANDOM:
      return v * ( (double)rand() / RAND_MAX );
    case RANDOM_MINIMUM:
      return 0.0;
    case RANDOM_MAXIMUM:
      return v;
  }
  return 0.0;
}

double trandom2(double v)
{
  switch(randomopt) {
    case RANDOM_RANDOM:
      return v * ( (double)rand() / RAND_MAX ) - (v / 2.0);
    case RANDOM_MINIMUM:
      return -v/2.0;
    case RANDOM_MAXIMUM:
      return v/2.0;
  }
  return 0.0;
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

const char* shapenames[] = {
  "conical",
  "spherical",
  "hemispherical",
  "cylindrical",
  "tapered cylindrical",
  "flame",
  "inverse conical",
  "tend flame",
  "envelope"
};

struct TIO
{
  TIO() {
    name = value = 0;
    found = false;
    row = -1;
    out = 0;
  }
  char *name;
  char *value;
  bool found;
  int row;
  ostream *out;
};

struct TStem
{
  TStem() {
    length = 1.0;
    lengthv = 0;
    taper = 0;
    curveres = 3;
    curve = 0;
    curvev = 0;
    curveback = 0;
    segsplits = 0;
    splitangle = 45.0;
    splitanglev = 0;
    downangle = 20;
    downanglev = 0;
    rotate = 140.0;
    rotatev = 0;
    branches = 0;
    branchesdist = 0;
  }
  double length;
  double lengthv;
  double taper;
  
  int curveres;      // number of segments a stem is divided into
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
  int branches;
  double branchesdist;
};

struct TTree
{
  TTree();
  
  void assign(const TTree&);
    TIntegerModel random;

  // tree shape
    TTextModel species;
    // EShape shape;
    TEnumSelectionModel<EShape> shape;
    TIntegerModel levels;
    TFloatModel scale;
    TFloatModel scalev;
    TFloatModel basesize;
    TFloatModel basesplits;
    TFloatModel ratiopower;
    TFloatModel attractionup;
  // trunk radius
    TFloatModel ratio;
    TFloatModel flare;
    TFloatModel lobes;
    TFloatModel lobedepth;
    TFloatModel scale0;
    TFloatModel scale0v;
  // leaves
    TFloatModel leaves;
    TFloatModel leafshape;
    TFloatModel leafscale;
    TFloatModel leafscalex;
    TFloatModel leafbend;
    TFloatModel leafstemlen;
    TFloatModel leafdistrib;
  // pruning
    TFloatModel prune_ratio;
    TFloatModel prune_width;
    TFloatModel prune_width_peak;
    TFloatModel prune_power_low;
    TFloatModel prune_power_high;
  // quality
    TFloatModel leafquality;
    TFloatModel smooth;

  GVector<TStem> stem;

  double shapeRatio(EShape shape, double ratio) const;

  bool load(const char *filename);
  bool save(const char *filename);
  void io1(TIO &io);
  void io2(TIO &io, TStem &s);
};

void
TTree::assign(const TTree &t)
{
  species = t.species;
  shape = t.shape;
  levels = t.levels;
  scale = t.scale;
  scalev = t.scalev;
  basesize = t.basesize;
  basesplits = t.basesplits;
  ratiopower = t.ratiopower;
  attractionup = t.attractionup;
  
  ratio = t.ratio;
  flare = t.flare;
  lobes = t.lobes;
  lobedepth = t.lobedepth;
  scale0 = t.scale0;
  scale0v = t.scale0v;
  
  leaves = t.leaves;
  leafshape = t.leafshape;
  leafscale = t.leafscale;
  leafscalex = t.leafscalex;
  leafbend = t.leafbend;
  leafstemlen = t.leafstemlen;
  leafdistrib = t.leafdistrib;
  
  prune_ratio = t.prune_ratio;
  prune_width = t.prune_width;
  prune_width_peak = t.prune_width_peak;
  prune_power_low = t.prune_power_low;
  prune_power_high = t.prune_power_high;
  
  leafquality = t.leafquality;
  smooth = t.smooth;
  random = t.random;
  
  stem.erase(stem.begin(), stem.end());
  for(GVector<TStem>::const_iterator p = t.stem.begin(); p!=t.stem.end(); ++p) {
    stem.push_back(TStem());
    TStem &d = *(stem.end()-1);
    const TStem &s = *p;
    
    d.length = s.length;
    d.lengthv = s.lengthv;
    d.taper = s.taper;
    d.curveres = s.curveres;
    d.curve = s.curve;
    d.curvev = s.curvev;
    d.curveback = s.curveback;
    d.segsplits = s.segsplits;
    d.splitangle = s.splitangle;
    d.splitanglev = s.splitanglev;
    d.downangle = s.downangle;
    d.downanglev = s.downanglev;
    d.rotate = s.rotate;
    d.rotatev = s.rotatev;
    d.branches = s.branches;
    d.branchesdist = s.branchesdist;
  }
}

template <typename T>
void
fetch(TIO &io, const char *name, TEnumSelectionModel<T> *v)
{
  if (io.out) {
    (*io.out) << "    <param name='";
    if (io.row>=0) (*io.out) << io.row;
    (*io.out)<<name<<"' value='"<<*v<<"'/>\n";
    return;
  }
  if (io.found || strcasecmp(io.name, name)!=0) return;
  *v = static_cast<T>(atoi(io.value));
  io.found=true;
}

void
fetch(TIO &io, const char *name, TFloatModel *v)
{
  if (io.out) {
    (*io.out) << "    <param name='";
    if (io.row>=0) (*io.out) << io.row;
    (*io.out)<<name<<"' value='"<<*v<<"'/>\n";
    return;
  }

  if (io.found || strcasecmp(io.name, name)!=0) return;
  double d;
  sscanf(io.value, "%lf", &d);
  *v = d;
  io.found=true;
}

void
fetch(TIO &io, const char *name, double *v)
{
  if (io.out) {
    (*io.out) << "    <param name='";
    if (io.row>=0) (*io.out) << io.row;
    (*io.out)<<name<<"' value='"<<*v<<"'/>\n";
    return;
  }
  if (io.found || strcasecmp(io.name, name)!=0) return;
  sscanf(io.value, "%lf", v);
  io.found=true;
}

void
fetch(TIO &io, const char *name, TIntegerModel *v)
{
  if (io.out) {
    (*io.out) << "    <param name='";
    if (io.row>=0) (*io.out) << io.row;
    (*io.out)<<name<<"' value='"<<*v<<"'/>\n";
    return;
  }
  if (io.found || strcasecmp(io.name, name)!=0) return;
  *v = atoi(io.value);
  io.found=true;
}

void
fetch(TIO &io, const char *name, int *v)
{
  if (io.out) {
    (*io.out) << "    <param name='";
    if (io.row>=0) (*io.out) << io.row;
    (*io.out)<<name<<"' value='"<<*v<<"'/>\n";
    return;
  }
  if (io.found || strcasecmp(io.name, name)!=0) return;
  *v = atol(io.value);
  io.found=true;
}

void
fetch(TIO &io, const char *name, EShape *v)
{
  if (io.out) {
    (*io.out) << "    <param name='";
    if (io.row>=0) (*io.out) << io.row;
    (*io.out)<<name<<"' value='"<<*v<<"'/>\n";
    return;
  }
  if (io.found || strcasecmp(io.name, name)!=0) return;
  *v = (EShape)atoi(io.value);
  io.found=true;
}

bool
TTree::load(const char *filename)
{
  xmlTextReaderPtr reader = xmlReaderForFile(filename, NULL, 0);
  if (!reader)
    return false;
  bool result = true;
  TIO io;
  while(true) {
    if (!xmlTextReaderRead(reader)) {
//cout << __FILE__ << ":" << __LINE__ << endl;
//      result = false;
      break;
    }
    
    const xmlChar *name, *value;
    name = xmlTextReaderConstName(reader);
    if (!name) {
      result = false;
      break;
    }
    int depth = xmlTextReaderDepth(reader);
    int type  = xmlTextReaderNodeType(reader);

    value = xmlTextReaderConstValue(reader);
/*
    cout << "depth:" << depth
         << ", type:" << xmlTextReaderNodeType(reader)
         << ", name:" << name
         << ", empty:" << (xmlTextReaderIsEmptyElement(reader)?"yes":"no")
         << ", value:" << (xmlTextReaderHasValue(reader)?"yes":"no")
         << ", attrs:" << xmlTextReaderAttributeCount(reader)
         << endl;
*/
    if (depth==1 && type==1 && name && strcmp((char*)name, "species")==0) {
      name = xmlTextReaderGetAttribute(reader, (xmlChar*)"name");
      if (name) {
        species = (const char*)name;
      }
    } else
    
    if (depth==2 && type==1 && name && strcmp((char*)name, "param")==0) {
      io.name = (char*)xmlTextReaderGetAttribute(reader, (xmlChar*)"name");
      io.value = (char*)xmlTextReaderGetAttribute(reader, (xmlChar*)"value");
      io.found = false;
      if (io.name && io.value) {
        io1(io);
        if (io.found) continue;
        
        if (isdigit(*io.name)) {
          unsigned row = 0;
          while(isdigit(*io.name)) {
            row *= 10;
            row += (*io.name)-'0';
            ++io.name;
          }
          while(stem.size()<=row)
            stem.push_back(TStem());
          io2(io, stem[row]);
        }
        
        if (!io.found) {
          cout << "unknown attribute " << io.name << " = " << io.value << endl;
        }
      }
    }
    
  }
  xmlFreeTextReader(reader);
  xmlCleanupParser();

  return result;
}

bool
TTree::save(const char *filename)
{
  ofstream out(filename);
  if (!out)
    return false;
  out << "<?xml version='1.0' ?>\n"
         "<!-- created with MakeTree -->\n"
         "<arbaro>\n"
         "  <species name='"<<species<<"'>\n"
         "  <!-- general params -->\n";
  TIO io;
  io.row = -1;
  io.out = &out;
  io1(io);
cout << "we have " << stem.size() << " stems" << endl;
  for(int i=0; i<stem.size(); ++i) {
    out << "    <!-- level " << i << " -->\n";
    io.row = i;
    io2(io, stem[i]);
  }
  out << "  </species>\n"
         "</arbaro>\n";
  return true;
}

void
TTree::io1(TIO &io)
{
  fetch(io, "Levels", &levels);
  fetch(io, "Shape", &shape);
  fetch(io, "Scale", &scale);
  fetch(io, "ScaleV", &scalev);
  fetch(io, "BaseSize", &basesize);
  fetch(io, "0BaseSplits", &basesplits);
  fetch(io, "RatioPower", &ratiopower);
  fetch(io, "AttractionUp", &attractionup);
  fetch(io, "Ratio", &ratio);
  fetch(io, "Flare", &flare);
  fetch(io, "Lobes", &lobes);
  fetch(io, "LobeDepth", &lobedepth);
  fetch(io, "0Scale", &scale0);
  fetch(io, "0ScaleV", &scale0v);
  fetch(io, "Leaves", &leaves);
  fetch(io, "LeafShape", &leafshape);
  fetch(io, "LeafScale", &leafscale);
  fetch(io, "LeafScaleX", &leafscalex);
  fetch(io, "LeafBend", &leafbend);
  fetch(io, "LeafStemLen", &leafstemlen);
  fetch(io, "LeafDistrib", &leafdistrib);
  fetch(io, "PruneRatio", &prune_ratio);
  fetch(io, "PruneWidth", &prune_width);
  fetch(io, "PruneWidthPeak", &prune_width_peak);
  fetch(io, "PrunePowerLow", &prune_power_low);
  fetch(io, "PrunePowerHigh", &prune_power_high);
  fetch(io, "LeafQuality", &leafquality);
  fetch(io, "Smooth", &smooth);
  fetch(io, "RandomSeed", &random);
}        

void
TTree::io2(TIO &io, TStem &s)
{
  fetch(io, "Length", &s.length);
  fetch(io, "LengthV", &s.lengthv);
  fetch(io, "Taper", &s.taper);
  fetch(io, "CurveRes", &s.curveres);
  fetch(io, "Curve", &s.curve);
  fetch(io, "CurveV", &s.curvev);
  fetch(io, "CurveBack", &s.curveback);
  fetch(io, "SegSplits", &s.segsplits);
  fetch(io, "SplitAngle", &s.splitangle);
  fetch(io, "SplitAngleV", &s.splitanglev);
  fetch(io, "DownAngle", &s.downangle);
  fetch(io, "DownAngleV", &s.downanglev);
  fetch(io, "Rotate", &s.rotate);
  fetch(io, "RotateV", &s.rotatev);
  fetch(io, "Branches", &s.branches);
  fetch(io, "BranchDist", &s.branchesdist);
}

double
TTree::shapeRatio(EShape shape, double ratio) const
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
      if (ratio < (1 - prune_width_peak) && (ratio > 0.0))
         return pow(ratio/(1 - prune_width_peak), prune_power_high);
      if (ratio >= (1 - prune_width_peak) && ratio < 1.0)
        return pow((1 - ratio)/prune_width_peak, prune_power_low);
      return 0;
  }
  return 1.0;
}


double
taper(double taper, double radius, double segment, double length)
{
  double unit_taper = 0.0;
  if (0.0 <= taper && taper < 1.0) {
    unit_taper = taper;
  } else
  if (1.0 <= taper && taper < 2.0) {
    unit_taper = 2.0 - taper;
  } else
  if (2.0 <= taper && taper < 3.0) {
    unit_taper = 0.0;
  }

  double taper_z;
  double z = segment / length;
  taper_z = radius * ( 1.0 - unit_taper * z );

  double radius_z;
  if (0.0 <= taper && taper <= 1.0) {
    radius_z = taper_z;
  } else
  if (0.0 <= taper && taper <= 3.0) {
    double z2 = ( 1.0 - z ) * length;
    double depth;
    if (taper <= 2.0 || z2 < taper_z ) {
      depth = 1.0;
    } else {
      depth = taper - 2.0;
    }
    double z3;
    if ( taper < 2.0 ) {
      z3 = z2;
    } else {
      z3 = z2 - 2.0 * taper_z * fabs(z2 / ( 2.0 * taper_z) + 0.5 );
      if (z3 < 0.0)
        z3 = -z3;
    }
    if (taper < 2.0 && z3 >= taper_z) {
      radius_z = taper_z;
    } else {
      radius_z = (1-depth)*taper_z+depth*sqrt(pow(taper_z, 2.0)-pow(z3-taper_z, 2.0));
    }
  }
  return radius_z;
}

void
render(const Matrix &m,
       const TTree &tree,
       unsigned lvl=0,
       double length_parent=0.0,
       double radius_parent=0.0,
       double offset_child=0.0);

void renderSegment(size_t &ring0,
                   const Matrix &m,
                   const TTree &tree,
                   unsigned lvl,
                   double length_parent,
                   double radius_parent,
                   double offset_child,
                   
                   double radius,
                   double length,
                   double segment,
                   double segmentLength,
                   unsigned children,
                   double length_base,
                   double length_child_max,
                   double dist,
                   double ldist,
                   double leaves_per_branch,
                   double &r,
                   double &lr,
                   double &segsplits_error,
                   double split_angle_correction,
                   bool &alternate
                   )
{
  if (myPeekMessage()) {
    recalculate = true;
    return;
  }

  if (segment+segmentLength*0.9>=length)
    return;
//cout << "renderSegment: lvl="<<lvl<<", segment=" << segment << ", length="<<length<<", children="<<children<<", length_base="<<length_base<<endl;

  // curve rotation  
  double d;
  if (tree.stem[lvl].curvev>=0.0) {
    d = trandom2(tree.stem[lvl].curvev)/tree.stem[lvl].curveres;
    if (tree.stem[lvl].curveback==0.0)
      d += tree.stem[lvl].curve/tree.stem[lvl].curveres;
    else if (segment<tree.stem[lvl].curveres/2)
      d += tree.stem[lvl].curve/(tree.stem[lvl].curveres/2);
    else
      d += tree.stem[lvl].curveback/(tree.stem[lvl].curveres/2);
  } else {
    // the stem is a helix, with a declanation of tree.stem[lvl].curvevary
    cout << "NOTE: curvev < 0, stem is a helix, is not implemented yet" << endl;
  }
  d -= split_angle_correction;
  Matrix m1 = matrixRotate(Vector(1.0, 0.0, 0.0), d) * m;

  unsigned segsplits_effective = 0;
  if (segment==0.0) {
  } else
  if (lvl==0 && segment==segmentLength && tree.basesplits>0.0) {
    segsplits_effective = tree.basesplits;
  } else {
    segsplits_effective = fabs(tree.stem[lvl].segsplits + segsplits_error);
    segsplits_error -= segsplits_effective - tree.stem[lvl].segsplits;
  }

  double declination;
  {
    Vector v0(0.0, 0.0, 0.0);
    Vector v1(0.0, 1.0, 0.0);
    v0 = m1 * v0;
    v1 = m1 * v1;
    Vector v2 = v1 - v0;
    v2.normalize();
    declination = acos(v2.y()) * 180.0 / M_PI;
  }

  // determine global y vector
  Vector v;
  {
    Matrix m = m1.inverse();
    Vector v0(0,0,0);
    Vector v1(0,1,0);
    v0 = m * v0;
    v1 = m * v1;
    v = v1 - v0;
    v.normalize();
  }

  double splitangle = - (tree.stem[lvl].splitangle*segsplits_effective /*+
                     trandom(tree.stem[lvl].splitanglev*segsplits_effective)*/ -
                                        declination) * segsplits_effective / 2.0;

  for(unsigned i=0; i<=segsplits_effective; ++i) {

//cout << lvl << ": render split " << i << " out of " << segsplits_effective << endl;
    Matrix m2 = m1;

    if ((lvl!=0 || segment>segmentLength)  // this is not in the paper but it looks better this way
        && i!=0)
    {
      double spread_rotation = 20 + 0.75 * 				// 121.51
                               ( 30.0 + fabs(declination-90.0) ) * pow(trandom(1.0), 2.0);
      if (trandom(1.0) < 0.0)						// 121.52
        spread_rotation = -spread_rotation;
        m2 = matrixRotate(v, spread_rotation) * m2;
    }

    if (segsplits_effective>0) {
      if (i>0)
      splitangle += tree.stem[lvl].splitangle*segsplits_effective + 
                   trandom(tree.stem[lvl].splitanglev*segsplits_effective) -
                   declination;
      m2 = matrixRotate(Vector(0.0, 1.0, 0.0), splitangle) * m2;
      
      split_angle_correction += splitangle /  ((length-segment)/segmentLength);
    }

    double radius_z = taper(tree.stem[lvl].taper, radius, segment, length);

    size_t ring1;
    drawSegment(ring0, ring1, m2, segmentLength, radius_z, i==0 && segment==0.0);

    // render children (merge this one with 'render leaves') !!!
    if (children!=0 && dist!=0.0 && leaves_per_branch<=0.0) {
      double off0 = segment;
      double off1 = segment + segmentLength;

//cout << "children: off0="<<off0<<", off1="<<off1;

      if (lvl==0) {
        if (off1 <= length_base) { // below base, draw nothing
          off0 = off1+1.0;
        } else
        if (off0 < length_base && length_base < off1) {
          off0 = length_base;
        }
      }
//cout << " -> off0="<<off0<<", off1="<<off1<<", dist="<<dist<<", children="<<(off1-off0)/dist<<endl;
    
      off0 -= segment;
      off1 -= segment;

      for(; off0<off1; off0+=dist) {
        double offsetChild = off0 + segment;

//cout << "draw child at off=" << off0 << ", segment=" << segment << endl;
        
        Matrix m3 = matrixTranslate(0.0, off0, 0.0) * m2;
//cout << __FILE__ << ":" << __LINE__ << ": translate for child " << off0 << endl;

        if (tree.stem[lvl+1].rotate>=0.0) {
          r += tree.stem[lvl+1].rotate + trandom(tree.stem[lvl+1].rotatev);
        } else {
          alternate = !alternate;
          r = tree.stem[lvl+1].rotate + trandom(tree.stem[lvl+1].rotatev);
          if (alternate)
            r = -r;
        }

        // attraction up
        if (lvl!=0 && tree.attractionup != 0.0) {
          Vector v0(0.0, 0.0, 0.0);
          Vector v1(0.0, 1.0, 0.0);
          v0 = m3 * v0;
          v1 = m3 * v1;
          Vector v = v1 - v0;
          v.normalize();
          
          double declination = acos(v.y());
          double orientation = acos(v.z());
          double curve_up_segment = tree.attractionup * declination * cos(orientation) / tree.stem[lvl+1].curveres;
          r += curve_up_segment * 180.0 / M_PI;
        }

        m3 = matrixRotate(Vector(0.0, 1.0, 0.0), r) * m3;

        double downangle_child;
        if (tree.stem[lvl+1].downanglev >= 0.0) {
          downangle_child = tree.stem[lvl+1].downangle + trandom2(tree.stem[lvl+1].downanglev);
        } else {
          downangle_child = 
            tree.stem[lvl+1].downangle + trandom2(
              tree.stem[lvl+1].downanglev *
              ( 1.0 - 2.0 * tree.shapeRatio(SHAPE_CONICAL, (length-offsetChild)/(length-(lvl==0?length_base:0.0)))) );
        }
        m3 = matrixRotate(Vector(1.0, 0.0, 0.0), downangle_child) * m3;

        double length_child;
        if (lvl==0) {
          double length_base = tree.basesize * tree.scale;
          length_child = length *
                         length_child_max *
                         tree.shapeRatio(tree.shape, (length - offset_child)/(length-length_base));
        } else {
          length_child = length_child_max * ( length /*_parent*/ - 0.6 * offset_child );
        }

        render(m3, tree, lvl+1, length, radius_z, offsetChild);
        if (myPeekMessage()) {
          recalculate = true;
          return;
        }
      }
    }

    // render leaves
    if (leaves_per_branch>0.0) {
      unsigned ll = lvl+1;
/*
      if (ll>2)
        ll=2;
      if (ll+1>tree.stem.size())
        ll=tree.stem.size()-1;
*/    
      bool alternate = false;
      for(double off=0.0; off<segmentLength; off+=ldist) {
        double offsetChild = off + segment;

//cout << "lvl: leaf at offset " << off << endl;
        Matrix m3 = matrixTranslate(Vector(0.0, off, 0.0)) * m2;
//cout << __FILE__ << ":" << __LINE__ << ": translate for leaf" << endl;

        if (tree.stem[ll].rotate>=0.0) {
          lr += tree.stem[ll].rotate + trandom(tree.stem[ll].rotatev);
        } else {
          alternate = !alternate;
          lr = tree.stem[ll].rotate + trandom(tree.stem[ll].rotatev);
          if (alternate)
            lr = -lr;
        }

        m3 = matrixRotate(Vector(0.0, 1.0, 0.0), lr) * m3;

        double downangle_child;
        if (tree.stem[ll].downanglev >= 0.0) {
          downangle_child = tree.stem[ll].downangle + trandom2(tree.stem[ll].downanglev);
        } else {
          downangle_child = 
            tree.stem[ll].downangle + 
              trandom2(tree.stem[ll].downanglev *
              ( 1.0 - 2.0 * tree.shapeRatio(SHAPE_CONICAL, (length-offsetChild)/length)) );
        }
        m3 = matrixRotate(Vector(1.0, 0.0, 0.0), downangle_child) * m3;

        // leaf orientation
        if (tree.leafbend!=0.0) {
          Vector pos(0.0, 0.0, 0.0);
          pos = m3 * pos;
          
          Vector normal(0.0, 1.0, 0.0);
          normal = m3 * pos;
          normal -= pos;
          normal.normalize();
          
          double theta_position = atan2(pos.z(), pos.x());
          double theta_bend     = theta_position - atan2(normal.z(), normal.x());
          double rz = tree.leafbend * theta_bend * 180.0 / M_PI;
          m3 = matrixRotate(Vector(0,1,0), rz) * m3;
          
          pos = Vector(0.0, 0.0, 0.0);
          pos = m * pos;
          normal = Vector(0.0, 1.0, 0.0);
          normal = m * normal;
          normal -= pos;
          normal.normalize();
          
          double phi_bend = 
            atan2(
              sqrt(
                pow((double)normal.x(), 2.0)+pow((double)normal.z(), 2.0)), (double)normal.y()
            );
          double orientation = acos(normal.y()) * 180.0 / M_PI;
          
          m3 = matrixRotate(Vector(0.0, 1.0, 0.0), -orientation) * m3;
          m3 = matrixRotate(Vector(1.0, 0.0, 0.0), tree.leafbend * phi_bend * 180.0 / M_PI) * m3;
          m3 = matrixRotate(Vector(0.0, 1.0, 0.0), orientation) * m3;
        }
        drawLeaf(m3, tree);
        
      } // for(double off=0.0; off<segmentLength; off+=ldist) {
    } // if (leaves_per_branch>0.0) {

    m2 = matrixTranslate(Vector(0.0, segmentLength, 0.0)) * m2;

    renderSegment(ring1, m2, 
      tree, lvl, length_parent, radius_parent, offset_child,
      radius, length, segment+segmentLength, segmentLength, children, length_base, length_child_max,
      dist, ldist, leaves_per_branch,
      r, lr, segsplits_error, split_angle_correction, alternate);
      if (myPeekMessage()) {
        recalculate=true;
        return;
      }
  }
}

void
render(const Matrix &m,
       const TTree &tree,
       unsigned lvl,
       double length_parent,
       double radius_parent,
       double offset_child)
{
  if (myPeekMessage()) {
    recalculate = true;
    return;
  }

// cout << "render: lvl=" << lvl << endl;

  double length_child_max=0.0;
    
  length_child_max = tree.stem[lvl].length + trandom(tree.stem[lvl].lengthv);

  // double stemLength()
  double length;
  if (lvl==0) {
    length = tree.stem[0].length + trandom(tree.stem[0].lengthv) * tree.scale; // 122.6
  } else if (lvl==1) {
    double length_base = tree.basesize * tree.scale;
    length = length_parent *                                                   // 121.94
             length_child_max *
             tree.shapeRatio(tree.shape, (length_parent - offset_child)/
                                         (length_parent-length_base) );
  } else {
    length = length_child_max * (length_parent - 0.6 * offset_child);          // 122.1
  }

  // stem radius
  double radius;
  if (lvl==0) {
    radius = length * tree.ratio * tree.scale0;					// 122.
  } else {
    radius = pow(radius_parent * (length / length_parent ), tree.ratiopower);	// 122.
  }

  // void prepareSubstemParams()
  unsigned children=0; // substem_cnt
  if (lvl==0) {
    children = tree.stem[1].branches;
//cout << __FILE__ << ":" << __LINE__ << " lvl = " << lvl << " children = " << children << endl;
  } else
  if (lvl==1) {
    children = tree.stem[2].branches *                                         // 121.78
                ( 0.2 + 0.8 * ( length / length_parent ) / length_child_max);
//cout << __FILE__ << ":" << __LINE__ << " lvl = " << lvl << " children = " << children << endl;
  } else
  if ( (lvl+1 < tree.stem.size()) || (lvl+1 < tree.levels) ) {
      children = tree.stem[lvl+1].branches *                                   // 121.80
                 ( 1.0 - 0.5 * offset_child / length_parent);
//cout << __FILE__ << ":" << __LINE__ << " lvl = " << lvl << " children = " << children << endl;
  }

  assert(tree.stem[lvl].curveres>0);
  double segmentLength = length / tree.stem[lvl].curveres;
  double length_base = 0.0;
  if (lvl==0)
    length_base = tree.basesize * length;

  double dist=0;
  if (children!=0) {
    if (lvl==0) {
      dist = (length - length_base) / children;
//cout << "  1 dist = " << dist << endl;
    } else {
      dist = length / children;
//cout << "  2 dist = " << dist << endl;
    }
//cout << "length="<<length<<", length_base="<<length_base<<", children="<<children<<", children per segment:" << segmentLength/dist << endl;
  }
  
  double leaves_per_branch = 0.0;				 // 122.
  double ldist = 0.0;
  if ( lvl+1==tree.stem.size() || lvl+1==tree.levels ) {
    leaves_per_branch =
      tree.leaves * 
      tree.shapeRatio(SHAPE_TAPERED_CYLINDRICAL, offset_child/length_parent) * tree.leafquality;
    ldist = length / leaves_per_branch;
  }
  
  double r=0.0;  // children rotation
  double lr=0.0; // leaf rotation
  double segsplits_error = 0.0;

  bool alternate = false;
  size_t ring0;
  renderSegment(ring0, m,
    tree, lvl, length_parent, radius_parent, offset_child,
    radius, length, 0.0, segmentLength, children, length_base, length_child_max,
    dist, ldist, leaves_per_branch,
    r, lr, segsplits_error, 0.0, alternate);
}

Vector operator*(const Vector &v, const Matrix &m) {
  return m.operator*(v);
}

void 
drawSegment(size_t &ring0, size_t &ring1, const Matrix &m, GLfloat l, GLfloat r, bool initializeRing0)
{
  GLfloat s = 2.0 * M_PI / stem_res;
  GLfloat s0=0.0;
  
  Vector v[stem_res];
  for(unsigned i=0; i<stem_res; ++i) {
    s0 += s;
    v[i][0] = sin(s0);
    v[i][1] = 0;
    v[i][2] = cos(s0);
  }
  
  if (initializeRing0) {
    ring0 = normals[0].size();
    for(unsigned i=0; i<stem_res; ++i) {
      normals[0].push_back( v[i] * m );
      points[0].push_back( Vector(v[i][0]*r, 0, v[i][2]*r) * m);
    }
  }

  ring1 = normals[0].size();
  for(unsigned i=0; i<stem_res; ++i) {
    normals[0].push_back( v[i] * m );
    points[0].push_back( Vector(v[i][0]*r, l, v[i][2]*r) * m);
  }
  
  for(unsigned i=0; i<stem_res; ++i) {
    unsigned i1 = (i+1) % stem_res;
    faces[0].push_back(ring0+i1);
    faces[0].push_back(ring1+i1);
    faces[0].push_back(ring1+i);
    faces[0].push_back(ring0+i);
  }
}

void
drawLeaf(const Matrix &m, const TTree &tree)
{
  double f = sqrt(tree.leafquality);
  double sy=0.035 * tree.leafscale / f;
  double sx=sy * tree.leafscalex;

  Vector v[8] = {
    Vector(    0.0,    0.0, 0.0),
    Vector( sx*1.0, sy*1.0, 0.0),
    Vector( sx*1.0, sy*2.0, 0.0),
    Vector( sx*0.5, sy*3.0, 0.0),
    Vector(    0.0, sy*3.2, 0.0),
    Vector(-sx*0.5, sy*3.0, 0.0),
    Vector(-sx*1.0, sy*2.0, 0.0),
    Vector(-sx*1.0, sy*1.0, 0.0),
  };
  
  for(unsigned i=0; i<9; ++i) {
    v[i] *= m;
  }

  Vector n(0.0, 0.0, -1.0);
  n = n * m;
  n = n.normalize();
  
  size_t leaf = normals[1].size();
  for(unsigned i=0; i<8; ++i) {
    points[1].push_back(v[i]);
    normals[1].push_back(n);
  }

  unsigned face_list[] = {
    0, 1, 6, 7,
    1, 2, 5, 6,
    2, 3, 4, 5
  };
  
  for(unsigned i=0; i<12; ++i)
    faces[1].push_back(leaf+face_list[i]);
}


TTree tree;

TTree::TTree()
{
  levels.setMinimum(1);
  basesize.setMinimum(0.0);
  basesize.setMaximum(0.9999);

  stem.push_back(TStem());
  stem.push_back(TStem());
  stem.push_back(TStem());
  stem.push_back(TStem());

  random = RANDOM_RANDOM;

  species = "quaking_aspen";
  shape = SHAPE_TEND_FLAME;
  levels = 3;
  scale = 13.0;
  scalev = 3.0;
  basesize = 0.4;
  basesplits = 0.0;
  ratiopower = 1.2;
  attractionup = 0.5;

  ratio = 0.015;
  flare = 0.6;
  lobes = 5;
  lobedepth = 0.07;
  scale0 = 1.0;
  scale0v = 0.2;

  leaves = 25;
  leafshape = 0;
  leafscale = 0.17;
  leafscalex = 1.0;
  leafbend = 0.3;
  leafstemlen = 0.5;
  leafdistrib = 4;
  
  prune_ratio = 0.0;
  prune_width = 0.5;
  prune_width_peak = 0.5;
  prune_power_low = 0.5;
  prune_power_high = 0.5;
  
  leafquality = 1.0;
  smooth = 0.5;

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
  stem[0].branches = 1;
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
  stem[2].rotate = 140.0;
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
        case  3: handleInteger(te, &container->stem[te.col].curveres, 0, 1, 1); break;
        case  4: handleDouble(te, &container->stem[te.col].curve, 0, 1); break;
        case  5: handleDouble(te, &container->stem[te.col].curvev, 0, 1); break;
        case  6: handleDouble(te, &container->stem[te.col].curveback, 0, 1); break;
        case  7: handleDouble(te, &container->stem[te.col].segsplits); break;
        case  8: handleDouble(te, &container->stem[te.col].splitangle, 0, 1); break;
        case  9: handleDouble(te, &container->stem[te.col].splitanglev, 0, 1); break;
        case 10: handleDouble(te, &container->stem[te.col].downangle, 0, 1); break;
        case 11: handleDouble(te, &container->stem[te.col].downanglev, 0, 1); break;
        case 12: handleDouble(te, &container->stem[te.col].rotate, 0, 1); break;
        case 13: handleDouble(te, &container->stem[te.col].rotatev, 0, 1); break;
        case 14: handleInteger(te, &container->stem[te.col].branches, 0, 1, 0); break;
        case 15: handleDouble(te, &container->stem[te.col].branchesdist, 0, 1); break;
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
    :TWindow(p,t){modified=false;};
  protected:
    TViewer *gl;
    void invalidateGL();
    void create();

    void menuNew();
    void menuOpen();
    bool menuSave();
    void menuSaveAs();
    void menuExport();
    void menuQuit();
    void menuInfo();
    void menuCopyright();

    void closeRequest();
    string filename;
    bool modified;
    bool _Save(const string& title);
    bool _Check();
};

void
TMainWindow::menuNew()
{
}

void
TMainWindow::menuOpen()
{
  TFileDialog dlg(this, "Open..");
  dlg.doModalLoop();
  if (dlg.getResult()!=TMessageBox::OK)
    return;

  TTree t;

  setlocale(LC_NUMERIC, "C");
  bool b = t.load(dlg.getFilename().c_str());
  setlocale(LC_NUMERIC, "");
  if (b) {
    ::tree.assign(t);
    
    filename = dlg.getFilename();
    //  setTitle(programname+ ": " + basename((char*)filename.c_str()));
    setTitle(programname+ ": " + filename);
    modified = false;
    recalculate = true;
    gl->invalidateWindow();
  } else {
      messageBox(NULL, 
                 programname+": Open..",
                 "Failed to load " + dlg.getFilename(),
                 TMessageBox::ICON_STOP |
                 TMessageBox::OK);
  }
}

bool
TMainWindow::menuSave()
{
  string title = "Save";
  if (filename.size()==0) {
    TFileDialog dlg(this, title, TFileDialog::MODE_SAVE);
    dlg.doModalLoop();
    if (dlg.getResult()==TMessageBox::OK) {
      filename = dlg.getFilename();
    } else {
      return false;
    }
  }  
  return _Save(title);
}
 
void
TMainWindow::menuSaveAs()
{
  string title = "Save As..";

  TFileDialog dlg(this, title, TFileDialog::MODE_SAVE);
  dlg.setFilename(filename);
  dlg.doModalLoop();
  if (dlg.getResult()==TMessageBox::OK) {
    filename = dlg.getFilename();
    if (_Save(title))
//      setTitle(programname+ ": " + basename((char*)filename.c_str()));
      setTitle(programname+ ": " + filename);
  }
}  

/**
 * Save file and use `title' as the title for the message boxes
 */
bool
TMainWindow::_Save(const string &title)
{
  struct stat st;

  bool makebackup = false;
    
  // check original filename
  //-------------------------
  if (stat(filename.c_str(), &st)==0) {
    if (S_ISDIR(st.st_mode)) {
      messageBox(NULL, 
                 title,
                 "You've specified a directory but not a filename.",
                 TMessageBox::ICON_STOP |
                 TMessageBox::OK);
      return false;
    }
    if (!S_ISREG(st.st_mode)) {
      messageBox(NULL,
                 title,
                 "Sanity check. You haven't specified a regular file.",
                 TMessageBox::ICON_STOP |
                 TMessageBox::OK);
      return false;
    }
    if (st.st_mode & S_IFLNK) {
      char real[PATH_MAX];
      if (realpath(filename.c_str(), real) == NULL) {
        messageBox(NULL,
                   title,
                   "Internal error: realpath failed to resolve symlink.",
                   TMessageBox::ICON_STOP |
                   TMessageBox::OK);
        return false;
      }
      filename=real;
    }
    makebackup = true;
  }
   
  // check backup filename
  //-----------------------
  string backupfile = filename+"~";
  if (stat(backupfile.c_str(), &st)==0) {
    if (!S_ISREG(st.st_mode)) {
      if (messageBox(NULL,
                     title,
                     "I can't create the backup file.\n\n"
                     "Do you want to continue?",
                     TMessageBox::ICON_QUESTION |
                     TMessageBox::YES | TMessageBox::NO
                    ) != TMessageBox::YES)
      {
        return false;
      }
      makebackup = false;
    }
  }  

  if (makebackup) {
    if (rename(filename.c_str(), backupfile.c_str())!=0) {
      if (messageBox(NULL,
                     title,
                     "Failed to create the backup file.\n\n"
                     "Do you want to continue?",
                     TMessageBox::ICON_QUESTION |
                     TMessageBox::YES | TMessageBox::NO |
                     TMessageBox::BUTTON2) != TMessageBox::YES)
      {
        return false;
      }
    }  
  }    
  
  setlocale(LC_NUMERIC, "C");
  bool b = tree.save(filename.c_str());
  setlocale(LC_NUMERIC, "");
  if (!b) {
    if (makebackup)
      rename(backupfile.c_str(), filename.c_str());
    messageBox(NULL,
               title,
               "Damn! I've failed to create the file.",
               TMessageBox::ICON_EXCLAMATION | TMessageBox::OK);
    return false;
  }
  modified = false;
  return true;
}

void
TMainWindow::menuExport()
{
//  exporter = new LWOF();
//  ofstream out("tree.lwo");
  exporter = new WFOF();
  
  string filename2 = filename.substr(0, filename.rfind('.'));
  filename2 += ".obj";
  ofstream out(filename2.c_str());
//  surface_branch = exporter->createSurface("branch", 0.5, 0.0, 0.0);
//  surface_leaf   = exporter->createSurface("leaf", 0.0, 0.8, 0.0);
  
//  srand(0);
//  render(Matrix(), tree);

  exporter->write(&out);

  delete exporter;
  exporter = 0;
  
  printf("export done\n");
}

void
TMainWindow::closeRequest()
{
  if (!_Check())
    return;
  TWindow::closeRequest();
  // delete this; // delete window when closed
  sendMessageDeleteWindow(this);
}

bool
TMainWindow::_Check()
{ 
//  cout << "_Check: _toad_ref_cntr=" << ta->getModel()->_toad_ref_cntr << endl;

  if (modified) {
    unsigned r = messageBox(NULL,
                   "Buffer is modified",
                   "Do you want to save the current file?",
                   TMessageBox::ICON_QUESTION |
                   TMessageBox::YES | TMessageBox::NO );
    if (r==TMessageBox::YES) {
      if (!menuSave())
        return false;
    } else if (r!=TMessageBox::NO) {
      return false;
    }
  }
  return true;
}


int
main(int argc, char **argv, char **envv)
{
/*
  TTree t;
  t.load("bug001.xml");
  tree.assign(t);
*/
  toad::initialize(argc, argv, envv); {
    TMainWindow wnd(NULL, "MakeTree");
    toad::mainLoop();
  } toad::terminate();
  return 0;
}

void TMainWindow::invalidateGL()
{
  modified = true;
  recalculate = true;
  gl->invalidateWindow();
}

// TMainWindow
//--------------------------------------------------------------------
void TMainWindow::create()
{
  setSize(740,780);
  setBackground(TColor::DIALOG);

  // create menubar
  //----------------
  TMenuBar *mb=new TMenuBar(this, "mb");
  mb->loadLayout("menubar.atv");

  new TUndoManager(this, "undomanager");
  TAction *action;
  action = new TAction(this, "file|new");
  CONNECT(action->sigClicked, this, menuNew);
  action = new TAction(this, "file|open");
  CONNECT(action->sigClicked, this, menuOpen);
  action = new TAction(this, "file|save_as");
  CONNECT(action->sigClicked, this, menuSaveAs);
  action = new TAction(this, "file|save");
  CONNECT(action->sigClicked, this, menuSave);
  action = new TAction(this, "file|export");
  CONNECT(action->sigClicked, this, menuExport);
  action = new TAction(this, "file|quit");
  CONNECT(action->sigClicked, this, closeRequest);

/*  
  action = new TAction(this, "file|quit");
  CONNECT(action->sigClicked, this,menuQuit);

  action = new TAction(this, "help|info");
  CONNECT(action->sigClicked, this,menuInfo);

  action = new TAction(this, "help|copyright");
  CONNECT(action->sigClicked, this,menuCopyright);
*/

  // 'maketree --layout-editor' || make layout
  TWindow *dlg = new TWindow(this, "dlg");
  dlg->setBackground(TColor::DIALOG);
  TWindow *w;
  new TTextField(dlg, "species", &tree.species);
  new TTextField(dlg, "random", &tree.random);

  TComboBox *cb;
  
  cb = new TComboBox(dlg, "shape");
  cb->setAdapter(new TCStringTableAdapter(shapenames));
  cb->setSelectionModel(&tree.shape);
  
  cb = new TComboBox(dlg, "randomopt");
  cb->setAdapter(new TCStringTableAdapter(randomnames));
  cb->setSelectionModel(&::randomopt);

  new TTextField(dlg, "levels", &tree.levels);
  new TTextField(dlg, "scale", &tree.scale);
  new TTextField(dlg, "scalev", &tree.scalev);
  new TTextField(dlg, "basesize", &tree.basesize);
  new TTextField(dlg, "basesplits", &tree.basesplits);
  w = new TTextField(dlg, "ratiopower", &tree.ratiopower);
  w->setToolTip("Ratiopower: A higher setting will cause smaller child stems.");
  new TTextField(dlg, "attractionup", &tree.attractionup);
  new TTextField(dlg, "ratio", &tree.ratio);
  new TTextField(dlg, "flare", &tree.flare);
  new TTextField(dlg, "lobes", &tree.lobedepth);
  new TTextField(dlg, "scale0", &tree.scale0);
  new TTextField(dlg, "scale0v", &tree.scale0v);
  new TTextField(dlg, "leaves", &tree.leaves);
  new TTextField(dlg, "leafshape", &tree.leafshape);
  new TTextField(dlg, "leafscale", &tree.leafscale);
  new TTextField(dlg, "leafscalex", &tree.leafscalex);
  new TTextField(dlg, "leafbend", &tree.leafbend);
  new TTextField(dlg, "leafstemlen", &tree.leafstemlen);
  new TTextField(dlg, "leafdistrib", &tree.leafdistrib);
  new TTextField(dlg, "prune_ratio", &tree.prune_ratio);
  new TTextField(dlg, "prune_width", &tree.prune_width);
  new TTextField(dlg, "prune_width_peak", &tree.prune_width_peak);
  new TTextField(dlg, "prune_power_low", &tree.prune_power_low);
  new TTextField(dlg, "prune_power_high", &tree.prune_power_high);
  new TTextField(dlg, "leafquality", &tree.leafquality);
  new TTextField(dlg, "smooth", &tree.smooth);
  dlg->loadLayout("dlg.atv");

  CONNECT(tree.random.sigChanged, this, invalidateGL);
  CONNECT(::randomopt.sigChanged, this, invalidateGL);
  CONNECT(tree.levels.sigChanged, this, invalidateGL);
  CONNECT(tree.shape.sigChanged, this, invalidateGL);
  CONNECT(tree.scale.sigChanged, this, invalidateGL);
  CONNECT(tree.scalev.sigChanged, this, invalidateGL);
  CONNECT(tree.basesize.sigChanged, this, invalidateGL);
  CONNECT(tree.basesplits.sigChanged, this, invalidateGL);
  CONNECT(tree.ratiopower.sigChanged, this, invalidateGL);
  CONNECT(tree.attractionup.sigChanged, this, invalidateGL);
  CONNECT(tree.ratio.sigChanged, this, invalidateGL);
  CONNECT(tree.flare.sigChanged, this, invalidateGL);
  CONNECT(tree.lobes.sigChanged, this, invalidateGL);
  CONNECT(tree.scale0.sigChanged, this, invalidateGL);
  CONNECT(tree.scale0v.sigChanged, this, invalidateGL);
  CONNECT(tree.leaves.sigChanged, this, invalidateGL);
  CONNECT(tree.leafshape.sigChanged, this, invalidateGL);
  CONNECT(tree.leafscale.sigChanged, this, invalidateGL);
  CONNECT(tree.leafscalex.sigChanged, this, invalidateGL);
  CONNECT(tree.leafbend.sigChanged, this, invalidateGL);
  CONNECT(tree.leafstemlen.sigChanged, this, invalidateGL);
  CONNECT(tree.leafdistrib.sigChanged, this, invalidateGL);
  CONNECT(tree.prune_ratio.sigChanged, this, invalidateGL);
  CONNECT(tree.prune_width.sigChanged, this, invalidateGL);
  CONNECT(tree.prune_width_peak.sigChanged, this, invalidateGL);
  CONNECT(tree.prune_power_low.sigChanged, this, invalidateGL);
  CONNECT(tree.prune_power_high.sigChanged, this, invalidateGL);
  CONNECT(tree.leafquality.sigChanged, this, invalidateGL);
  CONNECT(tree.smooth.sigChanged, this, invalidateGL);

  // create viewer
  //---------------
  gl = new TViewer(this, "gl");

  // create stem table
  //-------------------
  TTable *tbl = new TTable(this, "tbl");

  TTableAdapter *adapter = new TTreeAdapter(&tree);
  CONNECT(adapter->sigChanged, this, invalidateGL);
  tbl->setAdapter(adapter);
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

  layout->attach("dlg", TSpringLayout::TOP, "mb");
  layout->attach("dlg", TSpringLayout::LEFT);

  layout->attach("tbl", TSpringLayout::TOP, "dlg");
  layout->attach("tbl", TSpringLayout::LEFT | TSpringLayout::BOTTOM);

  layout->attach("gl", TSpringLayout::TOP, "mb");
  layout->attach("gl", TSpringLayout::RIGHT | TSpringLayout::BOTTOM);
  layout->attach("gl", TSpringLayout::LEFT, "tbl");
  
  layout->distance("gl", 2);

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
    "Copyright (C) 2011 by Mark-André Hopf <mhopf@mark13.org>\n\n"
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

double distance = 2.0;

void
TViewer::glPaint()
{
  glClearColor( 0.0, 0.0, 0.5, 0.0 );
  glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

  glMatrixMode( GL_PROJECTION );
  glLoadIdentity();
  
  double aspect = (double)getWidth() / (double)getHeight();
  
  glFrustum( -aspect,    // left
              aspect,    // right
              -1.0,   // bottom
              1.0,    // top
              1.0,    // near
              40.0    // far
  );
  
  glMatrixMode( GL_MODELVIEW );
  glLoadIdentity();

  glEnable(GL_DEPTH_TEST);
  // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  
  glEnable(GL_LIGHTING);
  glEnable(GL_COLOR_MATERIAL);

  GLfloat diffuse0[] = { 0.4, 0.4, 0.4 };
  GLfloat ambient0[] = { 0.4, 0.4, 0.4 };
  GLfloat lightPos0[] = { -100, 200, 0 };
  glEnable(GL_LIGHT0);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse0);
  glLightfv(GL_LIGHT0, GL_AMBIENT, ambient0);
  glLightfv(GL_LIGHT0, GL_POSITION, lightPos0);
/*
  GLfloat diffuse1 = { 0,1,0 };
  GLfloat ambient1 = { 0,1,0 };
  GLfloat lightPos1 = { 200, 200, -200 };
  glEnable(GL_LIGHT1);
  glLightfv(GL_LIGHT1, GL_DIFFUSE, diffuse1 };
  glLightfv(GL_LIGHT1, GL_AMBIENT, ambient1 };
  glLightfv(GL_LIGHT1, GL_POSITION, lightPos1);
*/
  glTranslatef(0.0, -1.0, -::distance);

  observer.glMultMatrix();

  if (recalculate) {
    setCursor(TCursor::WAIT);
    recalculate = false;
    for(int i=0; i<2; ++i) {
      normals[i].clear();
      points[i].clear();
      faces[i].clear();
    }
    srand(tree.random);
    render(Matrix(), tree);
    if (!recalculate)
      setCursor(TCursor::DEFAULT);
  }

  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_NORMAL_ARRAY);

  glColor3f(0.5, 0.0, 0.0);
  glEnable(GL_CULL_FACE);
  glVertexPointer(3, GL_FLOAT, 0, points[0].front());
  glNormalPointer(GL_FLOAT, 0, normals[0].front());
  glDrawElements(GL_QUADS, faces[0].size(), GL_UNSIGNED_INT, &faces[0].front());

  glColor3f(0.0, 0.5, 0.0);
  glDisable(GL_CULL_FACE);
  glVertexPointer(3, GL_FLOAT, 0, points[1].front());
  glNormalPointer(GL_FLOAT, 0, normals[1].front());
  glDrawElements(GL_QUADS, faces[1].size(), GL_UNSIGNED_INT, &faces[1].front());
  
  glDisableClientState(GL_VERTEX_ARRAY);
  glDisableClientState(GL_NORMAL_ARRAY);

/*
  GLuint id;
  glGenBuffers(1, &id);
  glBindBuffer(GL_ARRAY_BUFFER, id);
  
  // when we pass '0' instead of 'points' we can use glMapBuffer and glUnmapBuffer
  glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STREAM_DRAW);

  glVertexPointer(3, GL_FLOAT, 0, 0);
  glEnableClientState(GL_VERTEX_ARRAY);
  glDrawArrays(GL_QUADS, 0, 6); // glMultiDrawArrays
  glDisableClientState(GL_VERTEX_ARRAY);       
*/  

/*
  glScaled(10.0,10.0,10.0); // this is said to be slow for light stuff
  Matrix m;
  drawSegment(m, 0.1, 0.02);
  m = matrixTranslate(Vector(0,0.1,0)) * m;
  m = matrixRotate(Vector(0,0,1), 45) * m;
  drawSegment(m, 0.1, 0.02);
*/

  if (recalculate) {
    invalidateWindow();
  }
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
      ::distance -= 0.02;
      invalidateWindow();
      break;
    case TMouseEvent::ROLL_DOWN:
      ::distance += 0.02;
      invalidateWindow();
      break;
  }
}

// obj: format
// http://en.wikipedia.org/wiki/Obj
// counter clock wise order
// # <comment>
// v <x> <y> <z>
// vn <vertex> <vertex> ...

// f