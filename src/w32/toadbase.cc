#define _TOAD_PRIVATE

#include <toad/toadbase.hh>
#include <toad/window.hh>

HINSTANCE toad::w32instance;
int toad::w32cmdshow;

int toad::argc;
char** toad::argv;

void* toad::top_address;

namespace toad {
  LRESULT CALLBACK w32winproc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam);
} // namespace toad;

void
toad::initialize(HINSTANCE hInstance,
                 HINSTANCE hPrevInstance,
                 LPSTR lpCmdLine,
                 int CmdShow)
{
  w32instance = hInstance;
  w32cmdshow  = CmdShow;

  if (!hPrevInstance) {
    TWindow::w32registerclass();
  }
}

int 
toad::mainLoop()
{
	MSG msg;

  if (!TWindow::createParentless())
    return 0;

	while( GetMessage(&msg, NULL, 0, 0) )	{
    TranslateMessage( &msg );
		DispatchMessage( &msg );
	}

  TWindow::destroyParentless();

  return 0;
}

void
toad::terminate()
{
}

void
toad::TWindow::w32registerclass()
{
	WNDCLASSEX wcex;
	wcex.lpszClassName= "TOAD:BASE";
 	wcex.hInstance		  = w32instance;
 	wcex.lpfnWndProc	  = (WNDPROC)&w32proc;
 	wcex.hCursor		    = LoadCursor(NULL, IDC_ARROW);
 	wcex.hIcon			    = 0; // LoadIcon(hInstance, (LPCTSTR)IDI_TOAD);
 	wcex.lpszMenuName	  = NULL;
 	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
 	wcex.style			    = CS_HREDRAW | CS_VREDRAW;
 	wcex.cbSize         = sizeof(WNDCLASSEX);
 	wcex.cbClsExtra		  = 0;
 	wcex.cbWndExtra		  = sizeof(TWindow*);
 	wcex.hIconSm		    = 0; // LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);
 	RegisterClassEx(&wcex);
}

LRESULT CALLBACK
toad::TWindow::w32proc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
  static TMouseEvent me;

	static TWindow* twLastMouseMove = NULL;
	TWindow *wnd = (TWindow*)GetWindowLong(hWnd,0);
	if (!wnd)
		return DefWindowProc(hWnd,msg,wParam,lParam);

	switch(msg)
	{
	  case WM_ERASEBKGND: {
	    if (wnd->bNoBackground)
	      return 0L;
	    HDC hdc = (HDC)wParam;
	    HBRUSH brush = ::CreateSolidBrush(wnd->background.colorref);
	    HGDIOBJ ob = ::SelectObject(hdc, brush);
	    ::Rectangle(hdc, -1, -1, wnd->getWidth()+2, wnd->getHeight()+2);
	    ::SelectObject(hdc, ob);
	    ::DeleteObject(brush);
	    return 1L;
	    } break;
		case WM_PAINT: {
		  assert(wnd->paintstruct==0);
		  PAINTSTRUCT w32ps;
		  ::BeginPaint(wnd->w32window, &w32ps);
		  wnd->paintstruct = new TPaintStruct();
		  wnd->paintstruct->hdc = w32ps.hdc;
		  wnd->paintstruct->refcount = 1;
			wnd->paint();
			assert(wnd->paintstruct->refcount==1);
      if (wnd->paintstruct->origpen) {
        HGDIOBJ prevpen = ::SelectObject(w32ps.hdc, wnd->paintstruct->origpen);
        wnd->paintstruct->origpen = 0;
        ::DeleteObject(prevpen);
      }
      if (wnd->paintstruct->origbrush) {
        HGDIOBJ prevbrush = ::SelectObject(w32ps.hdc, wnd->paintstruct->origbrush);
        wnd->paintstruct->origbrush = 0;
        ::DeleteObject(prevbrush);
      }
			delete wnd->paintstruct;
			wnd->paintstruct = 0;
			::EndPaint(wnd->w32window, &w32ps);
			} break;
	  case WM_SIZE:
	    #warning "size isn't right"
	    wnd->_w = (int)LOWORD(lParam) - 2;
	    wnd->_h = (int)HIWORD(lParam) - 2;
	    wnd->doResize();
	    break;
	  case WM_MOVE:
	    wnd->_x = (int)LOWORD(lParam);
	    wnd->_y = (int)HIWORD(lParam);
	    break;
		case WM_LBUTTONDOWN:
		  me.window = wnd;
		  me.x = LOWORD(lParam);
		  me.y = HIWORD(lParam);
		  me.modifier = wParam;
		  me.type= TMouseEvent::LDOWN;
			wnd->mouseEvent(me);
			break;
		case WM_MBUTTONDOWN:
		  me.window = wnd;
		  me.x = LOWORD(lParam);
		  me.y = HIWORD(lParam);
		  me.modifier = wParam;
		  me.type= TMouseEvent::MDOWN;
			wnd->mouseEvent(me);
			break;
		case WM_RBUTTONDOWN:
		  me.window = wnd;
		  me.x = LOWORD(lParam);
		  me.y = HIWORD(lParam);
		  me.modifier = wParam;
		  me.type= TMouseEvent::LDOWN;
			wnd->mouseEvent(me);
			break;
		case WM_LBUTTONUP:
		  me.window = wnd;
		  me.x = LOWORD(lParam);
		  me.y = HIWORD(lParam);
		  me.modifier = wParam;
		  me.type= TMouseEvent::LUP;
			wnd->mouseEvent(me);
			break;
		case WM_MBUTTONUP:
		  me.window = wnd;
		  me.x = LOWORD(lParam);
		  me.y = HIWORD(lParam);
		  me.modifier = wParam;
		  me.type= TMouseEvent::MUP;
			wnd->mouseEvent(me);
			break;
		case WM_RBUTTONUP:
		  me.window = wnd;
		  me.x = LOWORD(lParam);
		  me.y = HIWORD(lParam);
		  me.modifier = wParam;
		  me.type= TMouseEvent::RUP;
			wnd->mouseEvent(me);
			break;
		case WM_MOUSEMOVE:
			// fast and sloopy hack:
			if (wnd != twLastMouseMove) {
				if (twLastMouseMove) {
				  me.window = twLastMouseMove;
				  me.x = 0;
				  me.y = 0;
				  me.modifier = wParam;
				  me.type = TMouseEvent::LEAVE;
				  twLastMouseMove->mouseEvent(me);
				}
			}

		  me.window = wnd;
		  me.x = LOWORD(lParam);
		  me.y = HIWORD(lParam);
		  me.modifier = wParam;
		  me.type= TMouseEvent::MOVE;
			wnd->mouseEvent(me);

			if (wnd != twLastMouseMove) {
				if (wnd) {
				  me.window = twLastMouseMove;
				  me.x = LOWORD(lParam);
				  me.y = HIWORD(lParam);
				  me.modifier = wParam;
				  me.type = TMouseEvent::ENTER;
				  wnd->mouseEvent(me);
				}
				twLastMouseMove = wnd;
			}


			break;
#if 0
		case WM_SIZE:
// add/sub boder ?
			wnd->nWidth = (int)LOWORD(lParam);
			wnd->nHeight= (int)HIWORD(lParam);
//			InvalidateRect(wnd->window, NULL, TRUE);
			wnd->resize();
//			ValidateRect(wnd->window, NULL);
			break;
		case WM_MOVE:
			wnd->nXPos = LOWORD(lParam);
			wnd->nYPos = HIWORD(lParam);
			break;
#endif
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hWnd,msg,wParam,lParam);
	}
	return 0L;
}
