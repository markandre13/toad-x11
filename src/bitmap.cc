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

#ifdef __X11__
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif

#define _TOAD_PRIVATE
#include <toad/toad.hh>
#include <toad/io/urlstream.hh>
#include <sstream>

//#include <toad/filedialog.hh>
#include <toad/bitmapfilter.hh>
#include <toad/filter_bmp.hh>
#include <toad/filter_png.hh>
#include <toad/filter_gif.hh>
#include <toad/filter_jpeg.hh>

#include <assert.h>

using namespace toad;

#define THROW(CMD) { \
  ostringstream out; out << CMD << '\0'; \
  throw runtime_error(out.str()); \
}

/**
 * \class toad::TAlterBitmap
 * <B>THIS CLASS IS SCHEDULED FOR A COMPLETE REWRITE</B>
 * <P>
 * A bitmap for fast bitmap operations. Unlike <A HREF="TBitmap.html">TBitmap</A>
 * the data isn't stored
 * in the X11 server but on the same computer as your program.<BR>
 * The current implementation does contain some bugs but have a look at
 * TPaint program how it may be used.
 */
 
/**
 * \class toad::TBitmap
 * <B>THIS CLASS IS SCHEDULED FOR A COMPLETE REWRITE</B>
 * <P>
 * A bitmap is a rectangular raster graphic that can be used with
 * <A HREF="TPen.html">TPen</A>::DrawBitmap</A>, TWindow::SetIcon,
 * TOADBase::MessageBox and other methods.
 *
 * \todo
 * \li While loading all bitmaps are  stored pixel by pixel into a 24bit RGB 
 *     buffer. Line by line would be faster.
 * \li merging server- & client-side bitmaps in a single class was either
 *     a bad idea or I was to stupid to implement it
 */
 
// Open
//-----------------------------------------------------------------------------
void TBitmap::open()
{
  // install import/export filters
  //-------------------------------

  addFilter(new TFilterGIF);

#ifdef HAVE_LIBJPEG
  addFilter(new TFilterJPEG);
#endif
  addFilter(new TFilterBMP);

#ifdef HAVE_LIBPNG
  addFilter(new TFilterPNG);
#endif

//  AddFilter(new TFilterPCX);
//  AddFilter(new TFilterTIFF);
//  AddFilter(new TFilterIFF);
//  AddFilter(new TFilterTarga);  // see c't 11/95
}


static TBitmapFilter* filter_list = NULL;

// Close
//----------------------------------------------------------------------------
void TBitmap::close()
{
  // remove import/export filters
  //------------------------------  
  TBitmapFilter *ptr;
  while(filter_list) {
    ptr = filter_list;
    filter_list = filter_list->next;
    delete ptr;
  }
}

// AddFilter
//---------------------------------------------------------------------------- 

bool TBitmap::addFilter(TBitmapFilter* flt)
{
  flt->next = filter_list;
  filter_list = flt;
  return true;
}

#if 0

bool TBitmap::getInputFilter(TFileDialog &fd)
{
  fd.AddFileType("all files","",NULL);
  
  TBitmapFilter *filter = filter_list;
  while(filter) {
    fd.AddFileType(filter->getName(),filter->getExt(),static_cast<void*>(filter));
    filter = filter->next;
  }
  return true;
}

bool TBitmap::getOutputFilter(TFileDialog &fd)
{
  TBitmapFilter *filter = filter_list;
  
  while(filter) {
    fd.addFileType(filter->getName(),filter->getExt(),static_cast<void*>(filter));
    filter = filter->next;
  }
  return true;
}

#endif

// Constructor
//---------------------------------------------------------------------------- 
TBitmap::TBitmap()
{
  color=NULL;
  index=NULL;
  width = height = 0;
  zoom = 1;
  modified = false;

  mode = TBITMAP_SHOW;
#ifdef __X11__
  dither = TColor::_shouldNotDither() ? TBITMAP_SUBSTITUTE : TBITMAP_FLOYD_STEINBERG;
#endif
  
  pixmap = 0;
  mask   = 0;
}

