namespace toad {

/**
 * TX11CreateWindow is a data structure used in connection with 
 * 'createX11Window'; it was invented while adding OpenGL support
 */
struct TX11CreateWindow
{
  Display *display;
  Window parent;
  int x,y;
  unsigned width, height;
  unsigned border;      
  int depth;
  unsigned wclass;
  Visual *visual;
  unsigned long valuemask;
  XSetWindowAttributes *attributes;
};

} // namespace toad
