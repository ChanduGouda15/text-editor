#include "editor.hpp"
#include "style.hpp"
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/fl_ask.H>
#include <cstdio>
#include <cerrno>

namespace Editor {

int changed = 0;
int loading = 0;
char filename[256] = "";
Fl_Text_Buffer *textbuf = nullptr;
Fl_Text_Buffer *stylebuf = nullptr;

static Fl_Menu_Item menuitems[] = {
  { "&File",              0, 0, 0, FL_SUBMENU },
    { "&New File",        0, (Fl_Callback*)new_cb },
    { "&Open File...",    FL_CTRL + 'o', (Fl_Callback*)open_cb },
    { "&Insert File...",  FL_CTRL + 'i', (Fl_Callback*)insert_cb, 0, FL_MENU_DIVIDER },
    { "&Save File",       FL_CTRL + 's', (Fl_Callback*)save_cb },
    { "Save File &As...", FL_CTRL + FL_SHIFT + 's', (Fl_Callback*)saveas_cb, 0, FL_MENU_DIVIDER },
    { "New &View",        FL_ALT  + 'v', (Fl_Callback*)view_cb },
    { "&Close View",      FL_CTRL + 'w', (Fl_Callback*)close_cb, 0, FL_MENU_DIVIDER },
    { "E&xit",            FL_CTRL + 'q', (Fl_Callback*)quit_cb, 0 },
    { 0 },

  { "&Edit", 0, 0, 0, FL_SUBMENU },
    { "&Undo",       FL_CTRL + 'z', (Fl_Callback*)Fl_Text_Editor::kf_undo, 0, FL_MENU_DIVIDER },
    { "Cu&t",        FL_CTRL + 'x', (Fl_Callback*)cut_cb },
    { "&Copy",       FL_CTRL + 'c', (Fl_Callback*)copy_cb },
    { "&Paste",      FL_CTRL + 'v', (Fl_Callback*)paste_cb },
    { "&Delete",     0,             (Fl_Callback*)delete_cb },
    { 0 },

  { "&Search", 0, 0, 0, FL_SUBMENU },
    { "&Find...",       FL_CTRL + 'f', (Fl_Callback*)find_cb },
    { "F&ind Again",    FL_CTRL + 'g', (Fl_Callback*)find2_cb },
    { "&Replace...",    FL_CTRL + 'r', (Fl_Callback*)replace_cb },
    { "Re&place Again", FL_CTRL + 't', (Fl_Callback*)replace2_cb },
    { 0 },

  { 0 }
};

static int num_windows = 0;

EditorWindow::EditorWindow(int w, int h, const char* t)
: Fl_Double_Window(w, h, t) {
  begin();
  menubar = new Fl_Menu_Bar(0, 0, w, 30);
  menubar->copy(menuitems);

  editor = new Fl_Text_Editor(0, 30, w, h - 30);
  editor->buffer(textbuf);
  editor->textfont(FL_COURIER);
  editor->when(FL_WHEN_CHANGED);

  replace_dlg   = new Fl_Window(300, 105, "Replace");
  replace_find  = new Fl_Input(70, 10, 200, 25, "Find:");
  replace_with  = new Fl_Input(70, 40, 200, 25, "Replace:");
  replace_all   = new Fl_Button(10, 70, 90, 25, "Replace All");
  replace_next  = new Fl_Return_Button(105, 70, 120, 25, "Replace Next");
  replace_cancel= new Fl_Button(230, 70, 60, 25, "Cancel");

  replace_all->callback(replall_cb, this);
  replace_next->callback(replace2_cb, this);
  replace_cancel->callback(replcan_cb, this);
  replace_dlg->end();
  end();

  resizable(editor);
  set_title(this);
  num_windows++;

  attach_highlighting(editor);
}

EditorWindow::~EditorWindow() { num_windows--; }

void init_shared_buffer() {
  if (!textbuf) {
    textbuf  = new Fl_Text_Buffer();
    stylebuf = new Fl_Text_Buffer();
    textbuf->add_modify_callback(changed_cb, nullptr);
  }
}

static Fl_Window* make_window() {
  auto *w = new EditorWindow(800, 600, "FLTK Text Editor");
  w->callback(close_view, w);
  return w;
}

Fl_Window* new_view() {
  Fl_Window* w = make_window();
  w->show();
  return w;
}

void close_view(Fl_Widget*, void* v) {
  Fl_Window* w = static_cast<Fl_Window*>(v);
  if (num_windows == 1 && !check_save()) return;
  w->hide();
  delete w;
  if (!num_windows) std::exit(0);
}

void set_title(Fl_Window* w) {
  char title[512];
  if (filename[0] == '\0') std::strcpy(title, "Untitled");
  else {
    const char* slash = std::strrchr(filename, '/');
#ifdef _WIN32
    if (!slash) slash = std::strrchr(filename, '\\');
#endif
    if (slash) std::strcpy(title, slash + 1);
    else std::strcpy(title, filename);
  }
  if (changed) std::strcat(title, " (modified)");
  w->label(title);
}

int check_save() {
  if (!changed) return 1;
  int r = fl_choice("The current file has not been saved.\nWould you like to save it now?",
                    "Cancel", "Save", "Discard");
  if (r == 1) { save_cb(nullptr, nullptr); return !changed; }
  return (r == 2) ? 1 : 0;
}

void load_file(const char *newfile, int ipos) {
  loading = 1;
  int insert = (ipos != -1);
  changed = insert;
  if (!insert) std::strcpy(filename, "");
  int r = insert ? textbuf->insertfile(newfile, ipos)
                 : textbuf->loadfile(newfile);
  if (r) fl_alert("Error reading from file '%s':\n%s.", newfile, std::strerror(errno));
  else if (!insert) std::strcpy(filename, newfile);
  loading = 0;
  textbuf->call_modify_callbacks();
}

void save_file(const char *newfile) {
  if (textbuf->savefile(newfile))
    fl_alert("Error writing to file '%s':\n%s.", newfile, std::strerror(errno));
  else
    std::strcpy(filename, newfile);
  changed = 0;
  textbuf->call_modify_callbacks();
}

void changed_cb(int, int nInserted, int nDeleted, int, const char*, void* v) {
  if ((nInserted || nDeleted) && !loading) changed = 1;
  if (auto* w = static_cast<Fl_Window*>(v)) set_title(w);
}

void new_cb(Fl_Widget*, void*) {
  if (!check_save()) return;
  filename[0] = '\0';
  textbuf->select(0, textbuf->length());
  textbuf->remove_selection();
  changed = 0;
  textbuf->call_modify_callbacks();
}

void open_cb(Fl_Widget*, void*) {
  if (!check_save()) return;
  char *newfile = fl_file_chooser("Open File?", "*", filename);
  if (newfile) load_file(newfile, -1);
}

void insert_cb(Fl_Widget*, void* v) {
  char *newfile = fl_file_chooser("Insert File?", "*", filename);
  if (!newfile) return;
  auto* e = static_cast<EditorWindow*>(v);
  load_file(newfile, e->editor->insert_position());
}

void save_cb(Fl_Widget*, void*) {
  if (filename[0] == '\0') { saveas_cb(nullptr, nullptr); return; }
  save_file(filename);
}

void saveas_cb(Fl_Widget*, void*) {
  char *newfile = fl_file_chooser("Save File As?", "*", filename);
  if (newfile) save_file(newfile);
}

void view_cb(Fl_Widget*, void*) { new_view(); }
void close_cb(Fl_Widget*, void* v) { close_view(nullptr, v); }

void quit_cb(Fl_Widget*, void*) {
  if (changed && !check_save()) return;
  std::exit(0);
}

void copy_cb(Fl_Widget*, void* v) {
  auto* e = static_cast<EditorWindow*>(v);
  Fl_Text_Editor::kf_copy(0, e->editor);
}

void cut_cb(Fl_Widget*, void* v) {
  auto* e = static_cast<EditorWindow*>(v);
  Fl_Text_Editor::kf_cut(0, e->editor);
}

void paste_cb(Fl_Widget*, void* v) {
  auto* e = static_cast<EditorWindow*>(v);
  Fl_Text_Editor::kf_paste(0, e->editor);
}

void delete_cb(Fl_Widget*, void*) {
  textbuf->remove_selection();
}

void find_cb(Fl_Widget* w, void* v) {
  auto* e = static_cast<EditorWindow*>(v);
  const char *val = fl_input("Search String:", e->search);
  if (val) { std::strcpy(e->search, val); find2_cb(w, v); }
}

void find2_cb(Fl_Widget*, void* v) {
  auto* e = static_cast<EditorWindow*>(v);
  if (e->search[0] == '\0') { find_cb(nullptr, v); return; }
  int pos = e->editor->insert_position();
  int found = textbuf->search_forward(pos, e->search, &pos);
  if (found) {
    textbuf->select(pos, pos + std::strlen(e->search));
    e->editor->insert_position(pos + std::strlen(e->search));
    e->editor->show_insert_position();
  } else {
    fl_alert("No occurrences of '%s' found!", e->search);
  }
}

void replace_cb(Fl_Widget*, void* v) {
  auto* e = static_cast<EditorWindow*>(v);
  e->replace_dlg->show();
}

void replace2_cb(Fl_Widget*, void* v) {
  auto* e = static_cast<EditorWindow*>(v);
  const char *find = e->replace_find->value();
  const char *repl = e->replace_with->value();
  if (!find || find[0] == '\0') { e->replace_dlg->show(); return; }
  e->replace_dlg->hide();
  int pos = e->editor->insert_position();
  int found = textbuf->search_forward(pos, find, &pos);
  if (found) {
    textbuf->select(pos, pos + (int)std::strlen(find));
    textbuf->remove_selection();
    textbuf->insert(pos, repl ? repl : "");
    textbuf->select(pos, pos + (int)std::strlen(repl ? repl : ""));
    e->editor->insert_position(pos + (int)std::strlen(repl ? repl : ""));
    e->editor->show_insert_position();
  } else {
    fl_alert("No occurrences of '%s' found!", find);
  }
}

void replall_cb(Fl_Widget*, void* v) {
  auto* e = static_cast<EditorWindow*>(v);
  const char *find = e->replace_find->value();
  const char *repl = e->replace_with->value();
  if (!find || find[0] == '\0') { e->replace_dlg->show(); return; }
  e->replace_dlg->hide();
  e->editor->insert_position(0);
  int times = 0;
  for (int found = 1; found;) {
    int pos = e->editor->insert_position();
    found = textbuf->search_forward(pos, find, &pos);
    if (found) {
      textbuf->select(pos, pos + (int)std::strlen(find));
      textbuf->remove_selection();
      textbuf->insert(pos, repl ? repl : "");
      e->editor->insert_position(pos + (int)std::strlen(repl ? repl : ""));
      e->editor->show_insert_position();
      times++;
    }
  }
  if (times) fl_message("Replaced %d occurrences.", times);
  else fl_alert("No occurrences of '%s' found!", find);
}

void replcan_cb(Fl_Widget*, void* v) {
  auto* e = static_cast<EditorWindow*>(v);
  e->replace_dlg->hide();
}

} // namespace Editor