TBitmap::TBitmap(int w,int h, EBitmapType type)
{
  pixmap = 0;
  mask   = 0;
  pix_width = pix_height = 0;
  width = w;
  height = h;
  zoom = 1;

  switch(type) {
    case TBITMAP_INDEXED:
      color = new TRGB[256];
      index = new unsigned char[w*h];
      break;
    case TBITMAP_TRUECOLOR:
      color = new TRGB[w*h];
      index = NULL;
      break;
    case TBITMAP_SERVER:
      color = NULL;
      index = NULL;
#ifdef __X11__
      pixmap = XCreatePixmap(
          x11display,
          RootWindow(x11display, x11screen),
          width,height,
          DefaultDepth(x11display, x11screen)
      );
#endif
  }
  modified = false;
  mode = TBITMAP_SHOW;
#ifdef __X11__
  dither = TColor::_shouldNotDither() ? TBITMAP_SUBSTITUTE : TBITMAP_FLOYD_STEINBERG;
#endif
}

TBitmap::~TBitmap()
{
//  cout << "destroying bitmap " << this << endl;
  if (color) delete[] color;
  if (index) delete[] index;
  
#ifdef __X11__
  if (x11display) {
    if (pixmap)
      XFreePixmap(x11display, pixmap);
    if (mask)
      XFreePixmap(x11display, mask);
  }
#endif
}

// experimental bitmap mask code
//----------------------------------------------------------------------------

/**
 * \class toad::TBitmapMask
 * <B>Kludge Warning: This class is experimental</B> 
 *
 * The bitmap mask tells a bitmap where it's pixels should be transparent.
 * <P> 
 * See the methods `SetMask(const TBitmapMask &m)' and `ClearMask()' in
 * TBitmap.
 */

//! Removes the <A HREF="TBitmap.html">bitmap mask</A>.
void TBitmap::clearMask()
{
#ifdef __X11__
  if (x11display && mask) {
    XFreePixmap(x11display, mask);
    mask = 0;
  }
#endif
}

//! Sets the bitmap mask.
void TBitmap::setMask(const TBitmapMask &m)
{
#ifdef __X11__
  if (!mask) {
      mask = XCreatePixmap(
          x11display,
          RootWindow(x11display, x11screen),
          width,height,
          1
      );
  }
  GC gc = XCreateGC(x11display, mask,0,0);
  XSetForeground(x11display, gc, 1);
  XFillRectangle(x11display, mask, gc, 0,0,width,height);
  XSetForeground(x11display, gc, 0);
  for(int y=0; y<height; y++) {
    for(int x=0; x<width; x++) {
      if (!m.getPixel(x,y)) {
        XFillRectangle(x11display, mask, gc, x,y,1,1);
      }
    }
  }
  XFreeGC(x11display, gc);
#endif
}

TBitmapMask::TBitmapMask(int w, int h)
{
  _w = w; _h = h;
  data = new byte[(_w*_h+1)/8];
  clearAll();
}

TBitmapMask::~TBitmapMask()
{
  delete[] data;
}

//! All pixels will be drawn.
void TBitmapMask::clearAll()
{
  memset(data, 255, (_w*_h+1)/8);
}

/**
 * When <I>paint_pixel</I> is true the pixel at position (x,y) is
 * visible.
 */
void TBitmapMask::setPixel(int x, int y, bool paint_pixel)
{
  if (x<0 || x>=_w || y<0 || y>=_h)
    return;
  int p = x + y * _w;
  int s = p & 7;
  byte bm = 1<<s;
  byte *ptr = data + (p/8);
  if (paint_pixel) {
    *ptr |= bm;
  } else {
    *ptr &= (bm^255);
//    cout << "clearing mask at " << x << "," << y << endl;
//    cout << "   " << bm << ":" << (bm^255) << endl;
  }
}

/**
 * Returns `true' when the pixel at position (x,y) is visible.
 */
bool TBitmapMask::getPixel(int x, int y) const
{
  if (x<0 || x>=_w || y<0 || y>=_h)
    return true;
  int p = x + y * _w;
  int s = p & 7;
  byte bm = 1<<s;
  byte *ptr = data + (p/8);
  return (*ptr & bm);
}

// pGetColor
//---------------------------------------------------------------------------- 
TRGB& TBitmap::pGetColor(int x,int y)
{
  assert(color);
  assert(x>=0 && x<width && y>=0 && y<height);
  if (index)
    return color[index[x+y*width]];
  else
    return color[x+y*width];
}

