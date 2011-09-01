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

#include "page.hh"
#include "fischland.hh"

#include <toad/pushbutton.hh>
#include <toad/textfield.hh>

using namespace fischland;

static char*
number()
{
  static unsigned counter = 0;
  counter++;
  static char buffer[64];
  snprintf(buffer, sizeof(buffer), "%u", counter);
  return buffer;
}
  
TPlainSlide::TPlainSlide()
{
  name = "unnamed #";
  name += number();
  lock = false;
  show = true;
  print = true;
}

TSlideTreeModel*
TEditModel::getSlideTreeModel()
{
  if (!document)
    return 0;
  return &document->content;
}

TSlide*
TEditModel::getSlide()
{
  if (!document)
    return 0;
  int row = 0;
  if (slide.empty()) {
    cout << "warning, hack: 'slide' is empty, assuming 0" << endl;
  } else
    row = slide.getRow();
  if (row>=document->content.getRows()) {
    cout << "warning: slide is out of range" << endl;
    return 0;
  }
  return &document->content[row];
}

TLayerTreeModel*
TEditModel::getLayerTreeModel()
{
  TSlide *s = getSlide();
  if (!s)
    return 0;
  return &s->content;
}

bool
TEditModel::buildSlidePath(vector<TSlide*> *path, TSlide *slide, size_t *n)
{
  while(slide) {
    path->push_back(slide);
//    cout << "build path: slide '" << slide->name << "' of " << *n << endl;
    if (*n==0) {
//      cout << "got it! (1)" << endl;
      return true;
    }
    (*n)--;
    if (slide->down && buildSlidePath(path, slide->down, n)) {
      return true;
    }
    path->pop_back();
    slide = slide->next;
  }
  return false;
}

void
TEditModel::appendLayers(TLayer *layer, bool show, bool lock, size_t *n)
{
  while(layer) {
    if (show && layer->show) {
//      cout << "    append layer " << layer->name << " with n = " << *n << endl;
      modelpath.push_back(&layer->content);
    }
    if (!lock && !layer->lock && *n==0) {
//      cout << "    active layer " << layer->name << endl;
      figuremodel = &layer->content;
      lock = true; // when n==0, it isn't decremented anymore, so set lock
    }
    if (*n>0)
      --(*n);
    if (layer->down) {
      appendLayers(layer->down, show && layer->show, lock || layer->lock, n);
    }
    layer = layer->next;
  }
}

void
TEditModel::setDocument(TDocument *d)
{
  if (document==d)
    return;
  document = d;
  changed(DOCUMENT_CHANGED);
}

void
TEditModel::changed(EReason reason)
{
  if (lock)
    return;
  lock = true;

  switch(reason) {
    case DOCUMENT_CHANGED: {
      slide.select(0,0);
      TSlideTreeModel *nstm = getSlideTreeModel();
      if (nstm!=stm) {
        if (stm)
          disconnect(stm->sigChanged, this);
        stm = nstm;
        if (stm)
          connect(stm->sigChanged, this, &TEditModel::stmChanged);
      }
    }
    case SLIDE_CHANGED: {
      layer.select(0,0);
      TLayerTreeModel *nltm = getLayerTreeModel();
      if (nltm!=ltm) {
        if (ltm)
          disconnect(ltm->sigChanged, this);
        ltm = nltm;
        if (ltm)
          connect(ltm->sigChanged, this, &TEditModel::ltmChanged);
      }
    }
    case LAYER_CHANGED:
      modelpath.clear();
      figuremodel = 0;
      if (document) {
#if 0
        cout << endl << "build slide path: slide="
             << slide.getRow()
             << ", layer=" << layer.getRow() << endl;
#endif
        size_t n = slide.getRow();
        vector<TSlide*> path;
        buildSlidePath(&path, document->content.getRoot(), &n);
        for(vector<TSlide*>::iterator p = path.begin();
            p != path.end();
            ++p)
        {
//          cout << "  try slide " << (*p)->name << endl;
          if ((*p)->show) {
            figuremodel = 0;
            n = layer.getRow();
            appendLayers((*p)->content.getRoot(), true, 
              false, 
              &n);
          }
        }
//        cout << "TEditModel::changed: new figuremodel is " << figuremodel << endl;
      }
      break;
  }
  lock = false;
  this->reason = reason;
  sigChanged();
#if 0
cout << "begin selection fix hack" << endl;
lock = true;
  switch(reason) {
    case DOCUMENT_CHANGED:
    case SLIDE_CHANGED:
      layer.sigChanged();
    case LAYER_CHANGED:
      break;
  }
lock = false;
sigChanged();
cout << "end selection fix hack" << endl;
#endif
#if 0
lock = true;
cout << "begin selection fix hack" << endl;
  switch(reason) {
    case DOCUMENT_CHANGED:
      slide.lock();
      slide.clearSelection();
      slide.setSelection(0,0);
      slide.unlock();
    case SLIDE_CHANGED:
      layer.lock();
      layer.clearSelection();
      layer.setSelection(0,0);
      layer.unlock();
    case LAYER_CHANGED:
      break;
  }
lock = false;
  sigChanged();
cout << "end selection fix hack" << endl;
#endif
}

