#pragma once
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Text_Editor.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/fl_ask.H>
#include <cstring>

namespace Editor {

extern int            changed;
extern int            loading;
extern char           filename[256];
extern Fl_Text_Buffer *textbuf;
extern Fl_Text_Buffer *stylebuf;

class EditorWindow : public Fl_Double_Window {
public:
  EditorWindow(int w, int h, const char* t);
  ~EditorWindow();

  Fl_Menu_Bar      *menubar{nullptr};
  Fl_Text_Editor   *editor{nullptr};

  Fl_Window        *replace_dlg{nullptr};
  Fl_Input         *replace_find{nullptr};
  Fl_Input         *replace_with{nullptr};
  Fl_Button        *replace_all{nullptr};
  Fl_Return_Button *replace_next{nullptr};
  Fl_Button        *replace_cancel{nullptr};

  char search[256]{0};
};

void init_shared_buffer();
Fl_Window* new_view();
void close_view(Fl_Widget*, void*);

void set_title(Fl_Window* w);
int  check_save();
void load_file(const char *newfile, int ipos);
void save_file(const char *newfile);

void changed_cb(int pos, int nInserted, int nDeleted, int nRestyled, const char* dtext, void* v);
void new_cb(Fl_Widget*, void*);
void open_cb(Fl_Widget*, void*);
void insert_cb(Fl_Widget*, void*);
void save_cb(Fl_Widget*, void*);
void saveas_cb(Fl_Widget*, void*);
void view_cb(Fl_Widget*, void*);
void close_cb(Fl_Widget*, void*);
void quit_cb(Fl_Widget*, void*);

void copy_cb(Fl_Widget*, void*);
void cut_cb(Fl_Widget*, void*);
void paste_cb(Fl_Widget*, void*);
void delete_cb(Fl_Widget*, void*);

void find_cb(Fl_Widget*, void*);
void find2_cb(Fl_Widget*, void*);
void replace_cb(Fl_Widget*, void*);
void replace2_cb(Fl_Widget*, void*);
void replall_cb(Fl_Widget*, void*);
void replcan_cb(Fl_Widget*, void*);

void attach_highlighting(Fl_Text_Editor* e);

} // namespace Editor
