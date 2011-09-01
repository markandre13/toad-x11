/*
 * Fischland -- A 2D vector graphics editor
 * Copyright (C) 1999-2007 by Mark-André Hopf <mhopf@mark13.org>
 * Visit http://www.mark13.org/fischland/.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * still unhandled cases:
 * - bezier (4 point segment of path) intersects with itself
 * - more than 2 bezier intersect at the same point
 * - intersections with non-TFPath figures
 */

#ifndef DEBUG

#include <cmath>
#include <cfloat>
#include <algorithm>

#include "filltool.hh"
#include "fpath.hh"
#include "cairo.hh"

#ifdef DEBUG
#undef DEBUG
#endif

#else

#include <toad/toad.hh>
#include <toad/messagebox.hh>
#include <toad/figuremodel.hh>
#include <toad/figureeditor.hh>
#include "fpath.hh"
#include <cmath>
#include <cfloat>
#endif

#define DEBUG_PDF 1

#ifdef DEBUG_PDF
#include <sstream>
#endif
static TPenBase *pen = 0;

#define TEST07 1

static void divideBezier(const TPoint *a, TPoint *p, TCoord u=0.5);
static void divideBezier(const TFPath *a, TPoint *p, TCoord u=0.5);
static bool lineIntersection(const TPoint &a0, const TPoint &a1,
                             const TPoint &b0, const TPoint &b1);
static bool pointsOverlap(const TPoint *p, TCoord u, const TPoint *q, TCoord v, TCoord range=1.0);
static bool pointsOverlap(const TFPath *p0, TCoord u0, const TFPath *p1, TCoord u1, TCoord range=1.0);

struct IntersectionPoint {
  IntersectionPoint(const TPoint &p, TCoord u, TCoord v, const TFPath *pu, const TFPath *pv):
    coord(p), a(u), b(v), ap(pu), bp(pv) {}
  TPoint coord;
  TCoord a, b;
  const TFPath *ap, *bp;
};
typedef vector<IntersectionPoint> IntersectionPointList;

static void
bezierIntersection(IntersectionPointList &found,
                   const TPoint *a,
                   const TPoint *b,
                   const TFPath *ap,
                   const TFPath *bp,
                   TCoord as=0.0, TCoord bs=0.0,
                   TCoord a0=0.0, TCoord a1=1.0,
                   TCoord b0=0.0, TCoord b1=1.0,
                   unsigned d=0);

static void
bezierIntersection(IntersectionPointList &found,
                   const TFPath *ap,
                   const TFPath *bp);

struct direction_t {
  direction_t(TCoord aDegree, const TFPath *aPath, TCoord aPosition, bool aForward)
    :degree(aDegree), path(aPath), position(aPosition), forward(aForward)
  {}
  TCoord degree;
  const TFPath *path;
  TCoord position;
  bool forward;
};
typedef vector<direction_t> directionstack_t;

struct segment_t {
  segment_t() {
    path = 0;
    u0 = u1 = 0.0;
    mina = 0.0;
    neara = 0.0;
    nearpath = 0;
  }
  // found path segment
  const TFPath *path;
  TCoord u0, u1;

  // mina we found during the last time we've searched 'list'
  TCoord mina;

  // values for reset
  TCoord neara;
  const TFPath *nearpath;
};
typedef vector<segment_t> segmentstack_t;

static bool
insidepath(TCoord x,
           const IntersectionPointList &linelist,
           const segmentstack_t &stack);

static TFPath* segmentstack2path(segmentstack_t &stack);
static TFPath* floodfill(TFigureModel &model, TCoord x, TCoord y);

static inline TCoord
fac(unsigned n)
{
  TCoord u = 1;
  for(unsigned i=2; i<=n; ++i)
    u *= i;
  return u;
}

static inline TCoord
C(unsigned n, unsigned k)
{
  return fac(n) / (fac(k) * fac(n-k));
}

static inline TCoord
B(unsigned i, TCoord u)
{
  return C(3, i) * powl(1.0-u, 3-i) * powl(u, i);
}

#ifdef DEBUG_PDF
static void
bez2point(const TFPath *path, TCoord u, TPoint *p)
{
  size_t i = 0;
  while(u>1.0) {
    u-=1.0;
    i+=3;
  }
  p->x = p->y = 0;
  for(unsigned j=0; j<4; ++j) {
    TCoord b = B(j, u);
    p->x += path->polygon[i+j].x * b;
    p->y += path->polygon[i+j].y * b;
  }
}
#endif

using namespace fischland;

TFillTool*
TFillTool::getTool()
{
  static TFillTool* tool = 0;
  if (!tool)
    tool = new TFillTool();
  return tool;
}

void
TFillTool::mouseEvent(TFigureEditor *fe, const TMouseEvent &me)
{
  TCoord x, y;
  TFPath *path;
  
  switch(me.type) {
    case TMouseEvent::LDOWN:
      fe->mouse2sheet(me.x, me.y, &x, &y);
      fe->getWindow()->setCursor(TCursor::WAIT);
      toad::flush();
      path = floodfill(*fe->getModel(), x, y);
      fe->getWindow()->setCursor(TCursor::DEFAULT);
      if (path) {
        path->removeable = true;
        path->closed = true;
        fe->getAttributes()->reason = TFigureAttributes::ALLCHANGED;
        path->setAttributes(fe->getAttributes());
        fe->getModel()->add(path);
      }
      break;
    default:
      ;
  }
}