void
TEditModel::stmChanged()
{
  assert(stm!=0);
  switch(stm->reason) {
    case TTableModel::INSERT_ROW:
      cout << "TEditModel: a new slide was added" << endl;
      lock = true;
      slide.select(0, stm->where);
      layer.select(0, 0);
      document->content[stm->where].content.addBelow(0);
      lock = false;
      changed(SLIDE_CHANGED);
      break;
    case TTableModel::REMOVED_ROW:
      cout << "TEditModel: a slide was removed" << endl;
      lock = true;
      if (slide.getRow() >= stm->getRows()) {
        if (stm->getRows()==0) {
          slide.clear();
          layer.clear();
        } else {
          slide.select(0, stm->getRows()-1);
          layer.select(0, 0);
        }
      } else {
        layer.select(0,0);
      }
      lock = false;
      changed(SLIDE_CHANGED);
      break;
  }
}

void
TEditModel::ltmChanged()
{
  assert(ltm!=0);
  if (ltm->reason != TTableModel::INSERT_ROW)
    return;
  cout << "TEditModel: a new layer was added" << endl;
}

void      
TEditModel::appendLayer()
{
cout << "add layer to new slide" << endl;
  if (!document)
    return;
  if (slide.empty())
    return;
  if (slide.getRow() >= document->content.getRows()) {
    cout << __FILE__ << __LINE__ << ": current slide above number of slides" << endl;
    return;
  }
  if (layer.getRow() > document->content[slide.getRow()].content.getRows()) {
    cout << __FILE__ << __LINE__ << ": current layer above number of layers+1" << endl;
    return;
  }
  lock = true;
  document->content[slide.getRow()].content.addBelow(layer.getRow());
  layer.select(0, 0);
  lock = false;
  changed(LAYER_CHANGED);
cout << "added layer to new slide" << endl;
}

namespace fischland {

class TSlideTreeAdapter:
  public GTreeAdapter<TSlideTreeModel>
{
    typedef GTreeAdapter<TSlideTreeModel> super;
  public:
    TSlideTreeAdapter(TSlideTreeModel *m): super(m) {}
    size_t getCols() { return 4; }
    void tableEvent(TTableEvent &te);
    bool canDrag() const { return true; }
};

} // namespace fischland

