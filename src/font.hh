/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2005 by Mark-André Hopf <mhopf@mark13.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, 
 * MA  02111-1307,  USA
 */

#ifndef __TOAD_FONT_HH
#define __TOAD_FONT_HH

#include <string>
#include <cstring>
#include <toad/pointer.hh>
#include <fontconfig/fontconfig.h>

namespace toad {

using namespace std;

class TPenBase;
class TFont;
class TMatrix2D;

class TFontManager
{
  public:
    virtual ~TFontManager();
    virtual void drawString(TPenBase *pen, int x, int y, const char *str, size_t len, bool transparent) = 0;
    virtual int getHeight(TFont *font) = 0;
    virtual int getAscent(TFont *font) = 0;
    virtual int getDescent(TFont *font) = 0;
    virtual int getTextWidth(TFont *font, const char *text, size_t n) = 0;

    int getTextWidth(TFont *font, const char *text) {
      return getTextWidth(font, text, strlen(text));
    }
    int getTextWidth(TFont *font, const string &text) {
      return getTextWidth(font, text.c_str(), text.size());
    }
    
    virtual void freeCoreFont(TFont*) = 0;
    
    virtual string getName() const = 0;
    virtual FcConfig* getFcConfig() = 0;    
    virtual FcFontSet* getFcFontSet() = 0;
    
    static bool setDefaultByName(const string &engine);
    static TFontManager* getDefault();
};

class TFontManagerX11:
  public TFontManager
{
  public:
    void init() const;
    void drawString(TPenBase *pen, int x, int y, const char *str, size_t len, bool transparent);
    int getHeight(TFont *font);
    int getAscent(TFont *font);
    int getDescent(TFont *font);
    int getTextWidth(TFont *font, const char *text, size_t n);

    string getName() const { return "x11"; }
    FcConfig* getFcConfig();
    FcFontSet* getFcFontSet();
    
  protected:
    bool allocate(TFont *font, const TMatrix2D *mat);
    void freeCoreFont(TFont *font);
    static bool buildFontList(FcConfig *config);
};

class TFontManagerFT:
  public TFontManager
{
  public:
    void init() const;
    void drawString(TPenBase *pen, int x, int y, const char *str, size_t len, bool transparent);
    int getHeight(TFont *font);
    int getAscent(TFont *font);
    int getDescent(TFont *font);
    int getTextWidth(TFont *font, const char *text, size_t n);

    string getName() const { return "freetype"; }
    FcConfig* getFcConfig();
    FcFontSet* getFcFontSet();
    
  protected:
    bool allocate(TFont *font, const TMatrix2D *mat);
    void freeCoreFont(TFont *font);
    static bool buildFontList(FcConfig *config);
};

class TFont:
  public TSmartObject
{
  public:
    TFont() {
      font = FcPatternDuplicate(default_font.font);
      fontmanager = TFontManager::getDefault();
      corefont = 0;
    }
    TFont(const TFont &f) {
      font = FcPatternDuplicate(f.font);
      fontmanager = f.fontmanager;
      corefont = 0;
    }
    TFont(const string &fontname) {
      font = 0;
      fontmanager = TFontManager::getDefault();
      setFont(fontname);
      corefont = 0;
    }
    virtual ~TFont();
    
    void setFont(const string &fontname);
    const char* getFont() const;

    const char *getFamily() const;
    void setFamily(const string &family);
    double getSize() const;
    void setSize(double);
    void setWeight(int weight);
    int getWeight() const;
    void setSlant(int slant);
    int getSlant() const;

    FcPattern *font;
    TFontManager *fontmanager;
    void *corefont;

    int getHeight() { return fontmanager->getHeight(this); }
    int getAscent() { return fontmanager->getAscent(this); }
    int getDescent() { return fontmanager->getDescent(this); }
    int getTextWidth(const char *text) {
      return fontmanager->getTextWidth(this, text, strlen(text));
    }
    int getTextWidth(const char *text, size_t n) {
      return fontmanager->getTextWidth(this, text, n);
    }
    int getTextWidth(const string &text) {
      return fontmanager->getTextWidth(this, text.c_str(), text.size());
    }
    
    static TFont default_font;
};

typedef GSmartPointer<TFont> PFont;

extern PFont default_font;
extern PFont bold_font;


#ifndef FC_WEIGHT_BOOK                       
#define FC_WEIGHT_BOOK 75            
#endif

#ifndef FC_DUAL
#define FC_DUAL 90
#endif

#ifndef FC_WEIGHT_THIN
#define FC_WEIGHT_THIN 0
#endif

#ifndef FC_WEIGHT_EXTRALIGHT
#define FC_WEIGHT_EXTRALIGHT 40
#endif

#ifndef FC_WEIGHT_ULTRALIGHT
#define FC_WEIGHT_ULTRALIGHT FC_WEIGHT_EXTRALIGHT
#endif

#ifndef FC_WEIGHT_REGULAR
#define FC_WEIGHT_REGULAR 80
#endif

#ifndef FC_WEIGHT_NORMAL
#define FC_WEIGHT_NORMAL FC_WEIGHT_REGULAR
#endif

#ifndef FC_WEIGHT_SEMIBOLD
#define FC_WEIGHT_SEMIBOLD FC_WEIGHT_DEMIBOLD
#endif

#ifndef FC_WEIGHT_EXTRABOLD
#define FC_WEIGHT_EXTRABOLD 205
#endif

#ifndef FC_WEIGHT_ULTRABOLD
#define FC_WEIGHT_ULTRABOLD FC_WEIGHT_EXTRABOLD
#endif

#ifndef FC_WEIGHT_HEAVY
#define FC_WEIGHT_HEAVY FC_WEIGHT_BLACK
#endif

#ifndef FC_WIDTH_CONDENSED
#define FC_WIDTH_CONDENSED 75
#endif

#ifndef FC_WIDTH_EXPANDED
#define FC_WIDTH_EXPANDED 125
#endif

#ifndef FC_WIDTH_NORMAL
#define FC_WIDTH_NORMAL 100
#endif

#ifndef FC_WIDTH_SEMICONDENSED
#define FC_WIDTH_SEMICONDENSED 87
#endif

#ifndef FC_WIDTH_EXTRACONDENSED
#define FC_WIDTH_EXTRACONDENSED 63
#endif

#ifndef FC_WIDTH_ULTRACONDENSED
#define FC_WIDTH_ULTRACONDENSED 50
#endif

#ifndef FC_WIDTH_SEMICONDENSED
#define FC_WIDTH_SEMICONDENSED 87
#endif

#ifndef FC_WIDTH_SEMIEXPANDED
#define FC_WIDTH_SEMIEXPANDED 113
#endif

#ifndef FC_WIDTH_EXTRAEXPANDED
#define FC_WIDTH_EXTRAEXPANDED 150
#endif

#ifndef FC_WIDTH_ULTRAEXPANDED
#define FC_WIDTH_ULTRAEXPANDED 200
#endif

#ifndef FC_WIDTH
#define FC_WIDTH "width"
#endif

} // namespace toad

#endif