void
divideBezier(const TPoint *a, TPoint *p, TCoord u)
{
  if (u==0.5) {
    p[0] = a[0];
    p[1].x = (a[0].x+a[1].x)*0.5;
    p[1].y = (a[0].y+a[1].y)*0.5;
    p[5].x = (a[2].x+a[3].x)*0.5;
    p[5].y = (a[2].y+a[3].y)*0.5;
    p[6] = a[3];
  
    TCoord cx = (a[1].x+a[2].x)*0.5;
    TCoord cy = (a[1].y+a[2].y)*0.5;
    p[2].x = (p[1].x+cx)*0.5;
    p[2].y = (p[1].y+cy)*0.5;
    p[4].x = (cx+p[5].x)*0.5;
    p[4].y = (cy+p[5].y)*0.5;

    p[3].x = (p[2].x+p[4].x)*0.5;
    p[3].y = (p[2].y+p[4].y)*0.5;
    return;
  }
  p[0] = a[0];
  p[1].x = a[0].x + (a[1].x-a[0].x)*u;
  p[1].y = a[0].y + (a[1].y-a[0].y)*u;
  p[5].x = a[2].x + (a[3].x-a[2].x)*u;
  p[5].y = a[2].y + (a[3].y-a[2].y)*u;
  p[6] = a[3];
  
  TCoord cx = a[1].x + (a[2].x-a[1].x)*u;
  TCoord cy = a[1].y + (a[2].y-a[1].y)*u;
  p[2].x = p[1].x + (cx-p[1].x)*u;
  p[2].y = p[1].y + (cy-p[1].y)*u;
  p[4].x = cx + (p[5].x-cx)*u;
  p[4].y = cy + (p[5].y-cy)*u;

  p[3].x = p[2].x+(p[4].x-p[2].x)*u;
  p[3].y = p[2].y+(p[4].y-p[2].y)*u;
}

void
divideBezier(const TFPath *a, TPoint *p, TCoord u)
{
  size_t i = 0;
  while(u>1.0) {
    u-=1.0;
    i+=3;
  }
  TPoint pt[4];
  for(size_t j=0; j<4; ++j) {
    pt[j].x = a->polygon[i+j].x;
    pt[j].y = a->polygon[i+j].y;
  }
  divideBezier(pt, p, u);
}

double
angleAtPoint(const TFPath *a, TCoord u)
{
  size_t i = 0;
  while(u>1.0) {
    u-=1.0;
    i+=3;
  }
  if (u<=0.0) {
    return atan2(a->polygon[i+1].y - a->polygon[i+0].y,
                 a->polygon[i+1].x - a->polygon[i+0].x);
  }
  if (u>=1.0) {
    return atan2(a->polygon[i+3].y - a->polygon[i+2].y,
                 a->polygon[i+3].x - a->polygon[i+2].x);
  }
  TPoint pt[4], p[7];
  for(size_t j=0; j<4; ++j) {
    pt[j].x = a->polygon[i+j].x;
    pt[j].y = a->polygon[i+j].y;
  }
  divideBezier(pt, p, u);
  return atan2(p[4].y-p[3].y, p[4].x-p[3].x);
}

bool
lineIntersection(const TPoint &a0,
                 const TPoint &a1,
                 const TPoint &b0,
                 const TPoint &b1)
{
  //   a0        + α·(a1 -a0)  = b0 + β·(b1-b0)
  // ⇔ a0  - b0  + α·(a1 -a0)  =      β·(b1 -b0)
  //
  // ⇔ ax0 - bx0 + α·(ax1-ax0) =      β·(bx1-bx0)
  //   ay0 - by0 + α·(ay1-ay0) =      β·(by1-by0)
  //
  // ⇔ ax0 - bx0 + α·ax        = β·bx
  //   ay0 - by0 + α·ay        = β·by
  //
  // ⇔(ax0 - bx0 + α·ax) / bx  = β
  //   ay0 - by0 + α·ay        = β·by
  //
  // ⇔ ay0 - by0 + α·ay        = (ax0 - bx0 + α·ax) / bx · by
  // ⇔ ay0 - by0 + α·ay        = (ax0 - bx0)/bx·by + α·ax/bx·by
  // ⇔ α·ay - α·ax/bx·by       = (ax0 - bx0)/bx·by - ay0 + by0
  // ⇔ α·(ay-ax/bx·by)         = (ax0 - bx0)/bx·by - ay0 + by0
  // ⇔ α = ((ax0 - bx0)/bx·by - ay0 + by0) / (ay-ax/bx·by)
  
  TCoord ax = a1.x - a0.x;
  TCoord ay = a1.y - a0.y;
  TCoord bx = b1.x - b0.x;
  TCoord by = b1.y - b0.y;

  if (bx>ax) {
    TCoord a = ((a0.x-b0.x)/bx*by - a0.y + b0.y)/(ay-ax/bx*by);

    if (a<0.0 || a>1.0)
      return false;
    TCoord b = (a0.x - b0.x + a * ax) / bx;

//cout << "a=" << a << ", b="<<b<<endl;
    return b>=0.0 && b<=1.0;
  }
  TCoord b = ((b0.x-a0.x)/ax*ay - b0.y + a0.y)/(by-bx/ax*ay);

  if (b<0.0 || b>1.0)
    return false;
  TCoord a = (b0.x - a0.x + b * bx) / ax;

//cout << "a=" << a << ", b="<<b<<endl;
  return a>=0.0 && a<=1.0;
}

bool
pointsOverlap(const TPoint *p, TCoord u, const TPoint *q, TCoord v, TCoord range)
{
  TCoord x0 = 0;
  TCoord y0 = 0;
  TCoord x1 = 0;
  TCoord y1 = 0;
  for(unsigned i=0; i<4; ++i) {
    TCoord b;
    b = B(i, u);
    x0 += p[i].x * b;
    y0 += p[i].y * b;
    b = B(i, v);
    x1 += q[i].x * b;
    y1 += q[i].y * b;
  }
  if (fabsl(x1-x0)<range && fabsl(y1-y0)<range) {
    return true;
  }
//if (range>=1.0) cout<<"    points "<<u<<" "<<v<<" do not overlap "<<x0<<","<<y0<<" "<<x1<<","<<y1<<endl;
  return false;
}