TRGB& TBitmap::pGetColor(int adr)
{
  assert(color);
  assert(adr>=0 && adr<width*height);
  if (index)
    return color[index[adr]];
  else
    return color[adr];
}

// Update
//---------------------------------------------------------------------------- 
void TBitmap::update()
{
  copy_bitmap_to_pixmap_and_delete_it();
}

// copy bitmap to pixmap and delete it
//----------------------------------------------------------------------------
void TBitmap::copy_bitmap_to_pixmap_and_delete_it()
{
  if (!modified || !color)
    return;

#ifdef __X11__
  // create pixmap with the correct size
  //-------------------------------------
  if (pixmap && (pix_width!=width*zoom || pix_height!=width*height)) {
    XFreePixmap(x11display, pixmap);
    pixmap = 0;
  }

  if(!pixmap) {
    pixmap = XCreatePixmap(
        x11display,
        RootWindow(x11display, x11screen),
        width*zoom,height*zoom,
        DefaultDepth(x11display, x11screen)
      );
  }

  // create image
  //--------------
  XImage *img;
  img = XCreateImage(
    x11display,
    DefaultVisual(x11display,  DefaultScreen(x11display)),
    DefaultDepth(x11display, DefaultScreen(x11display)),
    ZPixmap,
    0,
    NULL,
    width*zoom,height*zoom,
    32,
    0
  );
  
  if (!img) {
    fprintf(stderr,"toad: (TBitmap.Update) XCreateImage failed\n");
    exit(1);
  }
  
  img->data = new char[img->bytes_per_line * height * zoom];
  
  // copy bitmap to image (zoom==1)
  //-------------------------------
  switch(dither) {
    case TBITMAP_SUBSTITUTE:
      for(int y=0; y<height; y++) {
        for(int x=0; x<width; x++) {
          XPutPixel(img,x,y,TColor::_getPixel(pGetColor(x,y)));
        }
      }
      break;
    case TBITMAP_ORDERED:
      for(int y=0; y<height; y++) {
        for(int x=0; x<width; x++) {
          XPutPixel(img,x,y,TColor::_getPixelAt(pGetColor(x,y),x,y));
        }
      }
      break;
    case TBITMAP_FLOYD_STEINBERG:
      {
        TRGB c;
        struct TRGB {int r,g,b;};
        int e,f,x,p,i,line,j,y,direction;
        TRGB ae[width+2][2],ne;
        line=0;
        direction=1;
        for(j=0; j<width+2; j++)
        {
          ae[j][0].r = 0;
          ae[j][0].g = 0;
          ae[j][0].b = 0;
          ae[j][1].r = 0;
          ae[j][1].g = 0;
          ae[j][1].b = 0;
        }
        x=1;
        for(y=0; y<height; y++)
        {
          ne.r=0; ne.g=0; ne.b=0;
          line ^= 1;
          p = x-1 + y*width;
          for(j=0; j<width; j++)
          {
/*
            #define CALC_FSC(color)\
              f = pGetColor(p).color + (ae[x][line^1].color+ne.color)/16;\
              ae[x][line^1].color = 0;\
              i = (f+32)&(~63); if (i>0) i--;\
              if (f<0) f=0; else if (f>255) f=255;\
              c.color = f;\
              e = f - i;\
              i = e * 2;\
              ae[x+direction][line].color += e;\
              e+=i;\
              ae[x-direction][line].color += e;\
              e+=i;\
              ae[x][line].color           += e;\
              e+=i;\
              ne.color =                     e;
            CALC_FSC(r);
            CALC_FSC(g);
            CALC_FSC(b);
            #undef CALC_FSC
*/
              f = pGetColor(p).r + (ae[x][line^1].r+ne.r)/16;\
              ae[x][line^1].r = 0;\
              i = (f+32)&(~63); if (i>0) i--;\
              if (f<0) f=0; else if (f>255) f=255;\
              c.r = f;\
              e = f - i;\
              i = e * 2;\
              ae[x+direction][line].r += e;\
              e+=i;\
              ae[x-direction][line].r += e;\
              e+=i;\
              ae[x][line].r           += e;\
              e+=i;\
              ne.r =                     e;

              f = pGetColor(p).g + (ae[x][line^1].g+ne.g)/16;\
              ae[x][line^1].g = 0;\
              i = (f+32)&(~63); if (i>0) i--;\
              if (f<0) f=0; else if (f>255) f=255;\
              c.g = f;\
              e = f - i;\
              i = e * 2;\
              ae[x+direction][line].g += e;\
              e+=i;\
              ae[x-direction][line].g += e;\
              e+=i;\
              ae[x][line].g           += e;\
              e+=i;\
              ne.g =                     e;

              f = pGetColor(p).b + (ae[x][line^1].b+ne.b)/16;\
              ae[x][line^1].b = 0;\
              i = (f+32)&(~63); if (i>0) i--;\
              if (f<0) f=0; else if (f>255) f=255;\
              c.b = f;\
              e = f - i;\
              i = e * 2;\
              ae[x+direction][line].b += e;\
              e+=i;\
              ae[x-direction][line].b += e;\
              e+=i;\
              ae[x][line].b           += e;\
              e+=i;\
              ne.b =                     e;
            XPutPixel(img,x-1,y,TColor::_getPixel(c));
            x+=direction;
            p+=direction;
          }
          x-=direction;
          direction = direction>0 ? -1 : 1;
        }
      }
      break;
  }
  
  XPutImage(x11display,pixmap,DefaultGC(x11display, DefaultScreen(x11display)),img,
    0,0,
    0,0,
    (int)width*zoom,(int)height*zoom);
#endif
  // free memory
  //-------------
  delete[] color; 
  color=NULL;
  if (index) {
    delete index; 
    index=NULL;
  }

#ifdef __X11__
  delete[] img->data;
  img->data=NULL;
  XFree(img); 
#endif
}

