#warning "Webdings results in a complete different (Andale Mono IPA) font under Freetype"

/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for X-Windows
 * Copyright (C) 1996-2003 by Mark-André Hopf <mhopf@mark13.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#define _TOAD_PRIVATE

#include <toad/toad.hh>
#include <toad/dialog.hh>
#include <toad/pushbutton.hh>
#include <toad/checkbox.hh>
#include <toad/radiobutton.hh>
#include <toad/textfield.hh>
#include <toad/config.h>
#ifdef HAVE_LIBXFT
#include <X11/Xft/Xft.h>
#endif

#include <toad/tablemodels.hh>

#include <sstream>
#include <iomanip>

using namespace toad;

// TrueType font
// supported since XFree86 4.x
// - copy 'em to /usr/X11R6/lib/X11/fonts/TrueType/
// - run 'ttmkfdir > fonts.scale' inside this directory
// - add the module "xtt" or "freetype" to your XF86Config file
//   (the later is faster but seems to fail for 180°, while the
//   first seems to fail around 90° and 270°)

// antialiased fonts
// - xterm -fa 'Andale Mono' -fs 14
// - env QT_XFT=true konqueror

// http://fontconfig.org/

/*
 * X11 provides 2d transformations for text
 *
 * + : plus
 * ~ : minus
 *                  / a b 0 \
 *  [ a b c d ] <-> | c d 0 |
 *                  \ 0 0 1 /
 */
string
d2s(double d)
{
  string s;
  
  if (d<0.0) {
    s+='~';
    d=-d;
  } else {
    s+='+';
  }
  
  double a = 1.0;
  while(a<=d) a*=10.0;

  bool b;

  a/=10.0;

  if (a>=1.0) {
    while(a>=1.0) {
      int digit=0; 
      while(d>=a) {
        digit++;
        d-=a;
      }
      a/=10.0;
      s+='0'+digit;
    }
  } else {
    s+='0';
  }
  
  if (d>=0.0001) {
    s+='.';
    while(d>=0.0001) {
      int digit=0; 
      while(d>=a) {
        digit++;
        d-=a;
      }
      a/=10.0;
      s+='0'+digit;
    }
  }

  return s;
}

class TFontStyle {
  public:
    TFontStyle() {}
    TFontStyle(const TFontStyle&) {}
  
    TFontStyle::TFontStyle(const string &weight,
                           const string &slant,
                           const string &set_width,
                           const string &adj_style);

    // operator to allow sorting in STL container
    bool operator<(const TFontStyle &f) const {
      return name < f.name;
    }

    // operator to allow display in TTable renderer
    operator const string&() const {
      return name;
    }

    string name;    
    string weight, slant, set_width, adj_style;
};
    
TFontStyle::TFontStyle(const string &weight,
                       const string &slant,
                       const string &set_width,
                       const string &adj_style)
{
  this->weight    = weight;
  this->slant     = slant;
  this->set_width = set_width;
  this->adj_style = adj_style;

  name=weight+" ";
  switch(slant[0]) {
    case 'r':
      break;
    case 'i':
      name+="italics ";
      break;
    case 'o':
      name+="oblique ";
      break;
    default:
      name+=slant+" ";
  }
  if (set_width!="normal") {
    name+=set_width+" ";
  }
  name+=adj_style;
}

typedef GSTLMap<map<string, TFontStyle>, string, TFontStyle> TFontStyleMap;
typedef GTableCellRenderer_String<TFontStyleMap> TTableCellRenderer_FontStyleMap;

class TFontFamily {
  public:
    TFontFamily();
    TFontFamily(TFontFamily const&);
    TFontFamily(const string &vendor, const string &family);

    // operator to allow sorting in STL container
    bool operator<(const TFontFamily &f) const {
      return name < f.name;
    }

    // operator to allow display in TTable renderer
    operator const string&() const {
      return name;
    }
    
    string name;
    
    string vendor;
    string family;
    
    TStringSet bitmap_size;
    
    bool type_bitmap;
    bool type_scaleable_x11;
    bool type_scaleable_ft;
    
    TStringSet size_px;
    TStringSet size_pt;
};

typedef GSTLMap<map<string, TFontFamily>, string, TFontFamily> TFontFamilyMap;
typedef GTableCellRenderer_String<TFontFamilyMap> TTableCellRenderer_FontFamilyMap;

