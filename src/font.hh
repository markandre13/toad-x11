/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2003 by Mark-André Hopf <mhopf@mark13.de>
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

#ifndef TFont
#define TFont TFont

#include <toad/toadbase.hh>
#include <toad/pointer.hh>

namespace toad {

class TPen;

class TFont:
  public TSmartObject, public TOADBase
{
  friend class TPen;
  
  public:
    enum EFamily
    {
      SANS=0, SANSSERIF=0,
      SERIF=1, 
      TYPEWRITER=3
    };
      enum EStyle
      {
      PLAIN = 0, 
      REGULAR = 0,
      BOLD = 1,
      ITALIC = 2,
      BOLD_ITALIC = 3,
      OBLIQUE = 4,
      BOLD_OBLIQUE = 5
      };
    TFont();
    TFont(EFamily,EStyle,int);
    TFont(const string &family, EStyle, int);
    void setFont(const string &x11fontname);
    void setFont(EFamily,EStyle,int);
    void setFont(const string& family, EStyle, int);
    virtual ~TFont();

    int getTextWidth(const string&) const;
    int getTextWidth(const char*) const;
    int getTextWidth(const char*,int len) const;
    int getTextWidth(const unsigned char *s) const { return getTextWidth((const char*)s); }
    int getTextWidth(const unsigned char *s,int len) const { return getTextWidth((const char*)s,len); }
    int getAscent() const;
    int getDescent() const;
    int getHeight() const;
    unsigned getHeightOfTextFromWidth(const string &text, unsigned width) const;
    unsigned getHeightOfTextFromWidth(const char* text, unsigned width) const;

  private:
    _TOAD_FONT fs;
    string mask;
    void build_fontname(EFamily family, EStyle style,int size);

    struct TWord
    {
      const char* pos;
      unsigned bytes;
      unsigned len;
      unsigned linefeeds;
    };
    void count_words_and_lines(const char*, unsigned*, unsigned*) const;
    TWord* make_wordlist(const char*,unsigned) const;
};

typedef GSmartPointer<TFont> PFont;

extern PFont default_font;
extern PFont bold_font;

} // namespace toad

#endif