// DrawBitmap
//----------------------------------------------------------------------------
void TBitmap::drawBitmap(const TPen*, int,int, int,int,int,int, int) const
{
  printf("toad: TBitmap::DrawBitmap(TPen*,int,int,int,int,int,int,int) not implemented\n");
  return;
}

void TBitmap::drawBitmap(const TPen *pen, int x, int y, int ax, int ay, int aw, int ah) const
{
#ifdef __X11__
//  cout << __PRETTY_FUNCTION__ << endl;
  switch(mode) {
    case TBITMAP_SHOW:
      const_cast<TBitmap*>(this)->copy_bitmap_to_pixmap_and_delete_it();
      if (pixmap) {
        if (mask) {
          XSetClipMask(x11display, pen->o_gc, mask);
          XSetClipOrigin(x11display, pen->o_gc, x-ax,y);
        }
        XCopyArea(
          x11display,
          pixmap,
          pen->x11drawable,
          pen->o_gc,
          ax,ay,aw,ah,
          x,y
        );
        if (mask) {
          XSetClipMask(x11display, pen->o_gc, None);
        }
      }
      break;
    default:
      cerr << "TBitmap::DrawBitmap: drawing mode not implemented" << endl;
  }
#endif
}

void TBitmap::drawBitmap(const TPen *pen, int x,int y) const
{
#ifdef __X11__
//  cout << __PRETTY_FUNCTION__ << endl;
  XImage *img;
  
  // TBitmap is a very old class and this dirty cast will remain until
  // i rewrote most of it... but i guess the compiler will still produce
  // correct code when optimizing this. [MAH]
  TBitmap *t = const_cast<TBitmap*>(this);
  
  switch(mode) {
    // server side bitmap, just copy the whole stuff
    //-----------------------------------------------
    case TBITMAP_SHOW:
      t->copy_bitmap_to_pixmap_and_delete_it();
      if (pixmap) {
        if (mask) {
          XSetClipMask(x11display, pen->o_gc, mask);
          XSetClipOrigin(x11display, pen->o_gc, x,y);
        }
        XCopyArea(
          x11display,
          pixmap,
          pen->x11drawable,
          pen->o_gc,
          0,0,width*zoom,height*zoom,
          x,y
        );
        if (mask) {
          XSetClipMask(x11display, pen->o_gc, None);
        }
      }
      break;

    // client side bitmap, copy the smallest area necessary
    //------------------------------------------------------
    case TBITMAP_EDIT:
      TRectangle r;
      if (pen->region) {
        // create an image line
        //----------------------
        img = XCreateImage(
          x11display,
          DefaultVisual(x11display,  DefaultScreen(x11display)),
          DefaultDepth(x11display, DefaultScreen(x11display)),
          ZPixmap,
          0,
          NULL,
          width*zoom,1,
          32,
          0
        );
        if (!img) {
          fprintf(stderr,"toad: (TBitmap.DrawImage) XCreateImage failed\n");
          exit(1);
        }
        img->data = new char[img->bytes_per_line];

        // get intersection of bitmap and update region
        //----------------------------------------------
        TRegion rgn;
        rgn|=TRectangle(x,y,width*zoom,height*zoom);
        rgn&=*pen->region;

        // copy bitmap to window
        //-----------------------       

        long n = 0;
        while(rgn.getRect(n++,&r)) {
          for (int y=r.y; y<r.y+r.h; y++) {
            int line_buffer[r.w];
            t->pCopyToLine(r.x, r.x+r.w-1, y, line_buffer);
            for(int i=0; i<r.w; i++)
              XPutPixel(img,i,0,line_buffer[i]);
            XPutImage(x11display,pen->x11drawable,pen->o_gc,img,0,0,r.x,y,r.w,1);
          }
        }
        
        // destroy the image line
        //------------------------
        delete[] img->data;
        img->data=NULL;
        XFree(img); 

        t->modified = false;
      } else {
        img = XCreateImage(
          x11display,
          DefaultVisual(x11display,  DefaultScreen(x11display)),
          DefaultDepth(x11display, DefaultScreen(x11display)),
          ZPixmap,
          0,
          NULL,
          width*zoom,1,
          32,
          0
        );
        if (!img) {
          fprintf(stderr,"toad: (TBitmap.DrawImage) XCreateImage failed\n");
          exit(1);
        }
        img->data = new char[img->bytes_per_line];
        for (int y2=0; y2<zoom*height; y2++) {
          int line_buffer[width*zoom];
          t->pCopyToLine(0, width*zoom-1, y2, line_buffer);
          for(int i=0; i<width*zoom; i++)
            XPutPixel(img,i,0,line_buffer[i]);
          XPutImage(x11display,pen->x11drawable,pen->o_gc,img,0,0, x, y+y2, width*zoom,1);
        }
        delete[] img->data;
        img->data=NULL;
        XFree(img); 
        t->modified = false;
      }
      break;
  }
#endif
}

