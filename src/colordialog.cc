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

#include <toad/colordialog.hh>
#include <toad/scrollbar.hh>
#include <toad/pushbutton.hh>
#include <toad/textfield.hh>
#include <cmath>

using namespace toad;

TColorDialog::TColorDialog(TWindow *parent, const string &title):
  super(parent, title), color(0)
{
  _init();
}

TColorDialog::TColorDialog(TWindow *parent, const string &title, TRGB *aColor):
  super(parent, title), color(aColor)
{
  _init();
}

TColorDialog::~TColorDialog()
{
  delete bmp1;
  delete bmp2;
}

static void
hsv2rgb(double h, double s, double v, int *red, int *green, int *blue)
{
  int w = static_cast<int>(v) * 255;
  if (s == 0) {
    *red = w;
    *green = w;
    *blue = w;
  } else {
    double f;
    int p, q, r;
    while(h<0.0)
      h+=360.0;
    while(h>360.0)
      h-=360.0;
    h = h / 60.0;
    int i = (int) h;
    f = h - i;
    p = static_cast<int>(v * (1.0 - s) * 255.0);
    q = static_cast<int>(v * (1.0 - s * f) * 255.0);
    r = static_cast<int>(v * (1.0 - s * (1.0 - f)) * 255.0);
    switch (i) {
      case 0: *red = w; *green = r; *blue = p; break;
      case 1: *red = q; *green = w; *blue = p; break;
      case 2: *red = p; *green = w; *blue = r; break;
      case 3: *red = p; *green = q; *blue = w; break;
      case 4: *red = r; *green = p; *blue = w; break;
      case 5: *red = w; *green = p; *blue = q; break;
    }
  }
}

void
TColorDialog::_init()
{
  setSize(478, 8+256+8+25+8);
  setMouseMoveMessages(TMMM_LBUTTON); // only mouseMove when left button down
  if (color) {
    origcolor = *color;
  } else {
    origcolor.set(128, 128, 128);
  }
  createBitmaps();

  lock = false;
  hue.       setRangeProperties(0, 0, 0, 360);
  saturation.setRangeProperties(0, 0, 0, 100); // 0.0-1.0
  value.     setRangeProperties(0, 0, 0, 100); // 0.0-1.0
  rgb = origcolor;
  rgb2hsv();

  int x,y,w,h,hmax=0,d=8;
  x=8+256+8+16+8+12; y=8+32+8; w=128; h=17; 
  
  TScrollBar *sb;
  TTextField *tf;
  for(int i=0; i<6; ++i) {
    switch(i) {
      case 0:
        sb = new TScrollBar(this, "sb.hue", &hue);
        tf = new TTextField(this, "tf.hue", &hue);
        break;
      case 1:  
        sb = new TScrollBar(this, "sb.saturation", &saturation);
        tf = new TTextField(this, "tf.hue", &saturation);
        break;
      case 2:
        sb = new TScrollBar(this, "sb.value", &value);
        tf = new TTextField(this, "tf.hue", &value);
        break;
      case 3:  
        sb = new TScrollBar(this, "sb.red", &rgb.r);
        tf = new TTextField(this, "tf.hue", &rgb.r);
        break;
      case 4:  
        sb = new TScrollBar(this, "sb.green", &rgb.g);
        tf = new TTextField(this, "tf.hue", &rgb.g);
        break;
      case 5:
        sb = new TScrollBar(this, "sb.blue", &rgb.b);
        tf = new TTextField(this, "tf.hue", &rgb.b);
        break;
    }
    if (i<3) {
      connect(sb->getModel()->sigChanged, this, &TColorDialog::hsv2rgb);
    } else {
      connect(sb->getModel()->sigChanged, this, &TColorDialog::rgb2hsv);
    }
    sb->setShape(x,y,w,h);
    tf->setShape(x+w+4,y,30,h);
    y+=h+d;
  }

  tf = new TTextField(this, "hex", &rgb);
  w=64;
  x=getWidth()-8-w;
  y+=4;
  tf->setShape(x,y,w,h);
  
  hmax = h;
  
  TPushButton *pb;
  
  w = 80;
  h = 25;
  x=(getWidth()-w-8-w)/2;
  y=8+256+8;
  
  pb = new TPushButton(this, "Abort");
  pb->setShape(x,y,w,h);
  connect(pb->sigActivate, this, &TColorDialog::done, false);

  x+=w+8;
  
  pb = new TPushButton(this, "OK");
  pb->setShape(x,y,w,h);
  connect(pb->sigActivate, this, &TColorDialog::done, true);
}

void
TColorDialog::createBitmaps()
{  
  bmp1 = new TBitmap(256, 256, TBITMAP_TRUECOLOR);
  int iy, ix;
  double x, y;
  for(iy=0, y=-1.0; iy<256; ++iy, y+=2.0/255.0) {
    for(ix=0, x=-1.0; ix<256; ++ix, x+=2.0/255.0) {
      double s = hypot(x, y);
      if (s<=1.0) {
        int r, g, b;
        ::hsv2rgb( (atan2(y, x) + M_PI) / (2.0*M_PI) * 360.0, s, 1.0,
                   &r, &g, &b);
        bmp1->setPixel(ix,iy,r,g,b);
      }
    }
  }

  bmp2 = new TBitmap(16,256, TBITMAP_TRUECOLOR);
  for(unsigned y=0; y<256; ++y) {
    for(unsigned x=0; x<16; ++x) {
      unsigned v = 256-y;
      bmp2->setPixel(x,y,v,v,v);
    }
  }
}

