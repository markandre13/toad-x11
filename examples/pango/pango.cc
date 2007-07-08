#define _TOAD_PRIVATE

#include <toad/toad.hh>
#include <toad/form.hh>
#include <toad/combobox.hh>
#include <toad/scrollpane.hh>

#include <pango/pango.h>
#include <pango/pangox.h>

using namespace toad;

char *text =
  "Παν語\n"
  "\n"
  "This is a list of ways to say hello in various languages. Its purpose is to illustrate a number of scripts:\n"
  "Arabic	السلام عليكم \n"
  "Bengali (বাঙ্লা)	ষাগতোম\n"
  "Burmese (မ္ရန္မာ)\n"
  "Cherokee	(ᏣᎳᎩ)	ᎣᏏᏲ\n"
  "Czech	(česky)	Dobrý den\n"
  "Danish	(Dansk)	Hej, Goddag\n"
  "English	Hello\n"
  "Esperanto	Saluton\n"
  "Estonian	Tere, Tervist\n"
  "FORTRAN	PROGRAM\n"
  "Finnish	(Suomi)	Hei\n"
  "French	(Français)	Bonjour, Salut\n"
  "German	(Deutsch Nord)	Guten Tag\n"
  "German	(Deutsch Süd)	Grüß Gott\n"
  "Georgian	(ქართველი)	გამარჯობა\n"
  "Gujarati     (ગુજરાતિ)\n"
  "Greek	(Ελληνικά)	Γειά σας\n"
  "Hebrew	שלום\n"
  "Hindi	नमस्ते, नमस्कार।\n"
  "Italiano	Ciao, Buon giorno\n"
  "ɪŋglɪʃ       hɛləʊ\n"
  "Maltese	Ċaw, Saħħa\n"
  "Nederlands, Vlaams	Hallo, Dag\n"
  "Norwegian	(Norsk)	Hei, God dag\n"
  "Punjabi     (ੁਪੁਂਜਾਬਿ)\n"
  "Polish	Dzień dobry, Hej\n"
  "Russian	(Русский)	Здравствуйте!\n"
  "Slovak	Dobrý deň\n"
  "Spanish	(Español)	‎¡Hola!‎\n"
  "Swedish	(Svenska)	Hej, Goddag\n"
  "Thai	(ภาษาไทย)	สวัสดีครับ, สวัสดีค่ะ\n"
  "Tamil		(தமிழ்) வணக்கம்\n"
  "Turkish	(Türkçe)	Merhaba\n"
  "Vietnamese	(Tiếng Việt)	Xin Chào\n"
  "Yiddish	‏(ײַדישע)	דאָס הײַזעלע \n"
  "\n"
  "Japanese	(日本語)	こんにちは, ｺﾝﾆﾁﾊ\n"
  "Chinese	(中文,普通话,汉语)	你好\n"
  "Cantonese	(粵語,廣東話)	早晨, 你好\n"
  "Korean	(한글)	안녕하세요, 안녕하십니까\n"
  "\n"
  "Difference among chinese characters in GB, JIS, KSC, BIG5:‎\n"
  " GB	--	元气	开发\n"
  " JIS	--	元気	開発\n"
  " KSC	--	元氣	開發\n"
  " BIG5	--	元氣	開發\n";


class TPangoExample:
  public TScrollPane
{
    PangoLayout *layout;
  public:
    TPangoExample(TWindow *parent, const string &title);
    ~TPangoExample();
    void paint();
    void adjustPane();
};

class TMainWindow:
  public TForm
{
  public:
    TMainWindow(TWindow *parent, const string &title);
    void fontChanged(TComboBox *cb, TPangoExample *view);
};

PangoContext *context = 0;

int
main(int argc, char **argv, char **envv)
{
  g_type_init();
  toad::initialize(argc, argv, envv);
  context = pango_x_get_context(toad::x11display);
  pango_context_set_language (context, pango_language_from_string("en-us"));
  pango_context_set_base_dir (context, PANGO_DIRECTION_LTR);
  {
    TMainWindow wnd(0, "TOAD Pango Example");
    toad::mainLoop();
  }
  toad::terminate();
  return 0;
}

static int n_families;
static PangoFontFamily **families;

class TTableCellRenderer_PangoFontFamily:
  public TAbstractTableCellRenderer
{
  public:
    TTableCellRenderer_PangoFontFamily() {
      pango_context_list_families (context, &families, &n_families);
    }
    int getRows() { return n_families; }
    int getCols() { return 1; }
    int getRowHeight(int) { TOADBase::getDefaultFont().getHeight()+2; }
    int getColWidth(int x) {
      return 150;
    }
    void renderItem(TPen &pen, int, int y, int, int, bool, bool, bool) {
      pen.drawString(0,0,pango_font_family_get_name(families[y]));
    }
};

TMainWindow::TMainWindow(TWindow *parent, const string &title):
  TForm(parent, title)
{
  TComboBox *cb = new TComboBox(this, "face");
  cb->setRenderer(new TTableCellRenderer_PangoFontFamily);

  TPangoExample *view = new TPangoExample(this, "pango");

  CONNECT(cb->sigSelection, this, fontChanged, cb, view);


  attach(view, TOP|LEFT|RIGHT);
  attach(view, BOTTOM, cb);
  attach(cb, LEFT|RIGHT|BOTTOM);



  nBorderOverlap = -3;
  setBackground(TColor::DIALOG);
}

void
TMainWindow::fontChanged(TComboBox *cb, TPangoExample *)
{
  cerr << pango_font_family_get_name(families[cb->getLastSelectionRow()]) << endl;

  // PangoFontDescription *font_desc = pango_font_face_describe(face);

  // view->setFont(
}

TPangoExample::TPangoExample(TWindow *parent, const string &title):
  TScrollPane(parent, title)
{
  layout = pango_layout_new (context);
  pango_layout_set_text(layout, text, strlen(text));
}

TPangoExample::~TPangoExample()
{
  g_object_unref(layout);
}

void
TPangoExample::adjustPane()
{
  PangoRectangle logical_rect;
  pango_layout_set_width(layout, getWidth() * PANGO_SCALE);
  pango_layout_get_extents (layout, NULL, &logical_rect);
  pane.w = getWidth();
  pane.h = logical_rect.height / PANGO_SCALE;
}

void
TPangoExample::paint()
{
  TPen pen(this);
  
  pango_x_render_layout(x11display, 
                        x11window, 
                        pen.o_gc, 
                        layout, 
                        getOriginX(), getOriginY());
  paintCorner(pen);
}