class TFontDialog:
  public TDialog
{
    typedef TFontDialog This;
  public:
    TFontDialog(TWindow *p, const string &t);

    void familySelected();
    void styleSelected();
    
    void updateStyle();
    void updateRendertype();
    void updateFont();
    
    void paint();

    TTable *tfont;
    TTable *tstyle;
    TTable *tsize;
    
    TFontFamilyMap ff;
    TFontStyleMap fs;

    enum ESizeType {
      SIZE_POINT,
      SIZE_PIXEL
    };
    
    GRadioStateModel<ESizeType> sizetype;

    TTextModel size;
    TTextModel dpi;

    enum ERenderType {
      RENDER_BITMAP,
      RENDER_SCALEABLE,
      RENDER_FREETYPE
    };
    
    GRadioStateModel<ERenderType> rendertype;
};

int
main(int argc, char **argv, char **envv)
{
  toad::initialize(argc, argv, envv); {
    TFontDialog wnd(NULL, "TFontDialog");
    toad::mainLoop();
  } toad::terminate();
}


TFontFamily::TFontFamily()
{
  type_bitmap = type_scaleable_x11 = type_scaleable_ft = false;
}

TFontFamily::TFontFamily(TFontFamily const &f)
{
  type_bitmap = type_scaleable_x11 = type_scaleable_ft = false;
}

TFontFamily::TFontFamily(const string &vendor, const string &family)
{
  type_bitmap = type_scaleable_x11 = type_scaleable_ft = false;
  this->vendor = vendor;
  this->family = family;
  name = family;
  if (!vendor.empty() && vendor!="*")
    name += " (" + vendor + ")";
}

// X Logical Font Description
struct TX11FontName {
  
  string vendor, family, weight, slant, set_width, adj_style,
         pixels, points, hdpi, vdpi, spacing, avg_width,
         registry, encoding;
  
  void setWildcards();
  void set(const TFontFamily &ff);
  void set(const TFontStyle &fs);
  
  void setXLFD(const char *name);
  string getXLFD() const;

  bool isScaleable() const;

};

void
TX11FontName::setWildcards()
{
  vendor = family = weight = slant = set_width = adj_style =
  pixels = points = hdpi = vdpi = spacing = avg_width =
  registry = encoding = "*";
}

void
TX11FontName::set(const TFontFamily &ff)
{
  vendor = ff.vendor;
  family = ff.family;
}

void
TX11FontName::set(const TFontStyle &fs)
{
  weight = fs.weight;
  slant = fs.slant;
  set_width = fs.set_width;
  adj_style = fs.adj_style;
}

void
TX11FontName::setXLFD(const char *name)
{
  int l, r;
    
  l = 1;
  for(r=l; name[r]!=0 && name[r]!='-'; ++r);
  vendor.assign(name+l, r-l);
    
  l=r+1;
  for(r=l; name[r]!=0 && name[r]!='-'; ++r);
  family.assign(name+l, r-l);

  l=r+1;
  for(r=l; name[r]!=0 && name[r]!='-'; ++r);
  weight.assign(name+l, r-l);

  l=r+1;
  for(r=l; name[r]!=0 && name[r]!='-'; ++r);
  slant.assign(name+l, r-l);

  l=r+1;
  for(r=l; name[r]!=0 && name[r]!='-'; ++r);
  set_width.assign(name+l, r-l);

  l=r+1;
  for(r=l; name[r]!=0 && name[r]!='-'; ++r);
  adj_style.assign(name+l, r-l);

  l=r+1;
  for(r=l; name[r]!=0 && name[r]!='-'; ++r);
  pixels.assign(name+l, r-l);

  l=r+1;
  for(r=l; name[r]!=0 && name[r]!='-'; ++r);
  points.assign(name+l, r-l);

  l=r+1;
  for(r=l; name[r]!=0 && name[r]!='-'; ++r);
  hdpi.assign(name+l, r-l);

  l=r+1;
  for(r=l; name[r]!=0 && name[r]!='-'; ++r);
  vdpi.assign(name+l, r-l);

  l=r+1;
  for(r=l; name[r]!=0 && name[r]!='-'; ++r);
  spacing.assign(name+l, r-l);

  l=r+1;
  for(r=l; name[r]!=0 && name[r]!='-'; ++r);
  avg_width.assign(name+l, r-l);

  l=r+1;
  for(r=l; name[r]!=0 && name[r]!='-'; ++r);
  registry.assign(name+l, r-l);

  l=r+1;
  for(r=l; name[r]!=0 && name[r]!='-'; ++r);
  encoding.assign(name+l, r-l);
}