void
TColorDialog::mouseLDown(int mx, int my, unsigned)
{
  if (mx>=8 && mx<=8+256 &&
      my>=8 && my<=8+256) 
  {
    double x = (double)mx - 8.0 - 128.0;
    double y = (double)my - 8.0 - 128.0;
    double s = hypot(x, y);
    if (s>128.0)
      s=128.0;
    double h = (atan2(y, x) + M_PI) / (2*M_PI) * 360.0;
    lock = true;
    saturation = (int)s * 100 / 128;
    hue = (int)h;
    lock = false;
    hsv2rgb();
  } else
  
  if (mx>=8+256+8 && mx<=8+256+8+16 &&
      my>=8 && my<=8+256)
  {
    value = (8 + 256 - my) * 100 / 255;
  }
}

void
TColorDialog::mouseMove(int x, int y, unsigned m)
{
  mouseLDown(x, y, m);
}

void
TColorDialog::paint()
{
  TPen pen(this);
  
  pen.drawBitmap(8      , 8, bmp1); // HS
  pen.drawBitmap(8+256+8, 8, bmp2); // V
  
  // paint HSV carets
  double h = (double)hue / 360.0 * 2*M_PI - M_PI;
  double s = (double)saturation / 100.0 * 128.0;
  
  int x = static_cast<int>(s*cos(h)) +8+128;
  if (x>8+255) x=8+255;
  int y = static_cast<int>(s*sin(h)) +8+128;
  if (y>8+255) y=8+255;
  int z = 8+255-value*255/100;

  pen.setColor(0,0,0);
  pen.fillRectanglePC(x-2, y-2, 5,5);
  pen.fillRectanglePC(8+256+8,z-2,16,5);

  pen.setColor(255,255,255);
  pen.fillRectanglePC(x-1, y-1, 3,3);
  pen.fillRectanglePC(8+256+8+1,z-1,14,3);
  
  pen.draw3DRectanglePC(8-2      ,8-2,256+4,256+4);
  pen.draw3DRectanglePC(8-2+256+8,8-2,16+4,256+4);

  // draw HSVRGB
  x=8+256+8+16+8; y=8+32+8;
  static const char * txt[] = { "H", "S", "V", "R", "G", "B" };
  pen.setColor(0,0,0);
  for(int i=0; i<6; ++i) {
    pen.drawString(x,y+2, txt[i]);
    y+=17+8;
  }
  
  y+=4;
  pen.drawString(x+44,y+2, "Hex Triplet:");
  
  // draw color field
  pen.setColor(origcolor);
  pen.fillRectanglePC(8+256+8+16+8+12, 8, 64, 32);
  
  pen.setColor(rgb.r.getValue(), rgb.g.getValue(), rgb.b.getValue());
  pen.fillRectanglePC(8+256+8+16+8+12 + 64 , 8, 64, 32);

  pen.draw3DRectanglePC(8+256+8+16+8+12, 8, 64+64, 32);
}

void
TColorDialog::hsv2rgb()
{
  if (lock)
    return;
  lock = true;

//  cerr << "hsv2rgb" << endl;
  int i;
  int w = (int)value * 255 / 100;

  if (saturation == 0) {
    rgb.set(w,w,w);
  } else {
    float f, h, v, s;
    int p, q, r;
    h = static_cast<float>(hue==360?0:hue) / 60.0;
    s = (float)saturation / 100.0;
    v = (float)value / 100.0;
    i = (int)h;
    f = h - i;
    p = static_cast<int>(v * (1.0 - s) * 255.0);
    q = static_cast<int>(v * (1.0 - s * f) * 255.0);
    r = static_cast<int>(v * (1.0 - s * (1.0 - f)) * 255.0);
    switch (i) {
      case 0: rgb.set(w, r, p); break;
      case 1: rgb.set(q, w, p); break;
      case 2: rgb.set(p, w, r); break;
      case 3: rgb.set(p, q, w); break;
      case 4: rgb.set(r, p, w); break;
      case 5: rgb.set(w, p ,q); break;
    }
  }
  invalidateWindow(false);

  lock = false;
}

void
TColorDialog::rgb2hsv()
{
  if (lock)
    return;
  lock = true;

//  cerr << "rgb2hsv" << endl;

  float h=0,s=1.0,v=1.0;
  float max_v,min_v,diff,r_dist,g_dist,b_dist;
  float undefined = 0.0;

  float r = (float)rgb.r / 255.0;
  float g = (float)rgb.g / 255.0;
  float b = (float)rgb.b / 255.0;

  max_v = max(max(r,g),max(g,b));
  min_v = min(min(r,g),min(g,b));
  diff = max_v - min_v;
  v = max_v;

  if( max_v > 0.009 ) // max_v != 0.0
    s = diff/max_v;
  else
    s = 0.0;
  if( s > 0.009 ) { // s != 0.0
    r_dist = (max_v - r)/diff;
    g_dist = (max_v - g)/diff;
    b_dist = (max_v - b)/diff;
    if( r == max_v ) {
      h = b_dist - g_dist;
    } else {
      if( g == max_v ) {
        h = 2.0 + r_dist - b_dist;
      } else {
        if (b == max_v) {
          h = 4.0 + g_dist - r_dist;
        } else {
          cerr << "rgb2hsv::How did I get here?" << endl;
        }
      }
    }
    h *= 60.0;
    if (h < 0.0)
      h += 360.0;
    hue = static_cast<int>(h);
  }
  saturation = static_cast<int>(s * 100.0);
  value = static_cast<int>(v * 100.0);
  invalidateWindow(false);

  lock = false;
}

void
TColorDialog::done(bool apply)
{
  if (apply && color)
    color->set(rgb.r.getValue(), rgb.g.getValue(), rgb.b.getValue());
  destroyWindow();
}