/*
void TBitmap::SetMode(EBitmapMode)
{
  mode = m;
}
*/

// SetZoom
//----------------------------------------------------------------------------
void TBitmap::setZoom(int z)
{
  zoom=z;
}

// SetDither
//----------------------------------------------------------------------------
void TBitmap::setDither(EBitmapDither d)
{
#ifdef __X11__
  dither=TColor::_shouldNotDither() ? TBITMAP_SUBSTITUTE : d;
#endif
}

/**
 * Copy a single line from the bitmap to an 1d-array of pixels.
 */
void TBitmap::pCopyToLine(int x1,int x2,int y,int *line)
{
#ifdef __X11__
  if (zoom==1) {
    switch(dither) {
      case TBITMAP_SUBSTITUTE:
        for(int x=x1; x<=x2; x++) {
          line[x-x1]=TColor::_getPixel(pGetColor(x,y));
        }
        break;

      case TBITMAP_ORDERED:
      case TBITMAP_FLOYD_STEINBERG:
        for(int x=x1; x<=x2; x++) {
          line[x-x1]=TColor::_getPixelAt(pGetColor(x,y),x,y);
        }
        break;
    }
  } else {
    switch(dither) {
      case TBITMAP_SUBSTITUTE:
        for(int x=x1; x<=x2; x++) {
          line[x-x1]=TColor::_getPixel(pGetColor(x/zoom,y/zoom));
        }
        break;

      case TBITMAP_FLOYD_STEINBERG:
      case TBITMAP_ORDERED:
        for(int x=x1; x<=x2; x++) {
          line[x-x1]=TColor::_getPixelAt(pGetColor(x/zoom,y/zoom),x,y);
        }
        break;
    }
  }
#endif
}

void TBitmap::setPixel(int x,int y,short r,short g,short b)
{
  if (!color) return;
  if (index) {
    cerr << "toad: warning: TBitmap::setPixel isn't implemented for indexed mode" << endl;
    return;
  }

  if (x<0 || x>=width || y<0 || y>=height)
    return;

  TRGB *p = color + x+y*width;
  p->r=r;
  p->g=g;
  p->b=b;
  modified = true;
}