string
TX11FontName::getXLFD() const
{
  string name;
  name = "-" + vendor +
         "-" + family +
         "-" + weight +
         "-" + slant + 
         "-" + set_width + 
         "-" + adj_style +
         "-" + pixels +
         "-" + points +
         "-" + hdpi +
         "-" + vdpi +
         "-" + spacing +
         "-" + avg_width +
         "-" + registry +
         "-" + encoding;
  return name;
}

bool
TX11FontName::isScaleable() const
{
   return (pixels=="0" &&
           points=="0" && 
           hdpi=="0" && 
           vdpi=="0" &&
           avg_width=="0");
}

TFontDialog::TFontDialog(TWindow *parent, const string &title):
  TDialog(parent, title)
{
  /*
   * set a few default values
   */
  size = "12";
  dpi = "100";
  sizetype = SIZE_POINT;
  rendertype = RENDER_BITMAP;

  /*
   * retrieve Xft (X FreeType Extension) font names
   * We do this before retrieving the names from the X11 server as the
   * names delivered by FreeType have correct upper and lower case names
   * and we will adjust the X11 names to these ones.
   */
#ifdef HAVE_LIBXFT
  cerr << "XFT - X FreeType interface library" << endl;
  // calling this function causes XftFontOpen to fail, but why?
  if (XftDefaultHasRender(x11display)) {
    cerr << "XFT supported by server" << endl;
  } else {
    cerr << "XFT isn't supported by server" << endl;
  }

#if 1
  XftFontSet * xftfl;
  xftfl = XftListFonts(x11display, x11screen,
                       0, // XftPattern
                       XFT_FAMILY, 0);
  if (xftfl) {
    // XftFontSetPrint(xftfl);
    cerr << "nfont " << xftfl->nfont << endl;
    for(int i=0; i<xftfl->nfont; ++i) {
      string family;

      XftPattern *pt = xftfl->fonts[i];
      for(int j=0; j<pt->num; ++j) {
        XftPatternElt *e = pt->elts+j;
        if (strcmp(e->object, XFT_FAMILY)==0) {
          XftValueList *vl = e->values;
          while(vl) {
            switch(vl->value.type) {  
              case XftTypeString:
                family = vl->value.u.s;
//                cerr << "found '" << vl->value.u.s << "'\n";
                break;
//              deault:
//                cerr << "(unhandled type)" << endl;
            }
            vl=vl->next;
          }
        }
      }
      TFontFamily thisfamily("*", family);
      thisfamily.type_scaleable_ft=true;
      TFontFamilyMap::iterator ffsp = ff.find(thisfamily);
      if (ffsp==ff.end()) {
        ff[thisfamily]=thisfamily;
        ffsp = ff.find(thisfamily);
      }
    }
  } else {
    cerr << "XftListFonts failed" << endl;
  }
  XftFontSetDestroy(xftfl);
#endif
#endif
  
  /*
   * retrieve X11 server font names
   */
#if 1
  int count;
  char **fl = XListFonts(x11display, "-*-*-*-*-*-*-*-*-*-*-*-*-*-*", 8192, &count);
//  char **fl = XListFonts(x11display, "-*-times new roman-*-*-*-*-*-*-*-*-*-*-*-*", 8192, &count);

  for(int i=0; i<count; ++i) {
    TX11FontName xfn;
    xfn.setXLFD(fl[i]);
    
    /* hack to provide better sorting for humans: */
    if (!xfn.family.empty())
      xfn.family[0] = toupper(xfn.family[0]);
    if (!xfn.vendor.empty())
      xfn.vendor[0] = toupper(xfn.vendor[0]);

    TFontFamilyMap::iterator ffsp;
    
    ffsp = ff.begin();
    while(ffsp!=ff.end()) {
      if (strcasecmp(xfn.family.c_str(), ffsp->second.family.c_str())==0) {
        xfn.family = ffsp->second.family;
        break;
      }
      ++ffsp;
    }
    TFontFamily thisfamily(xfn.vendor, xfn.family);
    
//    ffsp = ff.find(index);
    if (ffsp==ff.end()) {
      ff[thisfamily]=thisfamily;
      ffsp = ff.find(thisfamily);
    } else {
      ffsp->second.vendor = xfn.vendor;
      ffsp->second.name   = xfn.family;
    }

    if (xfn.isScaleable()) {
      ffsp->second.type_scaleable_x11 = true;
    } else {
      ffsp->second.type_bitmap = true;
      if (xfn.points!="0")
        ffsp->second.size_pt.insert(xfn.points);
      if (xfn.pixels!="0")
        ffsp->second.size_px.insert(xfn.pixels);
    }
  }

  XFreeFontNames(fl);
#endif  

  /*
   * create widgets
   */
  TPushButton *pb;
  TTextField *tf;
  
  tfont = new TTable(this, "font family");
  tfont->setRenderer(new TTableCellRenderer_FontFamilyMap(&ff));
  connect(tfont->sigSelection, this, &This::familySelected); 

  tstyle = new TTable(this, "font style");
  tstyle->setRenderer(new TTableCellRenderer_FontStyleMap(&fs));
  connect(tstyle->sigSelection, this, &This::updateFont);

  tf = new TTextField(this, "font size", &size);
//  connect(size.sigChanged, this, &This::updateFont);

  tf = new TTextField(this, "dpi", &dpi);

  tsize = new TTable(this, "font size table");
  
  sizetype.add(new TRadioButton(this, "metric pt"), SIZE_POINT);
  sizetype.add(new TRadioButton(this, "metric px"), SIZE_PIXEL);
  connect(sizetype.sigChanged, this, &This::updateFont);

  rendertype.add(new TRadioButton(this, "type bitmap"), RENDER_BITMAP);
  rendertype.add(new TRadioButton(this, "type scaleable x11"), RENDER_SCALEABLE);
  rendertype.add(new TRadioButton(this, "type scaleable ft"), RENDER_FREETYPE);
  connect(rendertype.sigChanged, this, &This::updateStyle);
  
  pb = new TPushButton(this, "ok");
  pb = new TPushButton(this, "abort");

  #warning "why don't we just add a style flag to TTable that the first entry in a new model shall be selected"
  tfont->getSelectionModel()->setSelection(0, 0);

  loadLayout("TFontDialog.atv");
}

