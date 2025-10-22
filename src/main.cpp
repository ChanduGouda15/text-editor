#include <FL/Fl.H>
#include "editor.hpp"

int main(int argc, char **argv) {
  Editor::init_shared_buffer();
  Fl_Window* w = Editor::new_view();
  w->show(argc > 0 ? 1 : 0, argv);
  if (argc > 1) {
    Editor::load_file(argv[1], -1);
  }
  return Fl::run();
}
