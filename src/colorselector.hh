#ifndef _TOAD_COLORSELECTOR_HH
#define _TOAD_COLORSELECTOR_HH

#include <toad/dnd/color.hh>

namespace toad {

class TPushButton;
class TFigureEditor;

class TColorSelector:
  public TWindow
{
    typedef TWindow super;

    TFigureEditor *gedit;
    TRGB linecolor;
    TRGB fillcolor;
    bool filled;
    int border, w2, h;
    TPushButton *pb1, *pb2;
    TDropSiteColor *ds;

  public:
    TColorSelector(TWindow *parent, const string &title, TFigureEditor *gedit = 0);

  protected:
    void resize();
    void paint();
    void mouseLDown(int, int, unsigned);
    void mouseMDown(int, int, unsigned);

    void dropColor(const PDnDColor&);
    void openColorDialog();
    void openColorPalette();
};

} // namespace toad

#endif
