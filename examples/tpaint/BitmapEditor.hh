/* TPaint -- a simple bitmap editor
 * Copyright (C) 1996-2002 by Mark-André Hopf <mhopf@mark13.de>
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef TBitmapEditor
#define TBitmapEditor TBitmapEditor

#include <toad/color.hh>
#include <toad/pen.hh>

using namespace toad;

class TBitmapEditor: public TWindow
{
    int w,h,zoom;
  public:
    TBitmapEditor(TWindow *p,const string &t);
    bool save(const char*,void*);
    bool load(const char*);
    void setPixel(int x,int y,TColor &,TPen &);
    bool getPixel(int x,int y,TColor *);
    void invertPixel(int x,int y,TPen &);
    bool isInside(int x,int y);

    void setEditSize(int x,int y);
    void setEditZoom(int z);
    int  getEditZoom();

  protected:
    void create();
    void destroy();
    void paint();
    void mouseLDown(int,int,unsigned);
    void mouseLUp(int,int,unsigned);
    void mouseMDown(int,int,unsigned);
    void mouseMove(int,int,unsigned);
    void keyDown(TKey key, char *string, unsigned state);
    void focus(bool);

    char *filename;
    bool bThread;   
    void thread();

  private:
    TAlterBitmap *bitmap;
};

inline int TBitmapEditor::getEditZoom()
{
  return zoom;
}

#endif
