/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2006 by Mark-André Hopf <mhopf@mark13.org>
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

/*
  rules for closing tooltips:
  - when the pointer leaves the window
  - when a mouse buttons is pressed
  - when a keyboard button is pressed
*/

#include <toad/toad.hh>
#include <toad/simpletimer.hh>

#include <map>

using namespace toad;

// currently tooltips are only one line of text
//----------------------------------------------
typedef map<TWindow*,string> TTextMap;
static TTextMap textmap;

class TToolTipTimer:
  public TSimpleTimer
{
  public:
    void tick();
};

class TToolTipWindow:
  public TWindow
{
  public:
    TToolTipWindow();
    void paint();
};

static TToolTipTimer timer;
static bool visible = false;
static TWindow *where;
static TToolTipWindow *tooltip_window = NULL;

static string current_text = "This is a tooltip";

/**
 * Set Tooltip text.
 *
 * A tooltip is a little yellow box with a text which appears when the
 * mouse pointer stayed for some time inside a window.
 *
 * Usually this text gives a short description (tip) about what the 
 * window (tool) is for.
 *
 * \param text  The text to be displayed.
 */
void TWindow::setToolTip(const string &text)
{
  if (text.size()!=0) {
    textmap[this] = text;
    _bToolTipAvailable = true;
  } else {
    if (_bToolTipAvailable) {
      TTextMap::iterator p = textmap.find(this);
      textmap.erase(p);
    }
    _bToolTipAvailable = false;
  }
}

/**
 * This method is called from the message loop when a window has the
 * _bToolTipAvailableFlag set.
 */
void 
TOADBase::toolTipOpen(TWindow *wnd)
{
  where = wnd;
  timer.startTimer(1, 0, true);   // 1s delay
}

void TToolTipTimer::tick()
{
  stopTimer();
  ::visible = true;
  if (tooltip_window==NULL)
    tooltip_window = new TToolTipWindow();

  current_text = textmap[where];
  tooltip_window->setSize(TOADBase::getDefaultFont().getTextWidth(current_text)+4,
                          TOADBase::getDefaultFont().getHeight()+4 );
  TOADBase::placeWindow(tooltip_window, PLACE_TOOLTIP, where);

  if (!tooltip_window->isRealized())  {
    tooltip_window->createWindow();
  } else {
    tooltip_window->setMapped(true);
    tooltip_window->raiseWindow();
  }
}

/**
 * This method is called from the message loop when a reason occured
 * to close an open tooltip
 */
void TOADBase::toolTipClose()
{
  timer.stopTimer();
  if (::visible) {
    ::visible = false;
    if (tooltip_window)
      tooltip_window->setMapped(false);
  }
}

TToolTipWindow::TToolTipWindow()
  :TWindow(NULL, "TToolTipWindow")
{
  flagPopup = true; // no frame
  bParentlessAssistant = true; // not important enough to keep app running
  setBackground(255,255,192);
}

void TToolTipWindow::paint()
{
  TPen pen(this);
  pen.drawString(2,2, ::current_text);
}