bool
pointsOverlap(const TFPath *p0, TCoord u0, const TFPath *p1, TCoord u1, TCoord range)
{
  TPoint a0[4], a1[4];
  size_t i0=0, i1=0;
  while(u0>1.0) {
    i0+=3;
    u0-=1.0;
  }
  while(u1>1.0) {
    i1+=3;
    u1-=1.0;
  }
  for(size_t i=0; i<4; ++i) {
    a0[i].x = p0->polygon[i+i0].x;
    a0[i].y = p0->polygon[i+i0].y;
    a1[i].x = p1->polygon[i+i1].x;
    a1[i].y = p1->polygon[i+i1].y;
  }
  return pointsOverlap(a0, u0, a1, u1, range);
}

bool foo=false;
double m;

void
bezierIntersection(IntersectionPointList &found,
                   const TPoint *a,
                   const TPoint *b,
                   const TFPath *ap,
                   const TFPath *bp,
                   TCoord as, TCoord bs,
                   TCoord a0, TCoord a1,
                   TCoord b0, TCoord b1,
                   unsigned d)
{
#if 0
  static int foo=0;
  ++foo;
  if (d==0 || foo>=807 || true) {
    cout <<"    bezierIntersection: "<<foo<<endl
         <<"      ("<<a[0].x<<","<<a[0].y<<"), ("
         <<a[1].x<<","<<a[1].y<<"), ("
         <<a[2].x<<","<<a[2].y<<"), ("
         <<a[3].x<<","<<a[3].y<<")"<<endl
         <<"      ("<<b[0].x<<","<<b[0].y<<"), ("
         <<b[1].x<<","<<b[1].y<<"), ("
         <<b[2].x<<","<<b[2].y<<"), ("
         <<b[3].x<<","<<b[3].y<<")"<<endl;
  }
#endif
  bool done;
#if 0
  done = true;
  for(int i=1; i<4; ++i) {
    if (fabs(a[0].x-a[i].x)>0.5 || fabs(a[0].y-a[i].y)>0.5) {
      done = false;
      break;
    }
  }
  if (done) {
    for(int i=0; i<4; ++i) {
      if (fabs(a[0].x-b[i].x)>0.5 || fabs(a[0].y-b[i].y)>0.5) {
        done = false;
        break;
      }
    }
  }
  if (done) {
    cout << "same point" << endl;
  }
#endif
  // return no intersection when both beziér are the same
  done = true;
  for(int i=0; i<4; ++i) {
    if (a[i].x!=b[i].x || a[i].y!=b[i].y) {
      done = false;
      break;
    }
  }
  if (done) {
    cout << "same line" << endl;
    return;
  }
  done = true;
  for(int i=0; i<4; ++i) {
    if (a[i].x!=b[3-i].x || a[i].y!=b[3-i].y) {
      done = false;
      break;
    }
  }
  if (done) {
    cout << "same line reverted" << endl;
    return;
  }
#if 0
//cout << "depth " << d << endl;
if (d&1)
  cout << "depth " << d << " " << b0 << " - " << b1 << ", " << a0 << " - " << a1 << endl;
else
  cout << "depth " << d << " " << a0 << " - " << a1 << ", " << b0 << " - " << b1 << endl;
#endif
if (d>100) {
  cout << "**** depth is above 100, emergency abort" << endl;
  return;
}

  done = true;

  // intersection check with bounding rectangle
  TCoord ax0, ax1, ay0, ay1;
  ax0=ax1=a[0].x;
  ay0=ay1=a[0].y;
  TCoord bx0, bx1, by0, by1;
  bx0=bx1=b[0].x;
  by0=by1=b[0].y;
  for(int i=1; i<4; ++i) {
    if (ax0>a[i].x) ax0=a[i].x;
    if (ax1<a[i].x) ax1=a[i].x;
    if (ay0>a[i].y) ay0=a[i].y;
    if (ay1<a[i].y) ay1=a[i].y;
    if (bx0>b[i].x) bx0=b[i].x;
    if (bx1<b[i].x) bx1=b[i].x;
    if (by0>b[i].y) by0=b[i].y;
    if (by1<b[i].y) by1=b[i].y;
  }
/*
cout << "ax: " << ax0 << " - " << ax1
     <<" ay: " << ay0 << " - " << ay1 << endl;
cout << "bx: " << bx0 << " - " << bx1
     <<" by: " << by0 << " - " << by1 << endl;
*/
  if ((ax0<=bx0 && bx0<=ax1 &&
       ax0<=bx1 && bx1<=ax1 &&
       ay0<=by0 && by0<=ay1 &&
       ay0<=by1 && by1<=ay1) ||
      (bx0<=ax0 && ax0<=bx1 &&
       bx0<=ax1 && ax1<=bx1 &&
       by0<=ay0 && ay0<=by1 &&
       by0<=ay1 && ay1<=by1))
  {
//    cout << "contains" << endl;
  } else {
    // line intersection
    done = true;
    for(int i=0; i<4; ++i) {
      for(int j=0; j<4; ++j) {
        if (lineIntersection(a[i], a[(i+1)%4], b[j], b[(j+1)%4]))
          done = false;
      }
    }
    if (done) {
//      cout << "no line intersections" << endl;
      return;
    }
  }


  TPoint p[7];
  divideBezier(a, p);

  done = true;
  
  double m0 = 0.1;
  for(int i=1; i<4; ++i) {
    if (fabs(a[0].x-a[i].x)>m0 || fabs(a[i].y-a[i].y)>m0 ||
        fabs(b[0].x-b[i].x)>m0 || fabs(b[i].y-b[i].y)>m0) 
    {
      done = false;
      break;
    }
  }
  if (done) {
    m = min(m, fabs(b[0].x-p[3].x));
    m = min(m, fabs(b[0].y-p[3].y));
    if (fabs(b[0].x-p[3].x)>m0 || fabs(b[0].y-p[3].y)>m0)
      done = false;
  }

  if (done) {
//    cout << "draw circle" << endl;
#ifdef DEBUG
    if (paintflag) {
      pen->drawCirclePC(a[0].x-2, a[0].y-2, 5, 5);
    }
#endif
#if 0
    cout << " depth=" << d << ", intersection " 
         << bp << ":" << (b0+(b1-b0)/2.0+bs) << "; "
         << ap << ":" << (a0+(a1-a0)/2.0+as) << endl;
#endif
    if (d & 1) {
      found.push_back(IntersectionPoint(a[0], b0+(b1-b0)/2.0+bs, a0+(a1-a0)/2.0+as, bp, ap));
    } else {
      found.push_back(IntersectionPoint(a[0], a0+(a1-a0)/2.0+as, b0+(b1-b0)/2.0+bs, ap, bp));
    }
    return;
  }

  done = true;
  for(int i=1; i<4; ++i) {
    if (a[0].x!=a[i].x || a[0].y!=a[i].y) {
      done = false;
      break;
    }
  }
  if (done) {
    done = true;
    for(int i=1; i<4; ++i) {
      if (b[0].x!=b[i].x || b[0].y!=b[i].y) {
        done = false;
        break;
      }
    }
    if (done)
      bezierIntersection(found, b, a, bp, ap, bs, as, b0, b1, a0, a1, d+1);
    else
      cout << "SINGLE POINTS WHICH DO NOT OVERLAPP" << endl;
    return;
  }

  bezierIntersection(found, b, p,   bp, ap, bs, as, b0, b1, a0            , a0+(a1-a0)/2.0, d+1);
  bezierIntersection(found, b, p+3, bp, ap, bs, as, b0, b1, a0+(a1-a0)/2.0, a1            , d+1);
}

