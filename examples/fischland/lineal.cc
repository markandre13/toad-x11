/*
 * Fischland -- A 2D vector graphics editor
 * Copyright (C) 1999-2004 by Mark-André Hopf <mhopf@mark13.org>
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

#include "lineal.hh"

using namespace toad;

TLineal::TLineal(bool vertical)
{
  this->vertical = vertical;
}

int
TLineal::getSize()
{
  return 16;
}

/*
  o our internal resolution is 9600dpi
  o our assumed screen resolution is 100dpi
  o 1cm = 0.394in
  o 2.54cm = 1in
*/
void
TLineal::render(TPenBase &pen, int pos, int size, TMatrix2D *mat)
{
//cerr << "render pos " << pos << " with size " << size << endl;

  // paint header background
  pen.setColor(TColor::BTNFACE);
  if (!vertical) {
    pen.fillRectangle(pos, 0, size, 16);
    pen.setColor(TColor::BTNLIGHT);
    pen.drawLine(pos,0,pos+size,0);
    pen.setColor(TColor::BTNSHADOW);
    pen.drawLine(pos,15,pos+size,15);
  } else {
    pen.fillRectangle(0, pos, 16, size);
    pen.setColor(TColor::BTNLIGHT);
    pen.drawLine(0,pos,0,pos+size);
    pen.setColor(TColor::BTNSHADOW);
    pen.drawLine(15,pos,15,pos+size);
  }
  pen.setColor(0,0,0);

return;

  // draw scale
  double res;
  if (mat) {
    double x0, x1, y0, y1;
    mat->map(0,0, &x0, &y0);
    mat->map(9600,9600, &x1, &y1);
  
    double res;
    if (!vertical)
      res = x1-x0;
    else
      res = y1-y0;
  } else {
    res = 1.0;
  }
    
  // res is the resolution in percent
cerr << "res = " << res << endl;
#if 1
  double step = res;
  while(true) {
    double s = step / 10.0;
    if (s<10.0)
      break;
    step = s;
  }
  while(true) {
    double s = step / 5.0;
    if (s<10.0)
      break;
    step = s;
  }
cerr << "step = " << step << endl;
  
  pos = (pos/step)*step;
  for(double i=pos; i<pos+size; i+=step) {
    int y = 10;
/*
    if (i%20==0)
      y = 8;
*/
    int nres = static_cast<int>(res);
    if ( nres && (static_cast<int>(i)%nres) == 0 )
      y = 2;
    if (!vertical)
      pen.drawLine(i,16,i,y);
    else
      pen.drawLine(16,i,y,i);

    if (y==2) {
      char buffer[64];
      snprintf(buffer, sizeof(buffer), "%.1lf", ((double)i/res));
      if (!vertical) {
        pen.drawString(i+2, 0, buffer);
      } else {
        pen.push();
        pen.translate(0,i+2);
        pen.rotate(90);
        pen.drawString(0, -pen.getHeight(), buffer);
        pen.rotate(-90);
        pen.pop();
      }
    }
    
  }
  
#else
  int f = res/10;
  
  pos = (pos/f)*f;
  for(unsigned i=pos; i<pos+size; i+=f) {
    int y = 0;
    if (i%10==0)
      y = 10;
    if (i%20==0)
      y = 8;
    if (i%100==0)
      y = 2;
    if (y!=0) {
      if (!vertical)
        pen.drawLine(i,16,i,y);
      else
        pen.drawLine(16,i,y,i);
    }
    if (y==2) {
      char buffer[64];
      snprintf(buffer, sizeof(buffer), "%.1lf", ((double)i/res));
      if (!vertical) {
        pen.drawString(i+2, 0, buffer);
      } else {
        pen.push();
        pen.translate(0,i+2);
        pen.rotate(90);
        pen.drawString(0, -pen.getHeight(), buffer);
        pen.rotate(-90);
        pen.pop();
      }
    }
  }
#endif
}
