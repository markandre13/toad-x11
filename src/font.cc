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

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#define _TOAD_PRIVATE

#include <toad/toadbase.hh>
#include <toad/font.hh>

using namespace toad;

/**
 * \class toad::TFont
 * Represents fonts to be used with TPen.
 */

TFont::TFont()
{
  fs = NULL;
}

/**
 * Creates a new font.
 * <H4>family</H4>
 * <UL>
 * <LI>SANS
 * <LI>SANSSERIF
 * <LI>SERIF
 * <LI>TYPEWRITER
 * </UL>
 * <H4>style</H4>
 * <UL>
 * <LI>PLAIN
 * <LI>REGULAR
 * <LI>BOLD
 * <LI>ITALIC
 * <LI>BOLD_ITALIC
 * <LI>OBLIQUE
 * <LI>BOLD_OBLIQUE
 * </UL>
 * </TABLE>
 */
TFont::TFont(EFamily family, EStyle style,int size)
{
  fs = NULL;
  setFont(family, style, size);
}

TFont::TFont(const string &family, EStyle style, int size)
{
  fs = NULL;
  setFont(family, style, size);
}

TFont::~TFont()
{
  if (fs) {
//cout << "~TFont TFont::fid = " << fs->fid << " removed" << endl;
    XUnloadFont(x11display, fs->fid);
    XFreeFontInfo(NULL,fs,0);
  }
}

int TFont::getTextWidth(const string &str) const
{
  return getTextWidth(str.c_str(),str.size());  
}

int TFont::getTextWidth(const char *str) const
{
  return getTextWidth(str,strlen(str)); 
}

int TFont::getTextWidth(const char *str, int len) const
{
  TOAD_XLIB_MTLOCK();
  register int l=XTextWidth(fs,str,len);
  TOAD_XLIB_MTUNLOCK();
  return l;
}

int TFont::getAscent() const {return fs?fs->ascent:0;}
int TFont::getDescent() const {return fs?fs->descent:0;}
int TFont::getHeight() const {return getAscent()+getDescent();}

unsigned 
TFont::getHeightOfTextFromWidth(const string &text, unsigned width) const
{
  return getHeightOfTextFromWidth(text.c_str(),width);
}


unsigned 
TFont::getHeightOfTextFromWidth(const char* text, unsigned width) const
{
  unsigned i, y=getHeight();

  // 1st step: count number of words and lines
  unsigned word_count, min_lines;
  count_words_and_lines(text, &word_count, &min_lines);
  if (!word_count) return 0;

  // 2nd step: collection information on each word
  TWord* word = make_wordlist(text, word_count);
  
  // 3rd step: output
  unsigned blank_width = getTextWidth(" ",1);
  unsigned line_len = 0;
  unsigned word_of_line = 1;
  
  for(i=0; i<word_count; i++) {
    if ((line_len+word[i].len>width && i!=0) || word[i].linefeeds) {
      if (word[i].linefeeds)
        y+=getHeight()*word[i].linefeeds;
      else
        y+=getHeight();
      line_len = 0;
      word_of_line = 0;
    }
    line_len+=word[i].len+blank_width;
    word_of_line++;
  }
  delete[] word;
  return y+getHeight();
}

void TFont::count_words_and_lines(const char *text, unsigned* word_count, unsigned* min_lines) const
{
  *word_count = 0;
  *min_lines = 1;
  const char* ptr = text;
  bool word_flag = false;
  while(*ptr) {
    if(!word_flag && *ptr!=' ' && *ptr!='\n') {
      word_flag=true;
      (*word_count)++;
    } else 
    if (word_flag && (*ptr==' ' || *ptr=='\n'))
      word_flag=false;
    if (*ptr=='\n')
      (*min_lines)++;
    ptr++;
  }
}

TFont::TWord* TFont::make_wordlist(const char *text, unsigned word_count) const
{
  TWord* word = new TWord[word_count];

  unsigned j,i = 0;
  const char* ptr = text;
  bool word_flag = false;
  unsigned lf=0;
  while(*ptr) {
    if(!word_flag && *ptr!=' ' && *ptr!='\n') {
      word[i].pos = ptr;
      j = 0;
      word_flag=true;
    }
    ptr++;
    j++;
    if (word_flag && (*ptr==' ' || *ptr=='\n' || *ptr==0)) {
      word[i].bytes     = j;
      word[i].len       = getTextWidth(word[i].pos,j);
      word[i].linefeeds = lf;
      word_flag=false;
//      printf("word %2u, bytes=%i\n",i,j);
      i++;
      lf=0;
    }
    if(*ptr=='\n')
      lf++;
  }
//  printf("word_count=%i\n",word_count);
  return word;
}