void
bezierIntersection(IntersectionPointList &found,
                   const TFPath *ap,
                   const TFPath *bp)
{
  size_t oldsize = found.size();

  TPoint a[4], b[4];
  TCoord as = 0.0;
  for(size_t i=0; i+3<ap->polygon.size(); i+=3) {
    for(size_t u=0; u<4; ++u) {
      a[u].x = ap->polygon[i+u].x;
      a[u].y = ap->polygon[i+u].y;
    }
    TCoord bs = 0.0;
    for(size_t j=0; j+3<bp->polygon.size(); j+=3) {
      for(size_t u=0; u<4; ++u) {
        b[u].x = bp->polygon[j+u].x;
        b[u].y = bp->polygon[j+u].y;
      }
/*
cout << "    intersect segment a" << i << " with segment b" << j << endl;
for(size_t j=0; j<4; ++j) {
  cout << "      a["<<j<<"].set("<<a[j].x<<","<<a[j].y<<");"<<endl;
}
for(size_t j=0; j<4; ++j) {
  cout << "      b["<<j<<"].set("<<b[j].x<<","<<b[j].y<<");"<<endl;
}
*/
      bezierIntersection(found, a, b, ap, bp, as, bs);
      bs += 1.0;
    }
    as += 1.0;
  }

  // end points may not overlap with the other path but might be near
  // enough to look like on the screen

//cout << "look at end points" << endl;
  TCoord d, x, y, f;
assert(!ap->polygon.empty());
assert(!bp->polygon.empty());
  d = bp->findPointNear(ap->polygon[0].x, ap->polygon[0].y, &x, &y, &f);
//cout << "distance 1 = " << d << endl;
//printf("d=%f\n", d);
  if (d<1.0) {
    found.push_back(IntersectionPoint(ap->polygon[0], 0, f, ap, bp));
  }
  d = bp->findPointNear(ap->polygon.back().x, ap->polygon.back().y, &x, &y, &f);
//cout << "distance 2 = " << d << endl;
  if (d<1.0) {
//cout << "polygon.size() = " << ap->polygon.size() << " -> " << (ap->polygon.size()-1)/3 << ", f="<< f << endl;
    found.push_back(IntersectionPoint(ap->polygon.back(), (ap->polygon.size()-1)/3, f, ap, bp));
  }
  d = ap->findPointNear(bp->polygon[0].x, bp->polygon[0].y, &x, &y, &f);
//cout << "distance 3 = " << d << endl;
  if (d<1.0) {
//    found.push_back(IntersectionPoint(bp->polygon[0], 0, f, bp, ap));
    found.push_back(IntersectionPoint(bp->polygon[0], f, 0, ap, bp));
  }
  d = ap->findPointNear(bp->polygon.back().x, bp->polygon.back().y, &x, &y, &f);
//cout << "distance 4 = " << d << endl;
  if (d<1.0) {
//cout << "polygon.size() = " << ap->polygon.size() << " -> " << (ap->polygon.size()-1)/3 << endl;
//    found.push_back(IntersectionPoint(bp->polygon.back(), (bp->polygon.size()-1)/3, f, bp, ap));
    found.push_back(IntersectionPoint(bp->polygon.back(), f, (bp->polygon.size()-1)/3, ap, bp));
  }
//cout << "looked at end points" << endl;

}

/**
 * check if a point is inside a given bezier path
 * \param x
 *   x coordinate of the point
 * \param linelist
 *   list of intersections of a horizontal line through point (x,y) with
 *   all known beziers
 * \param stack
 *   
 */
bool
insidepath(TCoord x,
           const IntersectionPointList &linelist,
           const segmentstack_t &stack)
{
cout << "insidepath" << endl;
  if (stack.empty())
    return false;
  unsigned intersections = 0;
  for(IntersectionPointList::const_iterator p0 = linelist.begin();
      p0 != linelist.end();
      ++p0)
  {
    if (x <= p0->coord.x)
      continue;
/*
    cout << "  p0->coord.x=" << p0->coord.x
         << ", p0->bp=" << p0->bp
         << ", p0->b=" << p0->b
         << endl;
*/
    for(segmentstack_t::const_iterator p1 = stack.begin()+1;
        p1 != stack.end();
        ++p1)
    {
      if (p0->bp!=p1->path)
        continue;
      if (p1->u0 <= p1->u1) {
        if ( p1->u0 <= p0->b && p0->b <= p1->u1)
          ++intersections;
      } else {
        if ( p1->u1 <= p0->b && p0->b <= p1->u0)
          ++intersections;
      }
      cout << "    p1->path=" << p1->path
           << ", p1->u0=" << p1->u0
           << ", p1->u1=" << p1->u1
           << endl; 
    }
  }
  cout << "intersections = " << intersections << endl;
return true;
  return intersections!=0 && (intersections&1);
}

