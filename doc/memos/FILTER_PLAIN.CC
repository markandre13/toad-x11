/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for X-Windows
 * Copyright (C) 1996,97 by Mark-André Hopf
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <cstdio>
#include <cstring>

#include <toad/toad.hh>
#include <toad/bitmap.hh>
#include <toad/bitmapfilter.hh>
#include <toad/filter_png.hh>

int TFilterPNG::EditSpecific()
{
	return IDOK;
}


// Load GIF Picture
//---------------------------------------------------------------------------
TFilterPNG::EResult TFilterGIF::Load(TFile &file)
{
	CreateBuffer(__width__,__height__,TBITMAP_INDEXED || TBITMAP_TRUECOLOR);

	// indexed
	//---------
	for(int i=0; i<256; i++)
		SetIndexColor(i, __color__);
		
	for(int x=0; x<__width__; x++)
		for(int y=0; y<__height__; y++)
			SetIndexPixel(x,y,__index__);
	
	// true color
	//------------
	for(int x=0; x<__width__; x++)
		for(int y=0; y<__height__; y++)
			SetColorPixel(x,y,__color__);
			
	return OK;	
}

bool TFilterPNG::Save(TFile &file)
{
	// indexed
	//---------
	if (!ConvertToIndexed(&palette_size))
	 	return false;
	 	
	for(int i=0;i<256;i++)
		file << GetIndexColor(i,&c);
		
	for(int x=0; x<w; x++)
		for(int y=0; y<h; y++)
			file << GetIndexPixel(x,y);

	// true color
	//------------
	for(int x=0; x<w; x++)
		for(int y=0; y<h; y++)
			file << GetColorPixel(x,y,&c);
}