void
TSlideTreeAdapter::tableEvent(TTableEvent &te)
{
  renderBackground(te);
  if (!model)
    return;
//cout << "draw field " << te.col << ", " << te.row << " of size " << te.w << ", " << te.h << endl;
  switch(te.col) {
    case 0:
      {
      string s;
      if (te.type == TTableEvent::KEY || te.type == TTableEvent::PAINT)
        s = (*model)[te.row].name;
      handleString(te, &s, getLeafPos(te.row));
      if (te.type == TTableEvent::KEY || te.type == TTableEvent::PAINT)
        (*model)[te.row].name = s;
      handleTree(te);
      }
      break;
    case 1:
      handleCheckBox(te, &(*model)[te.row].lock);
      break;
    case 2:
      handleCheckBox(te, &(*model)[te.row].show);
      break;
    case 3:
      handleCheckBox(te, &(*model)[te.row].print);
      break;
    default:
      TTableAdapter::tableEvent(te);
      return;
  }
  renderCursor(te);
}

namespace fischland {

class TLayerTreeAdapter:
  public GTreeAdapter<TLayerTreeModel>
{
    typedef GTreeAdapter<TLayerTreeModel> super;
  public:
    TLayerTreeAdapter(TLayerTreeModel *m): super(m) {}
    size_t getCols() { return 4; }
    void tableEvent(TTableEvent &te);
    bool canDrag() const { return true; }
};

} // namespace fischland

void
TLayerTreeAdapter::tableEvent(TTableEvent &te)
{
  if (!model)
    return;
  renderBackground(te);
//cout << "draw field " << te.col << ", " << te.row << " of size " << te.w << ", " << te.h << endl;
  switch(te.col) {
    case 0:
      {
      string s;
      if (te.type == TTableEvent::KEY || te.type == TTableEvent::PAINT)
        s = (*model)[te.row].name;
      handleString(te, &s, getLeafPos(te.row));
      if (te.type == TTableEvent::KEY || te.type == TTableEvent::PAINT)
        (*model)[te.row].name = s;
      handleTree(te);
      }
      break;
    case 1:
      handleCheckBox(te, &(*model)[te.row].lock);
      break;
    case 2:
      handleCheckBox(te, &(*model)[te.row].show);
      break;
    case 3:
      handleCheckBox(te, &(*model)[te.row].print);
      break;
    default:
      TTableAdapter::tableEvent(te);
      return;
  }
  renderCursor(te);
}