/**
 * Create an TFPath object based the bezier path stored in 'stack'.
 *
 * There's a way to optimize this: When neigbouring segments are on
 * the same line, we can skip the points in the middle.
 */
TFPath*
segmentstack2path(segmentstack_t &stack)
{
//cout << "-------------------- segmentstack2path --------------------" << endl;
  size_t cntr = 0;
  TFPath *path = new TFPath;
  for(segmentstack_t::const_iterator p = stack.begin();
      p != stack.end();
      ++p)
  {
    if (p==stack.begin())
      continue;
    cout << "segment " << ++cntr << ": path " << p->path << " from " << p->u0 << " to " << p->u1 << endl;
    TCoord u0 = p->u0, u1=p->u1;
//if (cntr==2) {
//  u0 = p->u1, u1=p->u0;
//}

    size_t i0=0, i1=0;
    while(u0>1.0) {
      u0-=1.0;
      i0+=3;
    }
    while(u1>1.0) {
      u1-=1.0;
      i1+=3;
    }
    
    if (i1+2>=p->path->polygon.size()) {
cout << "******** i1 is out of bounds, fixing it without knowing why..." << endl;
      i1-=3;
      u1 = 1.0;
    }
    
    if (i0==i1) {
//      cout << "  segment is within a single bezier" << endl;
      TPoint in[4], out0[7], out1[7];
      if (u0<u1) {
        for(size_t i=0; i<4; ++i) {
          in[i].x = p->path->polygon[i0+i].x;
          in[i].y = p->path->polygon[i0+i].y;
        }
      } else {
        for(size_t i=0; i<4; ++i) {
          in[3-i].x = p->path->polygon[i0+i].x;
          in[3-i].y = p->path->polygon[i0+i].y;
        }
        u0 = 1.0 - u0;
        u1 = 1.0 - u1;
      }
      divideBezier(in, out0, u0);
      u1 = ( u1 - u0 ) / ( 1 - u0 );
      divideBezier(out0+3, out1, u1);
      for(int i=path->polygon.empty() ? 0 : 1; i<4; ++i) {
//cout << "0 cntr:" << out1[i].x << ", " << out1[i].y << endl;
        path->polygon.addPoint(out1[i].x, out1[i].y);
      }
      continue;
    }
    TPoint in[4], out[7];
    if (i0>i1) {
//cout << "  u0="<<u0<<", u1="<<u1<<endl;
      // this segment is backwards
      // head
      for(size_t i=0; i<4; ++i) {
        in[3-i].x = p->path->polygon[i0+i].x;
        in[3-i].y = p->path->polygon[i0+i].y;
      }
      divideBezier(in, out, 1.0-u0);
#ifdef DEBUG
pen->drawRectangle(out[3].x-2,out[3].y-2,5,5);
#endif
      for(int i=path->polygon.empty() ? 3 : 4; i<7; ++i) {
        path->polygon.addPoint(out[i].x, out[i].y);
//cout << "1 head:" << out[i].x << ", " << out[i].y << endl;
      }
      // middle
      for(size_t j=i0-3; j>i1; j-=3) {
//        printf("middle from %u to %u, doing %u\n", i0, i1, j);
        for(size_t i=0; i<4; ++i) {
          assert(j+i< p->path->polygon.size());
          out[3-i].x = p->path->polygon[j+i].x;
          out[3-i].y = p->path->polygon[j+i].y;
        }
        for(int i=1; i<4; ++i) {
          path->polygon.addPoint(out[i].x, out[i].y);
//cout << "1 midl:" << out[i].x << ", " << out[i].y << endl;
        }
      }

      // tail
      for(size_t i=0; i<4; ++i) {
        in[3-i].x = p->path->polygon[i1+i].x;
        in[3-i].y = p->path->polygon[i1+i].y;
      }
      divideBezier(in, out, 1.0-u1);
      for(int i=1; i<4; ++i) {
//cout << "1 tail:" << out[i].x << ", " << out[i].y << endl;
        path->polygon.addPoint(out[i].x, out[i].y);
      }
    } else {
      // head
//cout << "2 head: i0="<<i0<<", i1="<<i1<<", size="<<p->path->polygon.size()<<endl;
//for(size_t i=0; i<p->path->polygon.size(); ++i)
//  cout << i << ": " << p->path->polygon[i].x << ", " << p->path->polygon[i].y << endl;
      for(size_t i=0; i<4; ++i) {
        in[i].x = p->path->polygon[i0+i].x;
        in[i].y = p->path->polygon[i0+i].y;
      }
      divideBezier(in, out, u0);
#ifdef DEBUG
pen->drawRectangle(out[3].x-2,out[3].y-2,5,5);
#endif
      for(int i=path->polygon.empty() ? 3 : 4; i<7; ++i) {
//cout << "2 head:" << out[i].x << ", " << out[i].y << endl;
        path->polygon.addPoint(out[i].x, out[i].y);
      }
      // middle
      for(size_t j=i0+3; j<i1; j+=3) {
//        printf("middle from %u to %u, doing %u\n", i0, i1, j);
        for(size_t i=0; i<4; ++i) {
          assert(j+i< p->path->polygon.size());
          out[i].x = p->path->polygon[j+i].x;
          out[i].y = p->path->polygon[j+i].y;
        }
        for(int i=1; i<4; ++i) {
//cout << "2 midl:" << out[i].x << ", " << out[i].y << endl;
          path->polygon.addPoint(out[i].x, out[i].y);
        }
      }
    
      // tail
      for(size_t i=0; i<4; ++i) {
if (i1+i>=p->path->polygon.size()) {
  fprintf(stderr,"%s:%u: assertion\n",__FILE__,__LINE__);
  return 0;
}
        in[i].x = p->path->polygon[i1+i].x;
        in[i].y = p->path->polygon[i1+i].y;
      }
//cout << "divide at " << u1 << endl;
      divideBezier(in, out, u1);
      for(int i=1; i<4; ++i) {
//cout << "2 tail " << i << ":" << out[i].x << ", " << out[i].y << endl;
        path->polygon.addPoint(out[i].x, out[i].y);
      }
    }
  }
  return path;
}