void
TFontDialog::familySelected()
{
  const TFontFamily &f(ff.getElementAt(0, tfont->getSelectionModel()->begin().getY()));
  
  // we don't want a copy here!! reference or iterator would be fine!!!
  cerr << "selected family '" << f.family << "'" << endl;
  
  TFontFamily &f2(const_cast<TFontFamily&>(f));

  tsize->setRenderer(new TTableCellRenderer_StringSet(&f2.size_pt));
  
  updateRendertype();
  updateStyle();
  
  tsize->getSelectionModel()->setSelection(0, 0);
}

void
TFontDialog::updateRendertype()
{
  const TFontFamily &f(ff.getElementAt(0, tfont->getSelectionModel()->begin().getY()));

  rendertype.getButton(RENDER_BITMAP)->setEnabled(f.type_bitmap);
  rendertype.getButton(RENDER_SCALEABLE)->setEnabled(f.type_scaleable_x11);
  rendertype.getButton(RENDER_FREETYPE)->setEnabled(f.type_scaleable_ft);

  bool change = false;

  switch(rendertype) {
    case RENDER_BITMAP:
      if (!f.type_bitmap) change = true;
      break;
    case RENDER_SCALEABLE:
      if (!f.type_scaleable_x11) change = true;
      break;
    case RENDER_FREETYPE:
      if (!f.type_scaleable_ft) change = true;
      break;
  }

  if (change) {
    if (f.type_bitmap)
      rendertype = RENDER_BITMAP;
    else if (f.type_scaleable_x11)
      rendertype = RENDER_SCALEABLE;
    else
      rendertype = RENDER_FREETYPE;
  }
}