TPageDialog::TPageDialog(TWindow *parent, 
                         const string &title, 
                         TEditModel *model,
                         bool slides):
  super(parent, title)
{
  bStaticFrame = false;
  setSize(320,240);
  this->model = model;
  this->slides = slides;

//  CONNECT(model->sigChanged, this, modelChanged);
  table = new TTable(this, "pagetree");

//  sm = new TRectangleSelectionModel;

//  CONNECT(sm->sigChanged, this, selected);
//  sm->setRowColMode(TAbstractSelectionModel::WHOLE_ROW);
//  table->setSelectionModel(sm);
  
  table->selectionFollowsMouse = true;

//  table->setRenderer(pt);

  TDefaultTableHeaderRenderer *hdr = new TDefaultTableHeaderRenderer();
//  TAbstractTreeEditor *pe;
  if (slides) {
    hdr->setText(0, "Slide");
    hdr->setImage(1, RESOURCE("lock.png"));
    hdr->setImage(2, RESOURCE("show.png"));
    hdr->setImage(3, RESOURCE("print.png"));
    sm = &model->slide;
//    pe = pt.s = new TPlainSlideTreeRenderer<TSlide>(&model->document->content, sm);
  } else {
    hdr->setText(0, "Layer");
    hdr->setImage(1, RESOURCE("lock.png"));
    hdr->setImage(2, RESOURCE("show.png"));
    hdr->setImage(3, RESOURCE("print.png"));
    sm = &model->layer;
//    pe = pt.l = new TPlainSlideTreeRenderer<TLayer>(&model->document->content->content, sm);
  }
  table->setSelectionModel(sm);
  // CONNECT(sm->sigChanged, this, selected);
  sm->setRowColMode(TAbstractSelectionModel::WHOLE_ROW);
  
  tm = 0;
  TSlideTreeModel *stm = 0;
  TLayerTreeModel *ltm = 0;
  if (model && model->document) {
    if (slides) {
      if (model->document)
        stm = &model->document->content;
      tm = stm;
    } else {
      ltm = &model->document->content[model->slide.getRow()].content;
      tm = ltm;
    }
  }
  
  connect(model->sigChanged, this, &TPageDialog::editModelChanged);

  if (slides) {
    adapter.slide = new TSlideTreeAdapter(stm);
    table->setAdapter(adapter.slide);
  } else {
    adapter.layer = new TLayerTreeAdapter(ltm);
    table->setAdapter(adapter.layer);
  }
  // table->setModel(tm);
  
//  table->setAdapter(pe);
//  CONNECT(pe->sigChanged, this, rendererChanged);
  table->setColHeaderRenderer(hdr);

  // hmm, it should be the job of TTable to modify the selection after
  // changes...

  TPushButton *pb;
  pb = new TPushButton(this, "add above");
  pb->loadBitmap(RESOURCE("tree-insert-before.png"));
  CONNECT(pb->sigClicked, this, command, 0);
  pb = new TPushButton(this, "add below");
  pb->loadBitmap(RESOURCE("tree-insert-behind.png"));
  CONNECT(pb->sigClicked, this, command, 1);
  pb = new TPushButton(this, "add tree above");
  pb->loadBitmap(RESOURCE("tree-insert-above.png"));
  CONNECT(pb->sigClicked, this, command, 2);
  pb = new TPushButton(this, "add tree below");
  pb->loadBitmap(RESOURCE("tree-insert-below.png"));
  CONNECT(pb->sigClicked, this, command, 3);
  pb = new TPushButton(this, "del");
  pb->loadBitmap(RESOURCE("tree-delete.png"));
  CONNECT(pb->sigClicked, this, command, 4);
  
  loadLayout(RESOURCE("TPageDialog.atv"));
}

void
TPageDialog::editModelChanged()
{
  if (slides) {
//    cout << "TPageDialog: edit model changed slides" << endl;
    switch(model->reason) {
      case TEditModel::DOCUMENT_CHANGED:
cout << "TPageDialog::editModelChanged: set new slide tree model" << endl;
        adapter.slide->setModel(model->getSlideTreeModel());
        break;
      case TEditModel::SLIDE_CHANGED:
      case TEditModel::LAYER_CHANGED:
        break;
    }
  } else {
    switch(model->reason) {
      case TEditModel::DOCUMENT_CHANGED:
      case TEditModel::SLIDE_CHANGED:
cout << "TPageDialog::editModelChanged: set new layer tree model " 
     << model->getLayerTreeModel() << endl;
        adapter.layer->setModel(model->getLayerTreeModel());
        tm = model->getLayerTreeModel();
        break;
      case TEditModel::LAYER_CHANGED:
        break;
    }
  }
}

void
TPageDialog::command(unsigned cmd)
{
  if (!tm) {
    cout << "TPageModel::command: no model" << endl;
    return;
  }

  if (sm->empty() && !tm->empty()) {
    if (sm->empty())
      cout << "selection model is empty" << endl;
    if (!tm->empty())
      cout << "table model isn't empty" << endl;
    return;
  }

  size_t row = sm->getRow();
  switch(cmd) {
    case 0:
      row = tm->addBefore(row);
      break;
    case 1:
      row = tm->addBelow(row);
      break;
    case 2:
      row = tm->addTreeBefore(row);
      break;
    case 3:
      row = tm->addTreeBelow(row);
      break;
    case 4:
      row = tm->deleteRow(row);
      break;
    default:
      return;
  }
#if 0
  if (slides) {
    model->slide.setSelection(0, row);
    if (0<=cmd && cmd<=3) {
      if (cmd==0 || cmd==2)
        --row;
      model->appendLayer();
    }
  } else {
    model->layer.setSelection(0, row);
  }
#endif
  table->setFocus();
}