void
printPage(TFigureModel &model)
{
  for(TFigureModel::const_iterator p = model.begin();
      p != model.end();
      ++p)
  {
    if ((*p)->mat) {
      pen->push();
      pen->multiply( (*p)->mat );
      (*p)->paint(*pen);
      pen->pop();
    } else {
      (*p)->paint(*pen);
    }
  }
}

TFPath*
floodfill(TFigureModel &model, TCoord x, TCoord y)
{
#ifdef DEBUG_PDF
  TCairo cairo("test.pdf");
  ::pen = &cairo;
  pen->drawString(100,100, "Page 1");

  pen->scale(1.0/(96.0), 1.0/(96.0));
  pen->setFont("arial,helvetica,sans-serif:size=1152");
  
  printPage(model);

  pen->setLineWidth(96.0);
  pen->setColor(0,0.5,0);
  pen->drawLine(x-200, y-200, x+200, y+200);
  pen->drawLine(x+200, y-200, x-200, y+200);

  pen->showPage();
#endif

#ifdef DEBUG
  pen->setColor(255,0,0);
#endif
  
  // 1. find a figure which is nearest to points (x, y) left side:
  
  // 1.1 calculate all intersections with a horizontal line
  TFPath line;
  line.addPoint(INT_MIN, y);
  line.addPoint(INT_MIN, y);
  line.addPoint(INT_MAX, y);
  line.addPoint(INT_MAX, y);

  IntersectionPointList linelist;
  for (TFigureModel::const_iterator p = model.begin();
       p != model.end();
       ++p)
  {
    const TFPath *path = dynamic_cast<const TFPath*>(*p);
    if (!path)
      continue;
    // filling next to already filled areas failes sometimes, so we
    // do not do this until the real issue is fixed
    if (path->isFilled())
      continue;
    bezierIntersection(linelist, &line, path);
  }

  // 1.2 find the nearest intersection on the left
  TCoord nearx = -DBL_MAX;
  TCoord neara = 0.0;
  bool oldforward = true;
  const TFPath *nearpath = 0;
  for(IntersectionPointList::const_iterator q = linelist.begin();
      q != linelist.end();
      ++q)
  {
    if (q->coord.x < x && q->coord.x > nearx) {
      nearx = q->coord.x;
      nearpath = q->bp;
      neara = q->b;
    }
  }
  
  if (!nearpath) {
    cout << "no intersection with line" << endl;
    return 0;
  }

  // 2. find all figures which intersect with 'nearpath'
  //    and which intersections are near to 'neara' on nearpath

  TCoord mina, minb;
  const TFPath *minp;
#ifdef DEBUG
  TCoord r = 30.0;
#endif

  TCoord a = 0.0;
  minb = neara;
  minp = nearpath;

cout << "left of point is path " << nearpath << " at " << neara << endl;

  nearpath = 0;
  mina = -M_PI;

  segmentstack_t segmentstack;

  segmentstack.push_back(segment_t());
  segmentstack.back().path = minp;
  segmentstack.back().u0 = minb;

  // find segment of minp whose degree is nearest to 'a', clockwise
  // from minp to 'a'
  double b = angleAtPoint(minp, minb);
  double b0 = b + 2.0 * M_PI;
  double b1 = b + 3.0 * M_PI;
  while(b0>a)
    b0 -= 2.0 * M_PI;
  while(b1>a)
    b1 -= 2.0 * M_PI;

  // we might require the segment before the current back()...
  directionstack_t direction;
  direction.push_back(direction_t(b0,     minp,     minb, true));  // forward
  direction.push_back(direction_t(b1,     minp,     minb, false)); // backward

  IntersectionPointList list(linelist);

  for(int loop=0; loop<100; ++loop) {

    cout << "--------------------------- " << loop << " ---------------------------" << endl;
    // add other directions from lines which do overlap with this intersection
    cout << "looking for other points intersecting with " << minp << " at " << minb << endl;
    for(IntersectionPointList::const_iterator q = list.begin();
        q != list.end();
        ++q)
    {
      // list contains duplicates, ignore 'em here for now...
      bool boo = false;
      for(directionstack_t::const_iterator r = direction.begin();
          r != direction.end();
          ++r)
      {
        if (q->bp == r->path && q->b == r->position) {
          boo = true;
        }
      }
      if (boo) continue;
/*
      if (q->ap == nearpath && q->a == mina) {
        cout << "got origin path, ignore" << endl;
        continue;
      }
*/
      if (q->b==minb && q->bp==minp) {
        cout << "got first intersecting path, ignore" << endl;
        continue;
      }
      if (!pointsOverlap(q->bp, q->b, minp, minb, 0.2))
        continue;
cout << "got " << q->bp << " at " << q->b << endl;
      double b = angleAtPoint(q->bp, q->b);
      double b0 = b + 2.0 * M_PI;
      double b1 = b + 3.0 * M_PI;
      while(b0>a)
        b0 -= 2.0 * M_PI;
      while(b1>a)
        b1 -= 2.0 * M_PI;
      direction.push_back(direction_t(b0, q->bp, q->b, true));  // forward
      direction.push_back(direction_t(b1, q->bp, q->b, false)); // backward
    }

#ifdef DEBUG_PDF
printPage(model);

pen->setLineWidth(96.0);
pen->setColor(0,0.5,0);
pen->drawLine(x-200, y-200, x+200, y+200);
pen->drawLine(x+200, y-200, x-200, y+200);

pen->setColor(0,0.5,0);
for(directionstack_t::const_iterator p = direction.begin();
    p != direction.end();
    ++p)
{
  TPoint pt;
  bez2point(p->path, p->position, &pt);
  pen->drawLine(
    pt.x, pt.y,
    pt.x + cos(p->degree) * 800, pt.y + sin(p->degree) * 800);
}
pen->setColor(0,0,0);
ostringstream txt;
txt << "found " << direction.size() << " directions";
pen->drawString(10, 10, txt.str());
pen->showPage();
#endif

    // outer loop:
    //   1st: inner loop: find a direction or backtrace if there's none
    //   2nd: find all new intersections
    while(true) {
      bool backtrace = false;
      bool forward;

      // make a decision which direction to take
cout << "  taking a new direction" << endl;
      while(true) {
        // all direction values are below 'a', now find the largest one
        TCoord dm = -DBL_MAX;
        directionstack_t::iterator dp = direction.end();
        for(directionstack_t::iterator p = direction.begin();
            p != direction.end();
            ++p)
        {
          if (!p->path)
            continue;
cout << "    offered: degree=" << p->degree << ", path=" << p->path << endl;
          if (dm < p->degree) {
            dm = p->degree;
            dp = p;
          }
        }
        
        // when we have a new direction, take it
        if (dp!=direction.end()) {
cout << "    taking : degree=" << dp->degree << ", path=" << dp->path << endl;
          nearpath = dp->path;
          dp->path = 0; // mark this direction as used
          neara    = dp->position;
          forward  = dp->forward;
#ifdef DEBUG_PDF
          {
            printPage(model);
            pen->setLineWidth(96.0);
            pen->setColor(0,0.5,0);
            pen->drawLine(x-200, y-200, x+200, y+200);
            pen->drawLine(x+200, y-200, x-200, y+200);
            pen->setColor(0,0.5,0);
            TPoint pt;
            for(directionstack_t::const_iterator p = direction.begin();
                p != direction.end();
                ++p)
            {
              if (!p->path)
                continue;
              bez2point(p->path, p->position, &pt);
              pen->drawLine(
                pt.x, pt.y,
                pt.x + cos(p->degree) * 800, pt.y + sin(p->degree) * 800);
              }
              pen->setColor(1,0.5,0);
              bez2point(nearpath, neara, &pt);
              pen->drawLine(
                pt.x, pt.y,
                pt.x + cos(dp->degree) * 800, pt.y + sin(dp->degree) * 800);
              pen->setColor(0,0,0);
              pen->drawString(10, 10, "taking direction");
              pen->showPage();
          }
#endif
          break;
        }
        
        // all directions were dead ends, backtrace
        if (segmentstack.size()<=1/*empty()*/) {
          cout << "  no minp and can not backtrace (because segmentstack is empty)" << endl;
          return 0;
        }
        cout << "  backtrace, line " << nearpath << " is a dead end" << endl;
        segmentstack.pop_back();
        backtrace = true;
        break;
      } // end of finding a new direction

      oldforward = forward;
      // nearpath:=path to take, neara:=position on nearpath, direction:=...

      // find all beziers intersecting with 'nearpath'
cout << "find all beziers intersecting with " << nearpath << endl;
      list.clear();
      for (TFigureModel::const_iterator p = model.begin();
           p != model.end();
           ++p)
      {
        const TFPath *path = dynamic_cast<const TFPath*>(*p);
        if (!path)
          continue;
        // filling next to already filled areas failes sometimes, so we
        // do not do this until the real issue is fixed
        if (path->isFilled())
          continue;
        if (*p == nearpath) continue; // TEMPORARY AS bezierIntersection MIGHT FAIL HERE FOR NOW
//cout << "  intersect " << nearpath << " with " << path << endl;
        bezierIntersection(list, nearpath, path);
      }
cout << "got " << list.size() << " intersections" << endl;


#ifdef DEBUG
paintflag=false;
#endif

      // store the intersection being closest to 'nearpath' at 'neara' in min*
      cout<<"find nearest intersections out of " << list.size()<<" with "<<nearpath<<", neara="<<neara<<", forward="<<forward<<endl;
      if (forward)
        mina = minb = DBL_MAX;
      else
        mina = minb = -DBL_MAX;
      minp = 0;
#ifdef DEBUG_PDF
double xp = 32000, yp = 10, ys=14*96;
{
printPage(model);
ostringstream txt;
txt<<"forward="<<(forward?"true":"false")<<",_neara="<<neara;
pen->drawString(xp, yp, txt.str()); yp+=ys;
}
#endif

      for(IntersectionPointList::const_iterator q = list.begin();
          q != list.end();
          ++q)
      {
        if (pointsOverlap(nearpath, q->a, nearpath, neara, 1)) {
         cout << "    overlaps, ignore" << endl;
         continue;
        }
#ifdef DEBUG_PDF
{
ostringstream txt;
txt<<"q->a="<<q->a<<",_neara="<<neara<<",_mina="<<mina;
pen->drawString(xp, yp, txt.str()); yp+=ys;
}
#endif
        if (forward) {
          if (q->a >= neara) {
            if (q->a < mina) {
              if (!backtrace || q->a > segmentstack.back().mina) {
                mina = q->a;
                minb = q->b;
                minp = q->bp;
              }
            }
          }
        } else {
          if (q->a <= neara) {
            if (q->a > mina) {
              if (!backtrace || q->a < segmentstack.back().mina) {
                mina = q->a;
                minb = q->b;
                minp = q->bp;
              }
            }
          }
        }
      }

#ifdef DEBUG_PDF
{
pen->setLineWidth(96.0);

// mark start position
pen->setColor(0,0.5,0);
pen->drawLine(x-200, y-200, x+200, y+200);
pen->drawLine(x+200, y-200, x-200, y+200);

{
  TFPath *path = segmentstack2path(segmentstack);
  if (path) {
    pen->setColor(0,1,0);
    path->paint(*pen, TFigure::NORMAL);
    delete path;
  }
}

// mark found intersections
pen->setColor(1,0,0);
TPoint pt;
for(IntersectionPointList::const_iterator q = list.begin();
    q != list.end();
    ++q)
{
  if (nearpath==q->ap) {
    pen->setColor(1,0,0);
    bez2point(q->ap, q->a, &pt);
    pen->drawLine(pt.x-300,pt.y-300,pt.x+300,pt.y+300);
    pen->drawLine(pt.x+300,pt.y-300,pt.x-300,pt.y+300);
  }
  if (nearpath==q->bp) {
    pen->setColor(0,0,1);
    bez2point(q->bp, q->b, &pt);
    pen->drawLine(pt.x-300,pt.y-300,pt.x+300,pt.y+300);
    pen->drawLine(pt.x+300,pt.y-300,pt.x-300,pt.y+300);
  }
}
if (minp) {
  pen->setColor(1,0.5,0);
  bez2point(minp, minb, &pt);
  pen->drawLine(pt.x-300,pt.y-300,pt.x+300,pt.y+300);
  pen->drawLine(pt.x+300,pt.y-300,pt.x-300,pt.y+300);
}

pen->setColor(0,0,0);
if (!backtrace) {
  pen->drawString(10, 10, "collected intersections after taking new direction");
} else {
  pen->drawString(10, 10, "collected intersections after backtrace");
}
if (!minp) {
  pen->drawString(10, 10 + 14 * 96, "no_minp_found");
} else {
  bez2point(nearpath, neara, &pt);
  pen->drawCircle(pt.x-300, pt.y-300, 600, 600);
}
pen->drawString(10, 10 + 2*14 * 96, "circle: indicates current position");
pen->drawString(10, 10 + 3*14 * 96, "orange cross: indicates nearest_point_found");
pen->drawString(10, 10 + 4*14 * 96, "blue crosses: indicate wrong order of intersection entry");
pen->showPage();
}
#endif
    
      if (minp) {
        cout << "found minp" << endl;
        break;
      }
      cout << "no minp found, try again" << endl;
// ??? what about the directionstack dude ???
#warning "USING INVALID DIRECTIONSTACK AFTER BACKTRACE"
    }

    segmentstack.back().mina = mina;
    segmentstack.back().neara = neara;
    segmentstack.back().nearpath = nearpath;

    if (loop>0) {
      segmentstack.back().path = nearpath;
      segmentstack.back().u0 = neara;
      segmentstack.back().u1 = mina; // end of last segment
      // check for loop in already traversed path
      if (segmentstack.size()>1) {
        for(size_t i=1; i<segmentstack.size()-1; ++i) {
          if (pointsOverlap(segmentstack[i].path, segmentstack[i].u0,
                            segmentstack.back().path, segmentstack.back().u1))
          {
            cout << "found end after " << loop << " iterations" << endl;
/*
            cout << "started at  " << segmentstack[1].path << ", " << segmentstack[1].u0 << endl
                 << "finished at " << segmentstack.back().path << ", " << segmentstack.back().u1 << endl;
*/
            // when the end is not the first point, the start point must be outside
            // the area we have found
            if (i>1) {
              cout << "end is not first point, it is " << i << endl;
              segmentstack.erase(segmentstack.begin(), segmentstack.begin()+i-1);
            }
            if (insidepath(x, linelist, segmentstack))
              return segmentstack2path(segmentstack);
            cout << "not inside path" << endl;
            return 0;
          }
        }
      }
    }

    // divide bez0
    //divideBezier(bez0, split0, mina);
    TPoint split0[7];
    divideBezier(nearpath, split0, mina);
  
    cout << "continue with path " << minp << ", mina=" << mina << ", minb=" << minb << endl;

    cout << "neara = " << neara << ", mina=" << mina << endl;
    a = 0.0;
    if (mina>neara) {
      a = atan2(split0[2].y-split0[3].y, split0[2].x-split0[3].x);
    } else {
      // ∡ between split0[4]→split0[3] and split1[3]→spli1[2]
      //                               and split1[3]→spli1[4]
      a = atan2(split0[4].y-split0[3].y, split0[4].x-split0[3].x);
    }

cout << "  path: " << minp << ", mina=" << mina <<", minb=" << minb << endl;
    segmentstack.push_back(segment_t());
    segmentstack.back().path = minp;
    segmentstack.back().u0 = minb;

    direction.clear();
    direction.push_back(direction_t(a-M_PI, nearpath, mina, oldforward));

    // find segment of split1 whose degree is nearest to 'a', clockwise
    // from split1 to 'a'
#if 0
    pen->setColor(255,0,0);
    pen->drawLine(split1[3].x,
                  split1[3].y,
                  split1[3].x + r * cos(a),
                  split1[3].y + r * sin(a));
    pen->setColor(0,255,0);
#endif
//    cout << "at " << split1[3].x << ", " << split1[3].y << endl;
    double b = angleAtPoint(minp, minb);
    double b0 = b + 2.0 * M_PI;
    double b1 = b + 3.0 * M_PI;
    while(b0>a)
      b0 -= 2.0 * M_PI;
    while(b1>a)
      b1 -= 2.0 * M_PI;

    // we might require the segment before the current back()...
    cout << "collecting possible directions" << endl;
    direction.push_back(direction_t(b0, minp, minb, true));  // forward
    direction.push_back(direction_t(b1, minp, minb, false)); // backward
    
  }
cout << "floodfill exceeded loop maximum" << endl;
  return 0;
}
