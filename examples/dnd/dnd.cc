#include <toad/toad.hh>
#include <toad/dnd/textplain.hh>
#include <toad/dnd/color.hh>
#include <toad/dnd/image.hh>

using namespace toad;

class TMyWindow:
  public TWindow
{
  public:
    TMyWindow(TWindow*, const string&);
    
    void mouseMDown(int, int, unsigned);
};

class TMyDropSite:
  public TDropSite
{
    typedef TDropSite super;
  public:
    TMyDropSite(TWindow *p):super(p) {}
    TMyDropSite(TWindow *p, const TRectangle &r):super(p,r) {}
      
    void dropRequest(TDnDObject&);
    void drop(TDnDObject&);
};

int
main(int argc, char **argv, char **envv)
{
  toad::initialize(argc, argv, envv); {
    TMyWindow wnd(NULL, "Drag'n Drop Example");
    toad::mainLoop();
  } toad::terminate();
}

TMyWindow::TMyWindow(TWindow *p, const string& t):
  TWindow(p, t)
{
  bStaticFrame=true;
  new TMyDropSite(this, TRectangle(5,5,310,190));
}

void
TMyWindow::mouseMDown(int, int, unsigned modifier)
{
  startDrag(new TDnDTextPlain("Hello Folks!"), modifier);
}

void
TMyDropSite::dropRequest(TDnDObject &drop)
{
  drop.action=ACTION_COPY;
    
  // try to select an image or, when not available, a text
  if (TDnDImage::select(drop))
    return;
  if (TDnDTextPlain::select(drop))
    return;
  if (TDnDColor::select(drop))
    return;
    
  drop.action = ACTION_NONE;
}

void
TMyDropSite::drop(TDnDObject &drop)
{
  cout << "dropping " << drop.flatdata.size() << " bytes" << endl;
  if (drop.type)
    cout << "  type: " << drop.type->mime << endl;
  else
    cout << "  type: (unknown)" << endl;

  PDnDImage img = TDnDImage::convertData(drop);
  if (img) {
    TPen pen(parent);
    TBitmap *bmp = img->bmp;
    pen.drawBitmap(drop.x-bmp->getWidth()/2, drop.y-bmp->getHeight()/2, bmp);
    return;
  }
  
  PDnDTextPlain txt = TDnDTextPlain::convertData(drop);
  if (txt) {
    TPen pen(parent);
    pen.drawString(drop.x, drop.y, txt->text);
    return;
  }
  
  PDnDColor col = TDnDColor::convertData(drop);
  if (col) {
    TPen pen(parent);
    pen.setColor(col->rgb);
    pen.fillRectangle(5,5,310,190);
  }
}
