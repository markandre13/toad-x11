#include <toad/toad.hh>
#include <toad/bitmap.hh>

using namespace toad;

class TMyWindow: public TWindow
{
  public:
    TMyWindow(TWindow *parent, const string &title);
  protected:
    void paint();
  private:
    TBitmap bitmap;
};

TMyWindow::TMyWindow(TWindow *parent, const string &title)
  :TWindow(parent,title)
{
  try {
    bitmap.load("alien.png");
  } catch(exception &e) {
    cout << e.what() << endl;
  }
}

void 
TMyWindow::paint()
{
  cout << "paint" << endl;
  TPen pen(this);
  for(int i=0; i<320; i+=25)
    pen.drawLine(i,0, i,200);
  for(int i=0; i<200; i+=25)
    pen.drawLine(0,i, 320,i);

  pen.setColor(255,0,0);
  pen.fillRectangle(7,7,100,100);

  pen.setBitmap(&bitmap);
  pen.fillCircle(220,50,100,100);

  pen.setColor(0,255,0);
  pen.fillCircle(240,70,60,60);

  pen.setColor(0,0,0);
  pen.drawString(30,40, "This is cool...");

  pen.rotate(10);
  pen.drawString(30,40, "This is cooler...");

  pen.drawBitmap(128,68, bitmap);
  cout << "painted" << endl;
}

int 
main(int argc, char **argv, char **envv)
{
  toad::initialize(argc, argv, envv);
  {
    TMyWindow wnd(NULL, "4th program");
    toad::mainLoop();
  }
  toad::terminate();
}
