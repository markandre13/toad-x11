/*
 * This programs creates a text model and a number of views related to it,
 * and performs various operation like insert, delete, copy, move, etc. on
 * it while comparing the results with a control string.
 *
 * missing: UTF-8 multiple byte characters
 *          insert/remove more than one character
 */

#include <stdlib.h>

#define _TOAD_PRIVATE

#include <toad/toad.hh>
#include <toad/textarea.hh>

using namespace toad;

#define NVIEW 4

struct TView {
  TTextArea *view;
  unsigned pos;
};

TTextModel model;

void
checkCursor(TTextArea *ta, const string &str)
{
  unsigned pos = ta->getPos();
  unsigned x, y;
  x = y = 0;
  for(unsigned i=0; i<pos; ++i) {
    ++x;
    if (str[i]=='\n') {
      x=0;
      ++y;
    }
  }
  if (x!=ta->getCursorX() || y!=ta->getCursorY()) {
    cout << "model = '" << model << "'\n";
    cout << "str   = '" << str << "'\n";
    cout << "wrong cursor position: view is at (" 
         << ta->getCursorX() << ", " << ta->getCursorY()
         << ") but pos " << pos << " is at (" 
         << x << ", " << y << ")\n";
    exit(0);
  }
}

int
main(int argc, char **argv, char **envv)
{
  toad::initialize(argc, argv, envv); {
  string str;
  TView view[NVIEW];
  for(unsigned i=0; i<NVIEW; ++i) {
    view[i].view = new TTextArea(0, "view", &model);
    view[i].view->getPreferences()->autoindent=false;
    view[i].pos  = 0;
  }
  unsigned n2;

  TWindow::createParentless();
  TOADBase::bAppIsRunning = true;

  srand(1);
  char buffer[20];
  buffer[0] = 'a';
  for(unsigned i=0; i<10000; ++i) {
    toad::flush();
    while(TOADBase::peekMessage())
      TOADBase::handleMessage();
  
    unsigned n;
    unsigned cmd    = 1+(int) (3.0*rand()/(RAND_MAX+1.0));
    unsigned nmodel = (int) (static_cast<double>(NVIEW)*rand()/(RAND_MAX+1.0));
    buffer[0]       = 'a'+(int) (28.0*rand()/(RAND_MAX+1.0));
    buffer[1]       = 0;
    switch(cmd) {
      case 1: // insert characters
        n = 1+(int) (10.0*rand()/(RAND_MAX+1.0));
        for(unsigned j=0; j<n; ++j) {
          if ((unsigned char)buffer[0]<='z') {
            // cout << "insert '" << buffer[0] << "' in view " << nmodel << endl;
            view[nmodel].view->keyDown(0, buffer, 0);
            str.insert(view[nmodel].pos, buffer);
          } else {
            cout << "start: insert new line in view " << nmodel << endl;
            buffer[0]='\n';
            view[nmodel].view->keyDown(TK_RETURN, buffer, 0);
            str.insert(view[nmodel].pos, buffer);
            cout << "end: insert new line in view " << nmodel << endl;
            n=1;
          }
          for(unsigned l=0; l<NVIEW; ++l) {
            checkCursor(view[l].view, str);
          } 
          for(unsigned k=0; k<NVIEW; ++k) {
            if (k==nmodel)
              continue;
            if (view[k].pos >= view[nmodel].pos)
              view[k].pos++;
          }
          view[nmodel].pos++;
          buffer[0] = 'A'+(int) (26.0*rand()/(RAND_MAX+1.0));
        }
        break;
      case 2: // move cursor
        n = (int) (static_cast<double>(str.size())*rand()/(RAND_MAX+1.0));
        n2=0;
        while(view[nmodel].view->getPos()!=n) {
          n2++;
          if ((n2%1000)==0) {
            cout << n2 << ":moving cursor from " << view[nmodel].view->getPos() << " to " << n << endl;
          }
/*
          if (i>=38 && n2>=4) {
            cerr << "i=" << i << ", n2=" << n2 << endl;
          }
*/
          unsigned m = (int) ( (3.0+n2/100.0)*rand()/(RAND_MAX+1.0));
          if (view[nmodel].view->getPos()<n) {
            if (m>0) {
              view[nmodel].view->keyDown(TK_RIGHT, buffer, 0);
              checkCursor(view[nmodel].view, str);
            } else {
              view[nmodel].view->keyDown(TK_DOWN, buffer, 0);
              checkCursor(view[nmodel].view, str);
            }
          } else {
            if (m>0) {
              view[nmodel].view->keyDown(TK_LEFT, buffer, 0);
              checkCursor(view[nmodel].view, str);
            } else {
              view[nmodel].view->keyDown(TK_RIGHT, buffer, 0);
              checkCursor(view[nmodel].view, str);
            }
          }
        }
        view[nmodel].pos = n;
        break;
      case 3: // delete
        n = 1+(int) (5.0*rand()/(RAND_MAX+1.0));
        n2 = 0;
        for(unsigned j=0; j<n; ++j) {
          n2++;
          if (i>=63 && n2>=2) {
            cerr << "i=" << i << ", n2=" << n2 << endl;
          }
          view[nmodel].view->keyDown(TK_DELETE, buffer, 0);
          if (view[nmodel].pos==str.size())
            break;
          try {
            str.erase(view[nmodel].pos, 1);
          } catch (...) {
            cerr << "erase failed at " << view[nmodel].pos << " in string of size " << str.size() << endl;
            exit(1);
          }
          for(unsigned k=0; k<NVIEW; ++k) {
            if (k==nmodel)
              continue;
            if (view[k].pos > view[nmodel].pos)
              view[k].pos--;
          }
          for(unsigned l=0; l<NVIEW; ++l) {
            checkCursor(view[l].view, str);
          } 
        }
        break;
      case 4: // backspace
        break;
    }
    if (model.getValue() != str) {
      cout << "model = '" << model << "'\n";
      cout << "str   = '" << str << "'\n";
      cout << "MISMATCH\n";
      exit(0);
    }
/*
    cout << "model = '" << model << "'\n";
    cout << "str   = '" << str << "'\n";
    cout << "---------------------------------------------\n";
*/
  }
  
  TWindow::destroyParentless();
  
  cout << "model = '" << model << "'\n";
  cout << "OKAY\n";
  } toad::terminate();
  return 0;
}
