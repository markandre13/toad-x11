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

#include <toad/layout.hh>
#include <toad/window.hh>
#include <toad/textarea.hh>
#include <toad/pushbutton.hh>
#include <toad/io/urlstream.hh>

#include <sstream>

/**
 * \class toad::TLayout
 *
 * TLayout is the base class for layout managers, which can be plugged
 * into all TWindow based objects.
 *
 * \sa toad::TDialogLayout, toad::TTabbedLayout, toad::TFormLayout
 * \sa toad::TWindow::loadLayout, toad::TWindow::setLayout.
 *
 * \todo
 *   don't destroy the control window when the dialog editor is just
 *   deactivated. the generic text layout editor is loosing all its data
 *   at this moment
 */

namespace toad {

/**
 * \class TLayoutEditGeneric
 *
 * A simple layout editor to edit the layout as text.
 *
 * \todo
 *   \li
 *     TEditLayout: show error messages for 'restore'
 *   \li
 *     Show when the layout was modified
 *   \li
 *     Load/Save with different filenames
 */
class TLayoutEditGeneric:
  public TLayoutEditor
{
    TWindow *window;    // window to be edited
    TLayout *layout;    // layout to be edited
    PTextModel text;

  public:
    TLayoutEditGeneric(TWindow*, const string&, TLayout*, TWindow *forWindow);
  protected:
    void fetch();
    void apply();
};

} // namespace toad

using namespace toad;

//-----------------------------------------------------------------

TLayoutEditor::TLayoutEditor(TWindow *parent, const string &title):
  super(parent, title)
{
}

/**
 * When the layout editor has to handle events in the window being edited,
 * it should return a filter here or NULL if otherwise.
 */
TEventFilter *
TLayoutEditor::getFilter()
{
  return 0;
}

//-----------------------------------------------------------------

TLayoutEditGeneric::TLayoutEditGeneric(TWindow *parent,
                                       const string &title,
                                       TLayout *layout,
                                       TWindow *forWindow)
  :TLayoutEditor(parent, title)
{
  int x, y;
  
  this->layout = layout;
  this->window = forWindow;
  
  text = new TTextModel();

  setBackground(TColor::DIALOG);
  
  TTextArea *ta;
  TPushButton *pb;
  
  x=5; y=5;
  
  ta = new TTextArea(this, "layout", text);
  ta->setShape(x, y, ta->getPreferences()->getFont()->getTextWidth("x")*60, 
                     ta->getPreferences()->getFont()->getHeight()*25);
  ta->getPreferences()->tabwidth=2;
  
  y+=ta->getHeight()+5;
  
  pb = new TPushButton(this, "Fetch");
  pb->setShape(x,y,80,25);
  connect(pb->sigActivate, this, &TLayoutEditGeneric::fetch);
  
  pb = new TPushButton(this, "Apply");
  pb->setShape(x+80+5,y,80,25);
  connect(pb->sigActivate, this, &TLayoutEditGeneric::apply);

  y+=25+5;
  
  setSize(5+ta->getWidth()+5, y);
  fetch();
}

void
TLayoutEditGeneric::fetch()
{
  ostringstream os;
  TOutObjectStream oo(&os);
  oo.store(layout);
  text->setValue(os.str());
}

void
TLayoutEditGeneric::apply()
{
  istringstream is(text->getValue());
  TInObjectStream io(&is);
  TSerializable *s = io.restore();
  if (s) {
    TLayout *layout2 = dynamic_cast<TLayout*>(s);
    if (layout2) {
      layout = layout2;
      layout->setModified(true);
      window->setLayout(layout);
    } else {
      delete s;
    }
  }
}


//-----------------------------------------------------------------

TLayout::TLayout()
{
  window = NULL;
  modified = false;
}

TLayout::TLayout(const TLayout &)
{
  window = NULL;
  modified = false;
}

void
TLayout::toFile()
{
  if (isModified() && filename.size()!=0) {
    try {
      cerr << "storing dialog layout in '" << filename << "'" << endl;
      ourlstream out(filename.c_str());
      out << "// this is a generated file" << endl;
      TOutObjectStream oout(&out);
      oout.store(this);
      setModified(false);
    }
    catch(exception &e) {
      cerr << "TLayout::toFile: exception while storing dialog layout: " << e.what() << endl;
    }
  }
}

/**
 * Arrange the children of window 'where' according to the
 * layout.
 */
void
TLayout::arrange()
{
}

/**
 * Some layouts may contain additional graphic besides the
 * layout of the children.
 */
void
TLayout::paint()
{
}

bool
TLayout::restore(TInObjectStream &in)
{
  return super::restore(in);
}

/**
 * Create a new layout editor control for this layout.
 *
 * The layout editor returned by TLayout is a simple text editor to
 * manipulate the serialized layout. Other classes may provide graphical
 * editors, for example like TDialogLayout.
 *
 * \param inWindow   Parent for the new window.
 * \param forWindow  Window to be edited.
 * \return           A layout editor.
 */
TLayoutEditor *
TLayout::createEditor(TWindow *inWindow, TWindow *forWindow)
{
  return new TLayoutEditGeneric(inWindow, "TLayout.editor", this, forWindow);
}