bool TBitmap::getPixel(int x,int y,short* r,short *g,short *b)
{
  if (!color || x<0 || x>=width || y<0 || y>=height) 
    return false;
  TRGB &p = pGetColor(x,y);
  *r=p.r;
  *g=p.g;
  *b=p.b;
  return true;
}

bool TBitmap::getPixel(int x,int y,TRGB *c)
{
  if (!color || x<0 || x>=width || y<0 || y>=height) 
    return false;
  *c = pGetColor(x,y);
  return true;
}

// Load
//----------------------------------------------------------------------------

struct TFree
{
  char *b;
  TFree(char *p) { b = p; }
  ~TFree() { free(b); }
};

/**
 * Load a bitmap file.<BR>
 * Throws an exception when an error occurs.
 */
void TBitmap::load(const string& url)
{
  if (!filter_list) {
    THROW("No filter available for bitmap \"" << url << "\".");
  }

  // read file into istrstream
  //----------------------------
  iurlstream is(url);
  char* buffer = NULL;
  unsigned size = 0;
  while(is) {
    unsigned pos = size;
    size += 8192;
    buffer = (char*) realloc(buffer, size);
    is.read(buffer+pos, 8192);
  }
  size-=8192;
  size+=is.gcount();
  TFree free_buffer(buffer);
  try {
#if 1
    #warning "use string in code above"
    string s(buffer, size);
    istringstream ds(s);
#else
    istrstream ds(buffer, size);
#endif
    load(ds);
  }
  catch (exception &e) {
    cerr << "warning: caught exception in bitmap filter "
         << e.what() << endl;
  }
}

void TBitmap::load(istream &ds)
{
  TBitmapFilter *filter, *last_filter;
  filter = filter_list;
    
  TBitmapFilter::EResult result;
  
  if (!ds) {
    throw runtime_error("file not found");
  }

  while(filter) {
    filter->deleteBuffer();
    try {
      ds.clear();     // clear old state
      ds.seekg(0);    // go to position 0
      result=filter->load(ds);
    } catch (exception &e) {
      cerr << "warning: caught exception in bitmap filter "
           << filter->getName() << ": " << e.what() << endl;
      result = TBitmapFilter::WRONG;
    }
    if (result==TBitmapFilter::OK) {
      // success: remove old bitmap
      //----------------------------
      if (color) delete[] color;
      if (index) delete[] index;
      color = NULL;
      index = NULL;

      // get size, color and index buffer from filter
      //----------------------------------------------
      filter->getBuffer(&width, &height, &color, &index);
      filter->deleteBuffer();
      modified = true;

// cout << __FILE__ << ":" << __LINE__ << " color=" << color << endl;

      if (mode==TBITMAP_EDIT && index) {
        unsigned size=width*height;
        TRGB *tc, *p;
        tc = p = new TRGB[size];
        unsigned char *ic = index;
        for(unsigned i=0; i<size; i++)
          *(tc++)=color[*(ic++)];
        delete[] index;
        delete[] color;
        index = NULL;
        color = p;
      }
      return;
    } else {
      filter->deleteBuffer();
      if (result==TBitmapFilter::ERROR) {
        throw runtime_error(filter->errortxt);
      }
    }
    last_filter = filter;
    filter = filter->next;
  }
  throw runtime_error("No filter available for bitmap graphic.");
}

// Save
//----------------------------------------------------------------------------
/**
 * Saves a bitmap file.
 *
 * This method will change in the future as it should be possible to
 * specify a graphic format and parameters for the graphic format.
 */
bool TBitmap::save(const string &url, void *flt)
{
  if (!color || height==0 || width==0)
    return true;

  ourlstream os(url);
  return save(os, flt);
}

bool TBitmap::save(ostream &os, void *flt)
{
  TBitmapFilter *filter;
  if (flt) {
    filter = static_cast<TBitmapFilter*>(flt);
  } else {
    filter = filter_list;
  }

  filter->setBuffer(width,height,color,index);
  filter->editSpecific();
  bool result = filter->save(os);
  filter->deleteBuffer();
  if (!result)
    throw runtime_error(filter->errortxt);
  return result;
}
