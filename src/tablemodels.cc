/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for X-Windows
 * Copyright (C) 1996-2004 by Mark-André Hopf <mhopf@mark13.org>
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
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, 
 * MA  02111-1307,  USA
 */

#include <toad/tablemodels.hh>

using namespace toad;

/**
 * \ingroup table
 * \class toad::TAbstractTableModel
 *
 * This class provides the basic interface between the applications data
 * and the TOAD toolkit. It should be subclassed via the generic class
 * GAbstractTableModel.
 *
 * \sa GAbstractTableModel
 */

int
TAbstractTableModel::getCols()
{
  return 1;
}

/**
 * \ingroup table
 * \class toad::GAbstractTableModel
 *
 * This interface can be subclassed to provide an interface between
 * you applications data and the TOAD toolkit.
 *
 * This generic class uses one type only for all cells in the table. To
 * handle tables with different types per column, per row or even a different
 * type in every field, one has to write an adapter class which might
 * look like this:
 *
 * \code
 * class TMyData
 * {
 *   enum TType { 
 *     STRING, NUMBER, BITMAP, DATE, CURRENCY, EXPRESSION 
 *   } type;
 *   union {
 *     string string;
 *     double number;
 *     TBitmap bitmap;
 *     time_t date;
 *     long currency;
 *     string expression;
 *   } data;
 * }
 * @endcode
 */

