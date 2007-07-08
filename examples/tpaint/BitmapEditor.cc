/*
 * TPaint -- a simple bitmap editor
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

#include <toad/toad.hh>
#include <toad/dnd/color.hh>

#include "BitmapEditor.hh"
#include "Tool.hh"

extern TTool *tool;
extern TBitmapEditor *bme;
extern void RegisterBitmapEditor(TBitmapEditor *ed);
extern void RemoveBitmapEditor(TBitmapEditor *ed);

TBitmapEditor::TBitmapEditor(TWindow *p,const string &t)
  :TWindow(p,t)
{
  w=32; h=32; zoom=1;
  filename = NULL;
  bitmap = NULL;
  bNoBackground = true;
}

void TBitmapEditor::create()
{
  ::RegisterBitmapEditor(this);

  if (!bitmap)
  { 
    bitmap = new TAlterBitmap(w,h,TBITMAP_TRUECOLOR);
    for(int y=0; y<h; y++)
      for(int x=0; x<w; x++)
        bitmap->setPixel(x,y,255,255,255);
  }
  setSize(w*zoom,h*zoom);
  setMouseMoveMessages(TMMM_LBUTTON);
}

void TBitmapEditor::destroy()
{
  ::RemoveBitmapEditor(this);
  if (bitmap)
  {
    delete bitmap;
    bitmap=NULL;
  }
}

void TBitmapEditor::focus(bool)
{
//  printf("TBitmapEditor::focus(%s)\n",b?"true":"false");
  if (isFocus()) bme = this;  // set current TBitmapEditor
}

void TBitmapEditor::keyDown(TKey, char *string,unsigned)
{
  switch(string[0])
  {
    case '+': case '>':
      if (zoom<16) {
        zoom++;
        setSize(w*zoom,h*zoom);
      }
      
      break;
    case '-': case '<':
      if(zoom>1) {
        zoom--;
        setSize(w*zoom,h*zoom);
      }
      break;
  }
}

void TBitmapEditor::setEditSize(int w,int h)
{
  this->w = w;
  this->h = h;
}

void TBitmapEditor::setEditZoom(int z)
{
  zoom = z;
  if (isRealized())
  {
    setSize(w*zoom,h*zoom);
  }
}

bool TBitmapEditor::save(const char* filename, void *xtra)
{
  if (!bitmap) return false;
  return bitmap->save(filename,xtra);
}

bool TBitmapEditor::load(const char* fn)
{
  if (filename)
    delete filename;
  filename=new char[strlen(fn)+1];
  strcpy(filename,fn);
  if (!bitmap)
  {
    bitmap = new TAlterBitmap;
  }

  try {
    bitmap->load(filename);
  }
  catch(runtime_error &e) {
    messageBox(NULL, "Load Bitmap Failed", e.what(), 
               TMessageBox::ICON_STOP | TMessageBox::OK);
    return false;
  }
//    SetTitle(filename);
  w = bitmap->width;
  h = bitmap->height;
  setSize(w*zoom, h*zoom);
  if (!isRealized()) {
    createWindow();
    //TPen pen(this);
    //pen.SetColorMode(TColor::DITHER);
    // bitmap->SetZoom(zoom);
    //pen.DrawBitmap(0,0,*bitmap);
  } else {
    invalidateWindow();
  }
  return true;
}

void TBitmapEditor::thread()
{
}

bool TBitmapEditor::isInside(int x,int y)
{
  return (x>=0 && x<w && y>=0 && y<h);
}

void TBitmapEditor::paint()
{
  TPen pen(this);
  TRectangle rect;
  pen.getClipBox(&rect);
  rect.x/=zoom;
  rect.y/=zoom;
  rect.w=(rect.w/zoom)+((rect.w%zoom)?2:1);
  rect.h=(rect.h/zoom)+((rect.h%zoom)?2:1);

  pen.setColorMode(TColor::DITHER);

  bitmap->setZoom(zoom);
  pen.drawBitmap(0,0,*bitmap);
}

void TBitmapEditor::mouseLDown(int x,int y,unsigned m)
{
  x/=zoom;
  y/=zoom;
  tool->mouseLDown(this,x,y,m);
}

void TBitmapEditor::mouseMDown(int x,int y,unsigned modifier)
{
  TColor c;
  x/=zoom; y/=zoom;
  getPixel(x,y, &c);
  startDrag(
    new TDnDColor(c), 
    modifier);
}

void TBitmapEditor::mouseLUp(int x,int y,unsigned m)
{
  x/=zoom;
  y/=zoom;
  tool->mouseLUp(this,x,y,m);
}

void TBitmapEditor::mouseMove(int x,int y,unsigned m)
{
  x/=zoom;
  y/=zoom;
  tool->mouseMove(this,x,y,m);
}

void TBitmapEditor::setPixel(int x,int y,TColor &color,TPen &pen)
{
  pen.setColorMode(TColor::DITHER);
  pen.setColor(color.r,color.g,color.b);
  pen.fillRectangle(x*zoom,y*zoom,zoom,zoom);
  
  bitmap->setPixel(x,y,color.r,color.g,color.b);
}

bool TBitmapEditor::getPixel(int x,int y,TColor *color)
{
  return bitmap->getPixel(x,y,color);
}

void TBitmapEditor::invertPixel(int x,int y,TPen &pen)
{
  pen.setMode(TPen::INVERT);
  pen.drawRectangle(x*zoom,y*zoom,zoom,zoom);
}
