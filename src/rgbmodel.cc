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

#include <toad/rgbmodel.hh>

using namespace toad;

TRGBModel::TRGBModel()
{
  _init();
}

void
TRGBModel::_init()
{
  lock = false;
  r.setRangeProperties(0, 0, 0, 255);
  g.setRangeProperties(0, 0, 0, 255);
  b.setRangeProperties(0, 0, 0, 255);
  connect(r.sigChanged, this, &TRGBModel::changed);
  connect(g.sigChanged, this, &TRGBModel::changed);
  connect(b.sigChanged, this, &TRGBModel::changed);
}

void
TRGBModel::changed()
{
  if (lock)
    return;
  sigChanged();
}

namespace {

class TRGBTextModel:
  public TTextModel
{
    TRGBModel *model;
  public:
    TRGBTextModel(TRGBModel *m) {
      model = m;
      if (model) {
        connect(model->sigChanged, this, &TRGBTextModel::slaveChanged);
        slaveChanged();
      }
    }
    ~TRGBTextModel() {
      if (model)
        disconnect(model->sigChanged, this);
    }
    int filter(int c) {
      if (c=='\n') {
        masterChanged();
        return 0;
      }
      if ( (c>='0' && c<='9') || 
           (c>='a' && c<='f') ||
           (c>='A' && c<='F') ||
           (c=='#') )
        return c;
      return 0;
    }
    void focus(bool b) {
      if (!b)
        masterChanged();
    }  
    void masterChanged()
    {
      int r, g, b;
      sscanf(data.c_str(), "#%02x%02x%02x", &r, &g, &b);
      model->set(r, g, b);
    }
    void slaveChanged()
    {
      char buffer[8];
#ifndef __WIN32__
      snprintf(buffer, 8, "#%02x%02x%02x", (int)model->r, 
                                           (int)model->g,
                                           (int)model->b);
#else
      sprintf(buffer, "#%02x%02x%02x", (int)model->r, 
                                       (int)model->g,
                                       (int)model->b);
#endif
      setValue(buffer);
    }
};

} // namespace

TTextModel *
toad::createTextModel(TRGBModel * m)
{
  return new TRGBTextModel(m);
}
