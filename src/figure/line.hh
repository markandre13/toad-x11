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

#ifndef _TOAD_FIGURE_LINE_HH
#define _TOAD_FIGURE_LINE_HH

#include <toad/figure.hh>

namespace toad {

/**
 * \ingroup figure
 */
class TFLine:
  public TFigure
{
  public:
    typedef TFigure super;
  
    TFLine();
    TFLine(int x1, int y1, int x2, int y2);
    void paint(TPenBase &, EPaintType);
    void getShape(TRectangle&);
    
    void translate(int dx, int dy);
    
    double distance(int x, int y);
    bool getHandle(unsigned, TPoint&);
    void translateHandle(unsigned h, int x, int y);

    unsigned mouseLDown(TFigureEditor*, int, int, unsigned);
    unsigned mouseMove(TFigureEditor*, int, int, unsigned);
    unsigned mouseLUp(TFigureEditor*, int, int, unsigned);
    
    TCloneable* clone() const { return new TFLine(*this); }
    const char * name() const { return "toad::TFLine"; }
    void store(TOutObjectStream&) const;
    bool restore(TInObjectStream&);
    
  protected:
    TPoint p1, p2;
};

} // namespace toad

#endif
