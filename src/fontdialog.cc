
// Q: Webdings results in a complete different (Andale Mono IPA) font under
//    Freetype
// A: This is a bug in the freetype library. Update to a newer version.

#warning "should separate Xft and X11 fonts"
#warning "should create font on-demand during drawString!!!"
#warning "the true type renderer is better, prefer arial over helvetica"

/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for X-Windows
 * Copyright (C) 1996-2003 by Mark-AndrÃ© Hopf <mhopf@mark13.org>
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

#ifdef _XFT_NO_COMPAT_
#undef _XFT_NO_COMPAT_
#endif

#include <X11/Xft/Xft.h>
#ifdef FC_VERSION
#define HAVE_FONTCONFIG
#endif

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
//   (the later is faster but seems to fail for 180°, which is also
//   true for ordinary X11(R6) rotated fonts while the first seems to fail
//   around 90° and 270°)

// antialiased fonts
// - xterm -fa 'Andale Mono' -fs 14
// - env QT_XFT=true konqueror

// http://fontconfig.org/
// /etc/fonts/*

// cd /usr/X11R6/lib/X11/fonts/TrueType ; fc-cache
// man font-config
// man fontconfig
// man xft

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

class TFont2
{
  public:
    enum ESizeType {
      SIZE_POINT,
      SIZE_PIXEL
    };
    enum ERenderType {
      RENDER_X11,
      RENDER_FREETYPE
    };
//  private:
    XFontStruct * x11fs;
    Font x11font; // only used for rotated fonts
    void createX11Font(TMatrix2D*);

#ifdef HAVE_LIBXFT    
    XftFont * xftfont;
    void createXftFont(TMatrix2D*);
#endif

    void clear();

    TFontFamily family;
    TFontStyle style;
    
    ESizeType sizetype;
    unsigned size;
    unsigned hdpi, vdpi;
    ERenderType rendertype;

  public:
    TFont2();
    ~TFont2();

    void setFamily(const TFontFamily &f) { family = f; clear(); }
    const TFontFamily& getFamily() const { return family; }

    void setStyle(const TFontStyle &s) { style = s; clear(); }
    const TFontStyle& getStyle() const { return style; }

    void setSizeType(ESizeType st) { sizetype = st; clear(); }
    ESizeType getSizeType() const { return sizetype; }

    void setSize(unsigned s) { size = s; clear(); }
    unsigned getSize() const { return size; }

    void setDPI(unsigned h, unsigned v) { hdpi=h; vdpi=v; clear(); }

    void setRenderType(ERenderType rt) { rendertype=rt; clear(); }
    ERenderType getRenderType() const { return rendertype; }
    
    void createFont(TMatrix2D*);
    
    Font getX11Font() const {
      if (x11font)
        return x11font;
      if (x11fs)
        return x11fs->fid;
      return 0;
    }
    
#ifdef HAVE_LIBXFT
    XftFont * getXftFont() const {
      return xftfont;
    }
#endif    

    int getTextWidth(const string&) const;
    int getTextWidth(const char*) const;
    int getTextWidth(const char*,int len) const;
    int getTextWidth(const unsigned char *s) const { return getTextWidth((const char*)s); }
    int getTextWidth(const unsigned char *s,int len) const { return getTextWidth((const char*)s,len); }
    int getAscent() const;
    int getDescent() const;
    int getHeight() const {return getAscent()+getDescent();}
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
    
    void showFontTable();
    
    void paint();

    TTable *tfont;
    TTable *tstyle;
    TTable *tsize;
    
    TFontFamilyMap ff;
    TFontStyleMap fs;

    GRadioStateModel<TFont2::ESizeType> sizetype;

    TTextModel size;
    TTextModel dpi;

    GRadioStateModel<TFont2::ERenderType> rendertype;
    
    TFont2 font;
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
  
//  registry = "iso10646";
//  encoding = "1";
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
  sizetype = TFont2::SIZE_POINT;
  rendertype = TFont2::RENDER_X11;

  updateRendertype();

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
  
  sizetype.add(new TRadioButton(this, "metric pt"), TFont2::SIZE_POINT);
  sizetype.add(new TRadioButton(this, "metric px"), TFont2::SIZE_PIXEL);
  connect(sizetype.sigChanged, this, &This::updateFont);

  rendertype.add(new TRadioButton(this, "type x11"), TFont2::RENDER_X11);
  rendertype.add(new TRadioButton(this, "type freetype"), TFont2::RENDER_FREETYPE);