void TFont::setFont(const string &family, EStyle style, int size)
{
  cerr << __PRETTY_FUNCTION__ << " isn't implemented yet" << endl;
}

void TFont::setFont(const string &x11fontname)
{
  if (fs) {
    XUnloadFont(x11display, fs->fid);
    XFreeFontInfo(NULL,fs,0);
  }
  fs = NULL;
  fs = XLoadQueryFont(x11display, x11fontname.c_str());
  if (!fs) {
    fprintf(stderr, "no such font\n");
  }
}

/**
 * The base function to set a font
 *
 * Prefers bitmap fonts when available in the right size, which
 * improves quality quite a lot.
 */
//----------------------------------------------------------------------------
void TFont::setFont(EFamily family, EStyle style, int size)
{
  if (fs) {
//    cout << "SetFont TFont::fid = " << fs->fid << " removed" << endl;
    XUnloadFont(x11display, fs->fid);
    XFreeFontInfo(NULL,fs,0);
  }
  fs = NULL;

  // this should be done in `build_fontname' when there's no
  // italic variant available!!!
  if (family==SANS) {
    switch(style) {
      case ITALIC:
        style = OBLIQUE;
        break;
      case BOLD_ITALIC:
        style = BOLD_OBLIQUE;
        break;
      default:
        break;
    }
  }

  build_fontname(family, style, size);

  int count;
  char **fl;
  
  fl = XListFonts(x11display, mask.c_str(), 10, &count);
  if (count==0) {
    switch(style) {
      case OBLIQUE:
        style = ITALIC;
        break;
      case ITALIC:
        style = OBLIQUE;
        break;
      case BOLD_OBLIQUE:
        style = BOLD_ITALIC;
        break;
      case BOLD_ITALIC:
        style = BOLD_OBLIQUE;
        break;
      default:
        break;
    }
    build_fontname(family, style, size);
    fl = XListFonts(x11display, mask.c_str(), 10, &count);
  }

  if (count==0) {
    printf("couldn't find font\n");
    exit(1);
  }
  
  // try to load a bitmap font
  //---------------------------
  for (int i=0; i<count; i++) {
    int j=0, c=0;
    while(c<12) {
      if (fl[i][j]=='-')
        c++;
      j++;
    }
    if (fl[i][j]=='0' && fl[i][j+1]=='-') // seems to be an outline font
      continue;
    
    fs = XLoadQueryFont(x11display, fl[i]);
    if (!fs) {
//      printf("couldn't load font %i\n",i);
      continue;
    }
    break;
  }

  // try to load a vector font
  //---------------------------
  if (!fs) {
    for (int i=0; i<count; i++) {
      int j=0, c=0;
      while(c<12) {
        if (fl[i][j]=='-')
          c++;
        j++;
      }
      if (fl[i][j]!='0' || fl[i][j+1]!='-')
        continue;
      
      fs = XLoadQueryFont(x11display, fl[i]);
      if (!fs) {
//        printf("couldn't load font %i\n",i);
        continue;
      }
      break;
    }
  }

  if (!fs) {
    fprintf(stderr, "no such font\n");
  }
  XFreeFontNames(fl);
//if (fs)
//  cout << "SetFont TFont::fid = " << fs->fid << endl;
}

/**
 * Build X11 fontname
 */
void TFont::build_fontname(EFamily family, EStyle style,int size)
{
  mask = "-*-";
  
  switch(family)
  {
    case SANS:
      mask += "helvetica";
      break;
    case SERIF:
      mask += "times";
      break;
    case TYPEWRITER:
      mask += "courier";
      break;
  }
  switch(style)
  {
    case REGULAR:
      mask += "-medium-r-";
      break;
    case BOLD:
      mask += "-bold-r-";
      break;
    case ITALIC:
      mask += "-medium-i-";
      break;
    case BOLD_ITALIC:
      mask += "-bold-i-";
      break;
    case OBLIQUE:
      mask += "-medium-o-";
      break;
    case BOLD_OBLIQUE:
      mask += "-bold-o-";
      break;
  }
  mask+="normal--";         // width
  
  char buffer[25];          // size
  sprintf(buffer,"%i",size);
  mask+=buffer;
  
  mask+="-*-*-*-*-*-iso8859-1";
//  printf("mask: %s\n",mask.c_str());
}