void
TDocument::store(TOutObjectStream &out) const
{
  ::store(out, "author", author);
  ::store(out, "date", date);
  ::store(out, "description", description);

  for(TSlide *slide = content.getRoot();
      slide;
      slide = slide->next)
  {
//cout << "document stores slide " << slide->name << endl;
    out.store(slide);
  }
}

bool
TDocument::restore(TInObjectStream &in)
{
  if (in.what == ATV_GROUP && in.type == "fischland::TSlide") {
    TSlide *slide = new TSlide();
    // content.push_back(slide);
    TSlide *p = content.getRoot();
    if (!p) {
      content.setRoot(slide);
    } else {
      while(p->next)
        p = p->next;
      p->next = slide;
    }
    in.setInterpreter(slide);
    return true;
  }
  if (in.what == ATV_FINISHED) {
    content.update(false);
  }
  if (
    ::restore(in, "author", &author) ||
    ::restore(in, "date", &date) ||
    ::restore(in, "description", &description) ||
    TSerializable::restore(in)
  ) return true;
  ATV_FAILED(in)
  return false;
}

void
TSlide::store(TOutObjectStream &out) const
{
//cout << "slide " << name << " stores itself" << endl;
  ::store(out, "name",    name);
  ::store(out, "comment", comment);
  ::store(out, "lock",    lock);
  ::store(out, "show",    show);
  ::store(out, "print",   print);

  for(TLayer *layer = content.getRoot();
      layer;
      layer = layer->next)
  {
//cout << "slide " << name << " stores layer " << layer->name << endl;
    out.store(layer);
  }
  
  for(TSlide *slide = down;
      slide;
      slide = slide->next)
  {
//cout << "slide " << name << " stores down slide " << slide->name << endl;
    out.store(slide);
  }
}

bool
TSlide::restore(TInObjectStream &in)
{
  if (in.what == ATV_GROUP && in.type == "fischland::TLayer") {
    TLayer *layer = new TLayer();
    // content.push_back(slide);
    TLayer *p = content.getRoot();
    if (!p) {
      content.setRoot(layer);
    } else {
      while(p->next)
        p = p->next;
      p->next = layer;
    }
    in.setInterpreter(layer);
    return true;
  }
  if (in.what == ATV_GROUP && in.type == "fischland::TSlide") {
    TSlide *slide = new TSlide();
    // content.push_back(slide);
    if (!down) {
      down = slide;
    } else {
      TSlide *p = down;
      while(p->next) {
        p = p->next;
      }
      p->next = slide;
    }
    in.setInterpreter(slide);
    return true;
  }
  if (in.what == ATV_FINISHED) {
    content.update(false);
  }
  if (
    ::restore(in, "name",    &name) ||
    ::restore(in, "comment", &comment) ||
    ::restore(in, "lock",    &lock) ||
    ::restore(in, "show",    &show) ||
    ::restore(in, "print",   &print) ||
    TSerializable::restore(in)
  ) return true;

  ATV_FAILED(in)
  return false;
}

void
TLayer::store(TOutObjectStream &out) const
{
//cout << "layer " << name << " stores itself" << endl;
  ::store(out, "name",    name);
  ::store(out, "comment", comment);
  ::store(out, "lock",    lock);
  ::store(out, "show",    show);
  ::store(out, "print",   print);

  content.store(out);

  for(TLayer *layer = down;
      layer;
      layer = layer->next)
  {
    out.store(layer);
  }
}

bool
TLayer::restore(TInObjectStream &in)
{
  if (
    content.restore(in) ||
    ::restore(in, "name",    &name) ||
    ::restore(in, "comment", &comment) ||
    ::restore(in, "lock",    &lock) ||
    ::restore(in, "show",    &show) ||
    ::restore(in, "print",   &print) ||
    TSerializable::restore(in)
  ) return true;
  ATV_FAILED(in)
  return false;
}
