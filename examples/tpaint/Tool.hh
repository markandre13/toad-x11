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

#ifndef TTool
#define TTool TTool

class TTool
{
  public:
    virtual void mouseLDown(TBitmapEditor*,int,int,unsigned){}
    virtual void mouseLUp(TBitmapEditor*,int,int,unsigned){};
    virtual void mouseMove(TBitmapEditor*,int,int,unsigned){};
};

class TPlotTool: public TTool
{
  public:
    void mouseLDown(TBitmapEditor*,int x,int y,unsigned m);
    void mouseMove(TBitmapEditor*,int x,int y,unsigned m);
};

class TDrawTool: public TTool
{
  public:
    void mouseLDown(TBitmapEditor*,int x,int y,unsigned m);
    void mouseLUp(TBitmapEditor*,int x,int y,unsigned m);
    void mouseMove(TBitmapEditor*,int x,int y,unsigned m);
  private:
    int dx,dy, ox,oy, state;
    void invertLine(TBitmapEditor *be,int x1,int y1,int x2,int y2);
    void drawLine(TBitmapEditor *be,int x1,int y1,int x2,int y2);
};

class TFillTool: public TTool
{
  public:
    void mouseLDown(TBitmapEditor*,int x,int y,unsigned m);
  private:
    void Push(int x,int y);
    bool Pop(int *x,int *y);
    struct coord {int x,y;coord *next;};
    coord *stack;
};

#endif
