#include <toad/colorselector.hh>
#include <toad/figureeditor.hh>
#include <toad/colordialog.hh>
#include <toad/pushbutton.hh>

using namespace toad;

#if 0
int
main(int argc, char **argv, char **envv)
{
  toad::initialize(argc, argv, envv);
  {
    TColorSelector window(0, "colorselector");
    toad::mainLoop();
  }
  toad::terminate();
  return 0;
}
#endif

TColorSelector::TColorSelector(TWindow *parent,
                               const string &title,
                               TFigureEditor *gedit):
  super(parent, title)
{
  this->gedit = gedit;
  setSize(64, 32);

  w2 = getWidth() - 16;
  h = getHeight();
  border = min(h,w2) / 4 + 1;

  setBorder(0);
  setBackground(TColor::DIALOG);
  filled = false; 
  linecolor.set(0,0,0);
  fillcolor.set(255,255,255);

  pb1 = new TPushButton(this, "palette");
  pb1->bNoFocus = true;
  pb1->loadBitmap("memory://toad/colorpalette.png");
  pb1->setToolTip("Open Color Palette");
  pb1->setShape(w2,0,getWidth()-w2,getHeight()/2);
  connect(pb1->sigActivate, this, &TColorSelector::openColorPalette);

  pb2 = new TPushButton(this, "coloreditor");
  pb2->bNoFocus = true;
  pb2->loadBitmap("memory://toad/colordialog.png");
  pb2->setToolTip("Open Color Editor");
  pb2->setShape(w2,getHeight()/2,getWidth()-w2,getHeight()/2);
  connect(pb2->sigActivate, this, &TColorSelector::openColorDialog);

  ds = new TDropSiteColor(this, TRectangle(0,0,w2,h));
  connect_value(ds->sigDrop,
                this, &TColorSelector::dropColor, ds);
}

void
TColorSelector::resize()
{
  w2 = getWidth() - 16;
  h = getHeight();
  border = min(h,w2) / 4 + 1;
  pb1->setShape(w2,0,getWidth()-w2,getHeight()/2);
  pb2->setShape(w2,getHeight()/2,getWidth()-w2,getHeight()/2);
  ds->setShape(0,0,w2,h);
}
 
void
TColorSelector::paint()
{
  TPen pen(this);
  
  pen.setColor(linecolor);
  pen.fillRectanglePC(0, 0, w2, h);

  if (filled) {
    pen.setColor(fillcolor);
    pen.fillRectanglePC(border, border, w2-border*2, h-border*2);
  } else {
    pen.setColor(255,255,255);
    pen.fillRectanglePC(border, border, w2-border*2, h-border*2);
    pen.setColor(0,0,0);
    pen.drawLine(border+2, border+2, w2-border-2, h-border-2);
    pen.drawLine(border+2, h-border-2, w2-border-2, border+2);
  }
  pen.draw3DRectangle(border, border, w2-border*2, h-border*2, filled);
}  
   
void
TColorSelector::mouseLDown(int x, int y, unsigned modifier)
{
  if (x<border ||
      y<border ||
      x>w2-border ||
      y>h-border)  
  {
/*
    TColorDialog ce(this, "Line Color", &linecolor);
    ce.doModalLoop();
    invalidateWindow();
*/
  } else {
/*
    TColorDialog ce(this, "Fill Color", &fillcolor);
    TCheckBox *fill = new TCheckBox(&ce, "Filled"); 
    fill->setShape(x=8+256+8+16+8+12, 228, 80, 32); 
    fill->getModel()->setValue(true);
    ce.doModalLoop();
    if (ce.apply)
      filled = fill->getModel()->getValue();
*/
    filled = !filled;
    invalidateWindow();
  }

  if (!gedit)
    return;
  gedit->setLineColor(linecolor);
  if (filled)
    gedit->setFillColor(fillcolor);
  else
    gedit->unsetFillColor();
}

void
TColorSelector::mouseMDown(int x, int y, unsigned modifier)
{
  if (x<border ||
      y<border ||
      x>w2-border ||
      y>h-border)  
  {
    startDrag(new TDnDColor(linecolor), modifier);
  } else {
    startDrag(new TDnDColor(fillcolor), modifier);
  }
}

void
TColorSelector::dropColor(const PDnDColor &drop)
{
  if (drop->x<border ||
      drop->y<border ||
      drop->x>w2-border ||
      drop->y>h-border)  
  {
    linecolor.set(drop->rgb.r, drop->rgb.g, drop->rgb.b);
  } else {
    fillcolor.set(drop->rgb.r, drop->rgb.g, drop->rgb.b);
  }
  invalidateWindow();

  if (!gedit)
    return;
  gedit->setLineColor(linecolor);
  if (filled)
    gedit->setFillColor(fillcolor);
  else
    gedit->unsetFillColor();
  
}

void
TColorSelector::openColorDialog()
{
#if 0
  TColorDialog dlg(this, "Color Editor");
  dlg.doModalLoop();
#else
  TWindow *dlg = new TColorDialog(0, "Color Editor");
  dlg->createWindow();
#endif
}

void
TColorSelector::openColorPalette()
{
}
