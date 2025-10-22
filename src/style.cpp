#include "editor.hpp"
#include "style.hpp"
#include <FL/Fl_Text_Display.H>
#include <cctype>
#include <cstring>
#include <cstdlib>

namespace Editor {

static Fl_Text_Display::Style_Table_Entry styletable[] = {
  { FL_BLACK,      FL_COURIER,        FL_NORMAL_SIZE }, // A - Plain
  { FL_DARK_GREEN, FL_COURIER_ITALIC, FL_NORMAL_SIZE }, // B - Line comments
  { FL_DARK_GREEN, FL_COURIER_ITALIC, FL_NORMAL_SIZE }, // C - Block comments
  { FL_BLUE,       FL_COURIER,        FL_NORMAL_SIZE }, // D - Strings
  { FL_DARK_RED,   FL_COURIER,        FL_NORMAL_SIZE }, // E - Directives
  { FL_DARK_RED,   FL_COURIER_BOLD,   FL_NORMAL_SIZE }, // F - Types
  { FL_BLUE,       FL_COURIER_BOLD,   FL_NORMAL_SIZE }  // G - Keywords
};

static const char* code_types[] = { "int","long","short","float","double","char","void","bool","size_t" };
static const char* code_keywords[] = { "if","else","for","while","switch","case","break","continue","return","class","struct","namespace","public","private","protected","virtual","template","try","catch","new","delete","include","define" };

static int compare_keywords(const void* a, const void* b) {
  const char* const *sa = (const char* const*)a;
  const char* const *sb = (const char* const*)b;
  return std::strcmp(*sa, *sb);
}

void attach_highlighting(Fl_Text_Editor* e) {
  if (!stylebuf) stylebuf = new Fl_Text_Buffer();
  e->highlight_data(stylebuf, styletable,
                    (int)(sizeof(styletable)/sizeof(styletable[0])),
                    'A', nullptr, 0);
  Editor::textbuf->add_modify_callback(style_update, e);
  int len = Editor::textbuf->length();
  char* init = (char*)std::malloc((size_t)len + 1);
  std::memset(init, 'A', (size_t)len);
  init[len] = '\0';
  stylebuf->text(init);
  std::free(init);
}

void style_update(int pos, int nInserted, int nDeleted, int, const char*, void* cbArg) {
  int start, end;
  char last, *style, *text;

  if (nInserted == 0 && nDeleted == 0) { stylebuf->unselect(); return; }

  if (nInserted > 0) {
    char* chunk = new char[nInserted + 1];
    std::memset(chunk, 'A', nInserted);
    chunk[nInserted] = '\0';
    stylebuf->replace(pos, pos + nDeleted, chunk);
    delete[] chunk;
  } else {
    stylebuf->remove(pos, pos + nDeleted);
  }

  stylebuf->select(pos, pos + nInserted - nDeleted);

  start = Editor::textbuf->line_start(pos);
  end   = Editor::textbuf->line_end(pos + nInserted - nDeleted);
  text  = Editor::textbuf->text_range(start, end);
  style = stylebuf->text_range(start, end);
  last  = style[end - start - 1];

  style_parse(text, style, end - start);
  stylebuf->replace(start, end, style);
  ((Fl_Text_Editor*)cbArg)->redisplay_range(start, end);

  if (last != style[end - start - 1]) {
    std::free(text);
    std::free(style);
    end   = Editor::textbuf->length();
    text  = Editor::textbuf->text_range(start, end);
    style = stylebuf->text_range(start, end);
    style_parse(text, style, end - start);
    stylebuf->replace(start, end, style);
    ((Fl_Text_Editor*)cbArg)->redisplay_range(start, end);
  }

  std::free(text);
  std::free(style);
}

void style_parse(const char *text, char *style, int length) {
  char current = *style;
  int col = 0;
  int last_word = 0;
  char buf[255], *bufptr;
  const char* temp;

  for (; length > 0; length--, text++) {
    if (current == 'A') {
      if (col == 0 && *text == '#') current = 'E';
      else if (std::strncmp(text, "//", 2) == 0) current = 'B';
      else if (std::strncmp(text, "/*", 2) == 0) current = 'C';
      else if (std::strncmp(text, "\\\"", 2) == 0) { *style++ = current; *style++ = current; text++; length--; col += 2; continue; }
      else if (*text == '\"') current = 'D';
      else if (!last_word && std::islower((unsigned char)*text)) {
        for (temp = text, bufptr = buf;
             std::islower((unsigned char)*temp) && bufptr < (buf + sizeof(buf) - 1);
             *bufptr++ = *temp++);
        if (!std::islower((unsigned char)*temp)) {
          *bufptr = '\0';
          const char* key = buf;
          if (std::bsearch(&key, code_types, sizeof(code_types)/sizeof(code_types[0]),
                           sizeof(code_types[0]), compare_keywords)) {
            while (text < temp) { *style++ = 'F'; text++; length--; col++; }
            text--; length++; last_word = 1; continue;
          } else if (std::bsearch(&key, code_keywords, sizeof(code_keywords)/sizeof(code_keywords[0]),
                                  sizeof(code_keywords[0]), compare_keywords)) {
            while (text < temp) { *style++ = 'G'; text++; length--; col++; }
            text--; length++; last_word = 1; continue;
          }
        }
      }
    } else if (current == 'C' && std::strncmp(text, "*/", 2) == 0) {
      *style++ = current; *style++ = current; text++; length--; current = 'A'; col += 2; continue;
    } else if (current == 'D') {
      if (std::strncmp(text, "\\\"", 2) == 0) { *style++ = current; *style++ = current; text++; length--; col += 2; continue; }
      else if (*text == '\"') { *style++ = current; col++; current = 'A'; continue; }
    }

    if (current == 'A' && (*text == '{' || *text == '}')) *style++ = 'G';
    else *style++ = current;
    col++;
    last_word = std::isalnum((unsigned char)*text) || *text == '.';

    if (*text == '\n') { col = 0; if (current == 'B' || current == 'E') current = 'A'; }
  }
}

} // namespace Editor