  pb = new TPushButton(this, "table");
  connect(pb->sigActivate, this, &This::showFontTable);  
  pb = new TPushButton(this, "ok");
  pb = new TPushButton(this, "abort");

  #warning "why don't we just add a style flag to TTable that the first entry in a new model shall be selected"
  tfont->getSelectionModel()->setSelection(0, 0);

  loadLayout("TFontDialog.atv");
}

void
TFontDialog::familySelected()
{
  if (ff.empty()) { // ff.begin()==ff.end()) {
    cerr << "no font families" << endl;
    return;
  }
  const TFontFamily &f(ff.getElementAt(0, tfont->getSelectionModel()->begin().getY()));
  
  // we don't want a copy here!! reference or iterator would be fine!!!
  cerr << "selected family '" << f.family << "'" << endl;
  
  TFontFamily &f2(const_cast<TFontFamily&>(f));

//  tsize->setRenderer(new TTableCellRenderer_StringSet(&f2.size_pt));
  
  updateRendertype();
  updateStyle();
  
//  tsize->getSelectionModel()->setSelection(0, 0);
}

void
TFontDialog::updateRendertype()
{
  ff.erase(ff.begin(), ff.end());

  /*
   * retrieve Xft (X FreeType Extension) font names
   * We do this before retrieving the names from the X11 server as the
   * names delivered by FreeType have correct upper and lower case names
   * and we will adjust the X11 names to these ones.
   */
#ifdef HAVE_LIBXFT
  if (rendertype==TFont2::RENDER_FREETYPE) {
    cerr << "XFT - X FreeType interface library" << endl;
    // calling this function causes XftFontOpen to fail, but why?
    if (XftDefaultHasRender(x11display)) {
      cerr << "XFT supported by server" << endl;
    } else {
      cerr << "XFT isn't supported by server" << endl;
    }
#ifdef HAVE_FONTCONFIG
    XftFontSet * xftfl;
#else
    FcFontSet * xftfl;
#endif
    xftfl = XftListFonts(x11display, x11screen,
                         0, // XftPattern
                         XFT_FAMILY, 0);
    if (xftfl) {
      // XftFontSetPrint(xftfl);
      cerr << "nfont " << xftfl->nfont << endl;
      for(int i=0; i<xftfl->nfont; ++i) {
        string family;
#ifdef HAVE_FONTCONFIG
        FcPattern *pt = xftfl->fonts[i];
        FcChar8 *s;

        FcPatternGetString(pt, XFT_FAMILY, 0, &s);
        cerr << "found family " << s << endl;
        family = (char*)s;
      
        FcPatternGetString(pt, XFT_FOUNDRY, 0, &s);
        cerr << "  foundry " << s << endl;
#else
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
#endif
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
  }
  
  /*
   * retrieve X11 server font names
   */
  if (rendertype==TFont2::RENDER_X11) {
    int count;
    TX11FontName xfn;
    xfn.setWildcards();
    char **fl = XListFonts(x11display, xfn.getXLFD().c_str(), 8192, &count);
  
    for(int i=0; i<count; ++i) {
      TX11FontName xfn;
      xfn.setXLFD(fl[i]);
    
if (xfn.family=="fixed") {
  cerr << "fixed vendor '" << xfn.vendor << "' -> '" << xfn.registry << "-" << xfn.encoding << endl;
}

      /* hack to provide better sorting for humans: */
      if (!xfn.family.empty())
        xfn.family[0] = toupper(xfn.family[0]);
      if (!xfn.vendor.empty())
        xfn.vendor[0] = toupper(xfn.vendor[0]);

      TFontFamilyMap::iterator ffsp;
    
      ffsp = ff.begin();
      while(ffsp!=ff.end()) {
        if (strcasecmp(xfn.family.c_str(), ffsp->second.family.c_str())==0)
        {
          xfn.family = ffsp->second.family; // use FreeType name
          
          if (!ffsp->second.vendor.empty()) { // not a pure freetype name
            if (strcasecmp(xfn.vendor.c_str(), ffsp->second.vendor.c_str())!=0) {
              ffsp=ff.end();
            }
          }
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
  }

#if 0
  const TFontFamily &f(ff.getElementAt(0, tfont->getSelectionModel()->begin().getY()));

  rendertype.getButton(TFont2::RENDER_BITMAP)->setEnabled(f.type_bitmap);
  rendertype.getButton(TFont2::RENDER_SCALEABLE)->setEnabled(f.type_scaleable_x11);
  rendertype.getButton(TFont2::RENDER_FREETYPE)->setEnabled(f.type_scaleable_ft);

  bool change = false;

  switch(rendertype) {
    case TFont2::RENDER_BITMAP:
      if (!f.type_bitmap) change = true;
      break;
    case TFont2::RENDER_SCALEABLE:
      if (!f.type_scaleable_x11) change = true;
      break;
    case TFont2::RENDER_FREETYPE:
      if (!f.type_scaleable_ft) change = true;
      break;
  }

  if (change) {
    if (f.type_bitmap)
      rendertype = TFont2::RENDER_BITMAP;
    else if (f.type_scaleable_x11)
      rendertype = TFont2::RENDER_SCALEABLE;
    else
      rendertype = TFont2::RENDER_FREETYPE;
  }
#endif
}

void
TFontDialog::updateStyle()
{
  if (tfont->getSelectionModel()->begin() ==
      tfont->getSelectionModel()->end() ) 
  {
    cerr << "no font selected" << endl;
    return;
  }

  const TFontFamily &f(ff.getElementAt(0, tfont->getSelectionModel()->begin().getY()));

  if (rendertype!=TFont2::RENDER_FREETYPE) {
    TX11FontName xfn;
    xfn.setWildcards();
    xfn.set(f);
    if (sizetype==TFont2::SIZE_PIXEL) {
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
    
#ifdef HAVE_FONTCONFIG
    static FcConfig *fc = 0;
    if (fc==0)
      fc = FcInitLoadConfigAndFonts();
    FcFontSet *xftfl = FcFontList(fc, pattern, os);
#else
    XftFontSet *xftfl = XftListFontsPatternObjects(
      x11display, x11screen,
      pattern, os);
#endif
    fs.lock();
    fs.clear();
    for(int i=0; i<xftfl->nfont; ++i) {
      XftPattern *pt = xftfl->fonts[i];
      TFontStyle thisstyle("", "", "", "");
#ifdef HAVE_FONTCONFIG
      FcChar8 *s;
      for(int j=0; FcPatternGetString(pt, XFT_STYLE, j, &s)==FcResultMatch; ++j) {
        cerr << "found style " << s << endl;
        thisstyle.name = (char*)s;
        fs[thisstyle]=thisstyle;
      }
#else
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
#endif
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
  if (ff.empty() || fs.empty())
    return;

  font.setFamily(ff.getElementAt(0, tfont->getSelectionModel()->begin().getY()));
  font.setStyle(fs.getElementAt(0, tstyle->getSelectionModel()->begin().getY()));
  font.setSizeType(sizetype);
  font.setSize(atoi(size.c_str()));
  font.setDPI( atoi(dpi.c_str()), atoi(dpi.c_str()) );
  font.setRenderType(rendertype);
#if 0
  const TFontFamily &f(
    ff.getElementAt(0, tfont->getSelectionModel()->begin().getY())
  );
  TFontFamily &f2(const_cast<TFontFamily&>(f));

  if (fs.begin() == fs.end()) {
    cerr << __FILE__ << ':' << __LINE__ << endl;
    cerr << "  font style list is empty" << endl;
  }
  
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

TFont2::TFont2()
{
  x11font = 0;
  x11fs = 0;
  size = 12;
  hdpi = vdpi = 100;
#ifdef HAVE_LIBXFT
  xftfont = 0;
#endif
}

TFont2::~TFont2() {
  clear();
}

void
TFont2::clear()
{
  if (x11font) {
    XUnloadFont(x11display, x11font);
    x11font = 0;
  }
  if (x11fs) {
    XUnloadFont(x11display, x11fs->fid);
    XFreeFontInfo(NULL,x11fs,0);
    x11fs = 0;
  }
#ifdef HAVE_LIBXFT
  if (xftfont) {
    XftFontClose(x11display, xftfont);
    xftfont = 0;
  }
#endif
}

int
TFont2::getAscent() const
{
  if (x11fs)
    return x11fs->ascent;
#ifdef HAVE_LIBXFT
  if (xftfont)
    return xftfont->ascent;
#endif
  return 0;
}

int
TFont2::getDescent() const
{
  if (x11fs)
    return x11fs->ascent;
#ifdef HAVE_LIBXFT
  if (xftfont)
    return xftfont->ascent;
#endif
  return 0;
}

int
TFont2::getTextWidth(const string &str) const
{
  return getTextWidth(str.c_str(),str.size());
}
 
int
TFont2::getTextWidth(const char *str) const
{
  return getTextWidth(str,strlen(str));
}
 
int
TFont2::getTextWidth(const char *str, int len) const
{
  if (x11fs)
    return XTextWidth(x11fs,str,len);
#ifdef HAVE_LIBXFT
  if (xftfont) {
    XGlyphInfo gi;
    XftTextExtents8(x11display, xftfont, (XftChar8*)str, len, &gi);
    return gi.width;
  }
#endif
  return 0;
}

void
TFont2::createFont(TMatrix2D *mat)
{
  switch(rendertype) {
    case RENDER_X11:
      createX11Font(mat);
      break;
#ifdef HAVE_LIBXFT
    case RENDER_FREETYPE:
      createXftFont(mat);
      break;
#endif
  }
}
              
/*
 * X11 provides 2d transformations for text
 *
 * + : plus
 * ~ : minus
 *                  / a b 0 \
 *  [ a b c d ] <-> | c d 0 |
 *                  \ 0 0 1 /
 */
static string
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

void
TFont2::createX11Font(TMatrix2D *mat)
{
  if (x11fs)
    return;
    
cerr << "allocate new X11Font" << endl;

  // build an XLFD mask
  TX11FontName xfn;
  xfn.setWildcards();
  xfn.set(family);
  xfn.set(style);

  if (sizetype==SIZE_PIXEL) {
    char buffer[10];
    snprintf(buffer, sizeof(buffer), "%i", size);
cerr << size << "px" << endl;
    xfn.pixels = buffer;
  } else {
    char buffer[11];
    snprintf(buffer, sizeof(buffer), "%i", size * 10);
cerr << size << "pt, hdpi=" << hdpi <<", vdpi=" << vdpi << endl;
    xfn.points = buffer;
    
    snprintf(buffer, sizeof(buffer), "%i", hdpi);
    xfn.hdpi = buffer;
    snprintf(buffer, sizeof(buffer), "%i", vdpi);
    xfn.vdpi = buffer;
  }

  // locate fonts matching this mask
  cerr << "createX11Font: fontmask: " << xfn.getXLFD() << endl;

  int count;
  char **fl;
    
  for(int i=0; i<=20; ++i) {
    fl = XListFonts(x11display, xfn.getXLFD().c_str(), 8192, &count);
    for(int j=0; j<count; ++j) {
      cerr << "found " << fl[j] << endl;
    }
  
    if (count==0) {
      cerr << "no font found for " << xfn.getXLFD() << endl;
      switch(i) {
        case 0:
          xfn.vendor = "*";
          break;
        case 1:
          xfn.adj_style = "*";
          break;
        case 2:
          xfn.set_width = "*";
          break;
        case 3:
          xfn.slant = "*";
          break;
        case 4:
          xfn.weight = "*";
          break;
        case 5:
          xfn.family = "fixed";
          break;
        case 6:
          xfn.family = "*";
          break;
        case 7:
          cerr << "stopped searching for a font" << endl;
          return;
      }
    }
  }

  // complete missing values
  TX11FontName xfn_new;
  xfn_new.setXLFD(fl[0]);
/* // leave a wildcard
  if (sizetype==SIZE_PIXEL) {
    xfn.points = xfn_new.points;
  } else {
    xfn.pixels = xfn_new.pixels;
  }
*/
  xfn.spacing   = xfn.spacing;
  xfn.avg_width = xfn.avg_width;
  xfn.registry  = xfn_new.registry;
  xfn.encoding  = xfn_new.encoding;
  
  XFontStruct *new_fs = XLoadQueryFont(x11display, xfn.getXLFD().c_str());
  if (!new_fs) {
    cerr << "failed to load font structure" << endl;
    return;
  }
  XFreeFontNames(fl);
  
  Font new_font = 0;
  if (mat) {
    if (sizetype==SIZE_PIXEL) {
      double d = size;
      string s;
      s = "[";
      s += d2s(mat->a11 * d);
      s += d2s(mat->a12 * d);
      s += d2s(mat->a21 * d);
      s += d2s(mat->a22 * d);
      s += "]";
      xfn.pixels = s;
    } else {
      double d = size;
      string s;
      s = "[";
      s += d2s(mat->a11 * d);
      s += d2s(mat->a12 * d);
      s += d2s(mat->a21 * d);
      s += d2s(mat->a22 * d);
      s += "]";
      xfn.points = s;
    }
    new_font = XLoadFont(x11display, xfn.getXLFD().c_str());
    if (!new_font) {
      cerr << "failed to load font" << endl;
      XUnloadFont(x11display, new_fs->fid);
      XFreeFontInfo(NULL,new_fs,0);
      return;
    }
  }
  
  // set new font
  clear();
  x11font = new_font;
  x11fs = new_fs;
}

#ifdef HAVE_LIBXFT
void
TFont2::createXftFont(TMatrix2D *mat)
{
  if (xftfont)
    return;
cerr << "allocate new XftFont" << endl;
  XftPattern *pattern = XftPatternCreate();

  XftPatternAddString(pattern, XFT_FAMILY, family.family.c_str());

  XftMatrix xftmat;
  if (mat) {
    XftMatrixInit(&xftmat);
    xftmat.xx = mat->a11;
    xftmat.yx = mat->a12;
    xftmat.xy = mat->a21;
    xftmat.yy = mat->a22;
    XftPatternAddMatrix(pattern, XFT_MATRIX, &xftmat);
  }
    
  if (sizetype==TFont2::SIZE_PIXEL) {
    double s = size;
    XftPatternAddDouble(pattern, XFT_PIXEL_SIZE, s);
  } else {
    double s = size;
    double d = hdpi;
    XftPatternAddDouble(pattern, XFT_SIZE, s);
    XftPatternAddDouble(pattern, XFT_DPI, d);
  }

  XftPatternAddString(pattern, XFT_STYLE, style.name.c_str());

  XftResult result;
  XftPattern *pattern2 = XftFontMatch(x11display, x11screen,
    pattern, &result);
    
  XftFont *new_font = XftFontOpenPattern(x11display, pattern2);
  if (new_font) {
    clear();
    xftfont = new_font;
  }
  #warning "xft structures not destroyed"
}
#endif

void
drawString(TPen &pen,
           TFont2 &font,
           XftDraw *xftdraw,
           int x, int y, const string &text)
{
  font.createFont(pen.mat);

  switch(font.getRenderType()) {
    case TFont2::RENDER_X11:
      if (!font.getX11Font())
        return;
      XSetFont(x11display, pen.o_gc, font.getX11Font());
      y+=font.getAscent();
      if (!pen.mat) {
        XDrawString(x11display, pen.x11drawable, pen.o_gc, x,y,
          text.c_str(), text.size() );
      } else {
        int x2, y2;
        string::const_iterator p(text.begin()), e(text.end());
        while(p!=e) {
          char buffer[2];
          buffer[0]=*p;
          buffer[1]=0;
          pen.mat->map(x, y, &x2, &y2);
          XDrawString(x11display, pen.x11drawable, pen.o_gc, x2,y2, buffer, 1);
          x+=font.getTextWidth(buffer);
          ++p;
        }
      }
      break;
    case TFont2::RENDER_FREETYPE: {
      y+=font.getAscent();
      if (pen.mat)
        pen.mat->map(x, y, &x, &y);
      XftColor color;
      color.color.red = 0;
      color.color.green = 0;
      color.color.blue = 0;
      color.color.alpha = 0xffff;
      XftDrawString8(xftdraw, &color, font.getXftFont(), x,y, (XftChar8*)text.c_str(), text.size());
      } break;
  }
}

void
drawString16(TPen &pen,
             TFont2 &font,
             XftDraw *xftdraw,
             int x, int y,
             XChar2b *text,
             unsigned len )
{
  font.createFont(pen.mat);

  switch(font.getRenderType()) {
    case TFont2::RENDER_X11:
      if (!font.getX11Font())
        return;
      XSetFont(x11display, pen.o_gc, font.getX11Font());
      y+=font.getAscent();
      if (!pen.mat) {
        XDrawString16(x11display, pen.x11drawable, pen.o_gc, x,y, text, len );
cerr << __LINE__ << endl;
      } else {
#if 1
        pen.mat->map(x, y, &x, &y);
        XDrawString16(x11display, pen.x11drawable, pen.o_gc, x,y, text, len );
#else
        int x2, y2;
        string::const_iterator p(text.begin()), e(text.end());
        while(p!=e) {
          XChar2b buffer[2];
          buffer[0]=*p;
          buffer[1]=0;
          pen.mat->map(x, y, &x2, &y2);
          XDrawString16(x11display, pen.x11drawable, pen.o_gc, x2,y2, buffer, 1);
          x+=font.getTextWidth(buffer);
          ++p;
        }
#endif
      }
      break;
    case TFont2::RENDER_FREETYPE: {
      y+=font.getAscent();
      if (pen.mat)
        pen.mat->map(x, y, &x, &y);
      XftColor color;
      color.color.red = 0;
      color.color.green = 0;
      color.color.blue = 0;
      color.color.alpha = 0xffff;
for(int i=0; i<len; ++i) {
  int a = text[i].byte1;
  text[i].byte1 = text[i].byte2;
  text[i].byte2 = a;
}
      XftDrawString16(xftdraw, &color, font.getXftFont(), x,y, (XftChar16*)text, len);
      } break;
  }
}

void
TFontDialog::paint()
{
  TPen pen(this);
  pen.translate(8, 160);
//  pen.rotate(15);

  XftDraw *xftdraw = XftDrawCreate(x11display, pen.x11drawable, x11visual, x11colormap);
  XftDrawSetClip(xftdraw, wnd->getUpdateRegion()->x11region);

  drawString(pen, font, xftdraw, 0, 0, "abcxyz ABCXYZ 123 `'´!?${}. áäæçëß ÂÄÆÇË");

  XftDrawDestroy(xftdraw);
}

class TFontTable:
  public TDialog
{
    TFont2 &font;
  public:
    TFontTable(TWindow *parent, const string &title, TFont2 &font);
    void paint();
    
    void table(int dx);
    int ct;
};

TFontTable::TFontTable(TWindow *parent, const string &title, TFont2 &aFont):
  TDialog(parent, title), font(aFont)
{
  setLayout(0);
  setSize(16*24+10, 16*24+30+10);
  
  TPushButton *pb;
  
  pb = new TPushButton(this, "<");
  connect(pb->sigActivate, this, &TFontTable::table, -1);
  pb->setShape(5,5,80,25);

  pb = new TPushButton(this, ">");
  connect(pb->sigActivate, this, &TFontTable::table, 1);
  pb->setShape(5+80+5,5,80,25);
  
  ct = 0;
  table(0);
}

#ifdef X_HAVE_UTF8_STRING
#endif

void
TFontTable::table(int dx)
{
  ct+=dx;
  if (ct<0)
    ct=0;
  if (ct>255)
    ct=255;
  invalidateWindow();
}

void
TFontTable::paint()
{
  TPen pen(this);
  
  unsigned i, j, x, y, c;
  int s = 24;

cerr << "table: " << ct << endl;
if (font.x11fs) {
  cerr << "  first char: " << font.x11fs->min_char_or_byte2 << endl;
  cerr << "  last  char: " << font.x11fs->max_char_or_byte2 << endl;
  cerr << "  first row : " << font.x11fs->min_byte1 << endl;
  cerr << "  last row  : " << font.x11fs->max_byte1 << endl;
  cerr << "  direction : " << font.x11fs->direction << endl;
  cerr << "  all_chars_exist: " << font.x11fs->all_chars_exist << endl;
  cerr << "  default char   : " << font.x11fs->default_char << endl;
}
  pen.translate(5,35);
  
  x = y = 0;
  c = 0;
  
  XftDraw *xftdraw = XftDrawCreate(x11display, pen.x11drawable, x11visual, x11colormap);
  XftDrawSetClip(xftdraw, getUpdateRegion()->x11region);

  pen.setColor(226, 145, 145);
  for(i=0; i<16; ++i) {
    for(j=0; j<16; ++j) {
      unsigned word = (ct<<8) + c;
      if (word<=0x007f) {
        pen.fillRectangle(x, y, s, s);
      }
      x+=s;
      c++;
    }
    x=0;
    y+=s;
  }
  pen.setColor(0, 0, 0);
  
  x = y = 0;
  c = 0;
  for(i=0; i<16; ++i) {
    pen.drawLine(i*s,0,i*s,s*16);
    pen.drawLine(0,i*s,s*16,i*s);
    for(j=0; j<16; ++j) {
#if 0
      char buffer[2];
      buffer[0]=c;
      buffer[1]=0;
      drawString(pen, font, xftdraw, x+2, y+2, buffer);
#endif
#if 1
      XChar2b buffer[2];
      buffer[0].byte1 = ct;
      buffer[0].byte2 = c;
      drawString16(pen, font, xftdraw, x+2, y+2, buffer, 1);
#endif
      x+=s;
      c++;
    }
    x=0;
    y+=s;
  }
  pen.drawLine(i*s,0,i*s,s*16);
  pen.drawLine(0,i*s,s*16,i*s);


  XftDrawDestroy(xftdraw);
}

void
TFontDialog::showFontTable()
{
  cerr << "show font table" << endl;
  TFontTable dlg(this, "Font Table", font);
  dlg.doModalLoop();
}