void
TFontDialog::updateStyle()
{
  const TFontFamily &f(ff.getElementAt(0, tfont->getSelectionModel()->begin().getY()));

  if (rendertype!=RENDER_FREETYPE) {
    TX11FontName xfn;
    xfn.setWildcards();
    xfn.set(f);
    if (sizetype==SIZE_PIXEL) {
      xfn.pixels = size;
    } else {
      xfn.points = size + "0";
      xfn.hdpi = xfn.vdpi = dpi;
    }
  
//    cerr << "mask: '" << xfn.getXLFD() << "'\n";
    int count;
    char **fl = XListFonts(x11display, xfn.getXLFD().c_str(), 8192, &count);
    fs.lock();
    fs.clear();
    for(int i=0; i<count; ++i) {
      // cerr << fl[i] << endl;
      
      TX11FontName xfn;
      xfn.setXLFD(fl[i]);
      TFontStyle thisstyle(xfn.weight, xfn.slant, xfn.set_width, xfn.adj_style);
  
      if (!thisstyle.name.empty()) {
        TFontStyleMap::iterator fsmp = fs.find(thisstyle);
        if (fsmp==fs.end()) {
          fs[thisstyle]=thisstyle;
          fsmp = fs.find(thisstyle);
        }
      }
    }
    XFreeFontNames(fl);
    fs.unlock();
  } else {
//    cerr << "getting freetype style's" << endl;
    
    XftPattern *pattern = XftPatternCreate();
    XftPatternAddString(pattern, XFT_FAMILY, f.family.c_str());
/*
    if (sizetype==SIZE_POINT) {
      double s = atoi(size.c_str());
      XftPatternAddDouble(pattern, XFT_PIXEL_SIZE, s);
    } else {
      double s = atoi(size.c_str());
      double d = atoi(dpi.c_str());
      XftPatternAddDouble(pattern, XFT_SIZE, s);
      XftPatternAddDouble(pattern, XFT_DPI, d);
    }
*/    
    XftObjectSet *os = XftObjectSetCreate();
    XftObjectSetAdd(os, XFT_STYLE);
    // XftObjectSetAdd(os, XFT_SLANT);
    // XftObjectSetAdd(os, XFT_WEIGHT);
    
    XftFontSet *xftfl = XftListFontsPatternObjects(
      x11display, x11screen,
      pattern, os);
      
//    XftFontSetPrint(xftfl);

    fs.lock();
    fs.clear();
    for(int i=0; i<xftfl->nfont; ++i) {
      XftPattern *pt = xftfl->fonts[i];
      TFontStyle thisstyle("", "", "", "");
      for(int j=0; j<pt->num; ++j) {
        XftPatternElt *e = pt->elts+j;
        if (strcmp(e->object, XFT_STYLE)==0) {
          XftValueList *vl = e->values;
          while(vl) {
            switch(vl->value.type) {  
              case XftTypeString:
                thisstyle.name = vl->value.u.s;
                fs[thisstyle]=thisstyle;
                break;
            }
            vl=vl->next;
          }
        }
      }
    }
    fs.unlock();

    XftObjectSetDestroy(os);
    XftFontSetDestroy(xftfl);
  }
  
  tstyle->getSelectionModel()->setSelection(0, 0);
  invalidateWindow();
}

void
TFontDialog::styleSelected()
{
  cerr << "font style selected" << endl;
  updateFont();
}

void
TFontDialog::updateFont()
{
  const TFontFamily &f(
    ff.getElementAt(0, tfont->getSelectionModel()->begin().getY())
  );
  TFontFamily &f2(const_cast<TFontFamily&>(f));
  
  const TFontStyle &s(
    fs.getElementAt(0, tstyle->getSelectionModel()->begin().getY())
  );
  
  TX11FontName xfn;
  xfn.setWildcards();
  xfn.set(f);
  xfn.set(s);
  if (sizetype==SIZE_PIXEL) {
    xfn.pixels = size;
  } else {
    xfn.points = size + "0";
    xfn.hdpi = xfn.vdpi = dpi;
  }
#if 0 
  #warning "encoding isn't selected" 
//  cerr << "mask: '" << xfn.getXLFD() << "'\n";
  int count;
  char **fl = XListFonts(x11display, xfn.getXLFD().c_str(), 8192, &count);
  for(int i=0; i<count; ++i) {
    if (i==0) {
      font.setFont(fl[0]);
    }
//    cerr << fl[i] << endl;
  }
  XFreeFontNames(fl);
#endif
  invalidateWindow();
}

