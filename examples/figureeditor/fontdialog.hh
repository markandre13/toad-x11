/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for X-Windows
 * Copyright (C) 1996-2005 by Mark-André Hopf <mhopf@mark13.org>
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

#ifndef _TOAD_FONTDIALOG_HH
#define _TOAD_FONTDIALOG_HH

#include <toad/dialog.hh>
#include <toad/textfield.hh>
#include <toad/table.hh>
#include <toad/integermodel.hh>

namespace toad {

class TFontDialog:
  public TDialog
{
    typedef TFontDialog This;
    typedef TDialog super;
    
    class TFontStyles;
    class TFontStyleAdapter;
    
  public:
    TFontDialog(TWindow *parent, const string &title);
    ~TFontDialog();
    
    const string& getFont() const { return font_name; }
    void setFont(const string &name);
    unsigned getResult() const { return result; }
    
    unsigned getFontSize() const { return font_size; }
    void setFontSize(unsigned size) { font_size = size; }
    
  protected:
    TTable *family, *style, *size;
    TTextField *example;

    string family_name;
    string font_name;
    TIntegerModel font_size;

    TFontStyles *font_styles;
    TFontStyleAdapter *style_rndr;
    
    void familySelected();
    void styleSelected();
    void sizeSelected();
    void updateFont();
    void showFontTable();    
    void button(unsigned);
    unsigned result;
};

} // namespace toad

#endif
