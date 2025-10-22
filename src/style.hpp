#pragma once
#include <FL/Fl_Text_Editor.H>

namespace Editor {
void attach_highlighting(Fl_Text_Editor* e);
void style_update(int pos, int nInserted, int nDeleted, int nRestyled, const char* dtext, void* cbArg);
void style_parse(const char* text, char* style, int length);
} // namespace Editor