void
TFontDialog::paint()
{
  TPen pen(this);
  pen.rotate(15);
  
  int x, y;
  x = 8; y=160;
  string text("abcxyz ABCXYZ 123 `'´!?${}. áäæçëß ÂÄÆÇË");

  const TFontFamily &f(ff.getElementAt(0, tfont->getSelectionModel()->begin().getY()));
  const TFontStyle &s(fs.getElementAt(0, tstyle->getSelectionModel()->begin().getY()));

  if (rendertype!=RENDER_FREETYPE) {
    TX11FontName xfn;
    xfn.setWildcards();
    xfn.set(f);
    xfn.set(s);
    if (sizetype==SIZE_PIXEL) {
      if(!pen.mat) {
        xfn.pixels = size;
      } else {
        double d = atof(size.c_str());
        string s;
        s = "[";
        s += d2s(pen.mat->a11 * d);
        s += d2s(pen.mat->a12 * d);
        s += d2s(pen.mat->a21 * d);
        s += d2s(pen.mat->a22 * d);
        s += "]";
        xfn.pixels = s;
      }
    } else {
      if (!pen.mat) {
        xfn.points = size + "0";
      } else {
        double d = atof(size.c_str());
        string s;
        s = "[";
        s += d2s(pen.mat->a11 * d);
        s += d2s(pen.mat->a12 * d);
        s += d2s(pen.mat->a21 * d);
        s += d2s(pen.mat->a22 * d);
        s += "]";
        xfn.points = s;
      }
      xfn.hdpi = xfn.vdpi = dpi;
    }

    cerr << "paint X11 fontmask: " << xfn.getXLFD() << endl;

    TFont font;
    int count;
    char **fl;
    
    fl = XListFonts(x11display, xfn.getXLFD().c_str(), 8192, &count);
//    cerr << "paint X11 fontmask: " << xfn.getXLFD() << endl;
    for(int i=0; i<count; ++i) {
        if (i==0)
          font.setFont(fl[i]);
//      cerr << fl[i] << endl;
    }
    XFreeFontNames(fl);
    pen.setFont(&font);
    
    if (!pen.mat) {
      pen.drawString(x, y, text);
    } else {
      if (sizetype==SIZE_PIXEL) {
        xfn.pixels = size;
      } else {
        xfn.points = size + "0";
      }
      TFont font_normal;
      fl = XListFonts(x11display, xfn.getXLFD().c_str(), 8192, &count);
//      cerr << "paint X11 fontmask: " << xfn.getXLFD() << endl;
      for(int i=0; i<count; ++i) {
          if (i==0)
            font_normal.setFont(fl[i]);
//        cerr << fl[i] << endl;
      }
      XFreeFontNames(fl);
      
      y-=font.getAscent();
      y+=font_normal.getAscent();

      string::iterator p(text.begin()), e(text.end());
      while(p!=e) {
        char buffer[2];
        buffer[0]=*p;
        buffer[1]=0;
        pen.drawString(x,y, buffer);
        x+=font_normal.getTextWidth(buffer);
        ++p;
      }
    }
  } else {
    cerr << "render freetype" << endl;

    XGlyphInfo gi;
    XftDraw *xftdraw = XftDrawCreate(x11display, x11window, x11visual, x11colormap);
    if (!xftdraw) {
      cerr << "failed to create xftdraw" << endl;
      return;
    }
    XftDrawSetClip(xftdraw, getUpdateRegion()->x11region);
    
    XftFont *font;
    XftPattern *pattern = XftPatternCreate();
    XftPatternAddString(pattern, XFT_FAMILY, f.family.c_str());

    XftMatrix xftmat;
    if (pen.mat) {
      XftMatrixInit(&xftmat);
      xftmat.xx = pen.mat->a11;
      xftmat.yx = pen.mat->a12;
      xftmat.xy = pen.mat->a21;
      xftmat.yy = pen.mat->a22;
      XftPatternAddMatrix(pattern, XFT_MATRIX, &xftmat);
    }
    
    if (sizetype==SIZE_PIXEL) {
      double s = atoi(size.c_str());
      XftPatternAddDouble(pattern, XFT_PIXEL_SIZE, s);
    } else {
      double s = atoi(size.c_str());
      double d = atoi(dpi.c_str());
      XftPatternAddDouble(pattern, XFT_SIZE, s);
      XftPatternAddDouble(pattern, XFT_DPI, d);
    }

    const TFontStyle &style(
      fs.getElementAt(0, tstyle->getSelectionModel()->begin().getY())
    );
    XftPatternAddString(pattern, XFT_STYLE, style.name.c_str());

    XftResult result;
    XftPattern *pattern2 = XftFontMatch(x11display, x11screen,
      pattern, &result);
    
    font = XftFontOpenPattern(x11display, pattern2);

    if (font) {
      XGlyphInfo gi;
      XftTextExtents8(x11display, font, (XftChar8*)text.c_str(), text.size(), &gi);
       y+=font->ascent;
      if (pen.mat)
        pen.mat->map(x, y, &x, &y);
      XftColor color;
      color.color.red = 0;
      color.color.green = 0;
      color.color.blue = 0;
      color.color.alpha = 0xffff;

      XftDrawString8(xftdraw, &color, font, x,y, (XftChar8*)text.c_str(), text.size());
      XftFontClose(x11display, font);
    } else {
      cerr << "no font!" << endl;
    }
  
//  XftColorFree(x11display, x11visual, x11colormap, &color);

  }
}
