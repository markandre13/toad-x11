/*
 * Fischland -- A 2D vector graphics editor
 * Copyright (C) 1999-2005 by Mark-André Hopf <mhopf@mark13.org>
 * Visit http://www.mark13.org/fischland/.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// This file defines the classes: TEditModel, TDocument, TSlide and TLayer.

#ifndef _FISCHLAND_PAGE_HH
#define _FISCHLAND_PAGE_HH 1

#include <toad/toad.hh>
#include <toad/dialog.hh>
#include <toad/treeadapter.hh>
#include <toad/io/serializable.hh>

namespace fischland {

using namespace toad;

/**
 * This class is the base class for TLayer and TSlide.
 */
struct TPlainSlide:
  public TSerializable
{
  TPlainSlide();

  string name;    // name of this slide
  string comment; // a comment for this slide/layer

  bool lock;      // don't modify this slide
  bool show;      // show this slide while editing
  bool print;     // show this slide when printing or presentation

  // the closed flag is usually stored in the tree renderer, but we also
  // want to make it persistent thus it is also available here
  bool closed;
};

// TDocument -> TSlide -> TLayer

struct TLayer:
  public TPlainSlide
{
  SERIALIZABLE_INTERFACE(fischland::, TLayer);

public:
  TLayer() {
    next = down = 0;
  }
  TFigureModel content;
  TLayer *next, *down;
};

class TLayerTreeModel:
  public GTreeModel<TLayer>
{
};

struct TSlide:
  public TPlainSlide
{
  SERIALIZABLE_INTERFACE(fischland::, TSlide);
public:
  TSlide() {
    wait = true;
    next = down = 0;
  }
  bool wait;    // wait here when flipping slides during presentation
  TLayerTreeModel content;
  TSlide *next, *down;
};

class TSlideTreeModel:
  public GTreeModel<TSlide>
{
};

struct TDocument:
  public TSerializable
{
  SERIALIZABLE_INTERFACE(fischland::, TDocument);
public:
  string author;
  string date;
  string description;
  TSlideTreeModel content;
};

// usually there's only one edit model? no!

struct TEditModel:
  public TModel
{
  // private:
    bool lock;
    TSlideTreeModel *stm;
    TLayerTreeModel *ltm;
  public:
    TEditModel() {
      document = 0;
      stm = 0;
      ltm = 0;
      lock = false;
      connect(slide.sigChanged, this, &TEditModel::changed, SLIDE_CHANGED);
      connect(layer.sigChanged, this, &TEditModel::changed, LAYER_CHANGED);
    }
    
    void setDocument(TDocument *d);
    TDocument *getDocument() const { return document; }
#if 0
    TLayer* getLayer();
    TFigureModel *getFigureModel();
    bool isModified() const;
    void setModified();
#endif    
    TSlideTreeModel* getSlideTreeModel();
    TSlide* getSlide();
    TLayerTreeModel* getLayerTreeModel();
    void appendLayer();
  
    enum EReason {
      DOCUMENT_CHANGED,
      SLIDE_CHANGED,
      LAYER_CHANGED
    } reason;
  
    TDocument *document;
    TSingleSelectionModel slide;
    TSingleSelectionModel layer;

    // all models to be drawn
    vector<TFigureModel*> modelpath;
    // the current model selected for editing
    TFigureModel *figuremodel;
    
  protected:
    void changed(EReason);
    void stmChanged();
    void ltmChanged();
    bool buildSlidePath(vector<TSlide*> *path, TSlide *slide, size_t *n);
    void appendLayers(TLayer *layers, bool show, bool lock, size_t *n);
};
typedef GSmartPointer<TEditModel> PEditModel;


class TSlideTreeAdapter;
class TLayerTreeAdapter;

class TPageDialog:
  public TDialog
{
    typedef TDialog super;
    TTreeModel *tm;
    PEditModel model;
    TTable *table;
    bool slides;
    union {
      TSlideTreeAdapter *slide;
      TLayerTreeAdapter *layer;
    } adapter;
    // TRectangleSelectionModel *sm;
    TSingleSelectionModel *sm;
  public:
    TPageDialog(TWindow *parent, const string &title, TEditModel *model, bool slides);
    ~TPageDialog() {
      // table->selectionmodel points to the editmodel, but the editmodel will be
      // destroyed before the table, thus the following ugly hack...
      table->setSelectionModel(0);
    }
    
    void editModelChanged();
    void command(unsigned);
};

} // namespace fischland

#endif
