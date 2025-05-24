#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <time.h>
#include <ncurses.h>

#define STRING_CHAR char
#define STRCHR strchr
#define STRDUP strdup
#define STRLEN strlen
#define STRNCPY strncpy
#define STRCPY strcpy
#define STRCAT strcat
#define STRNDUP strndup
#define ADDSTR addstr
#define ISPRINT isprint
#define ISSPACE isspace
#define ISALNUM isalnum
#define NEWLINE "\n"

#define MAX_LINES 10000
#define MAX_LINE_LEN 1024
#define MAX_FILENAME_LEN 256
#define MAX_KILL_RING 1
#define CTRL_KEY(k) ((k) & 0x1f)
#define ALT_KEY(k) (k)
#define CTRL_X_TIMEOUT 1

// Language types
typedef enum {
    LANG_NONE,
    LANG_HTML,
    LANG_CSS,
    LANG_C,
    LANG_PYTHON
} Language;

// Token types
typedef enum {
    TOKEN_NORMAL,
    TOKEN_KEYWORD,
    TOKEN_STRING,
    TOKEN_COMMENT,
    TOKEN_NUMBER,
    TOKEN_PREPROC
} TokenType;

typedef struct {
    char **lines;
    int num_lines;
    int cursor_x, cursor_y;
    int top_line;
    char *filename;
    int max_y, max_x;
    char message[256];
    struct {
        char *action;
        int x, y;
        char data;
        char *bulk_data;
        int line_count;
    } *undo_stack;
    int undo_count, undo_size;
    char *kill_ring[MAX_KILL_RING];
    int mark_x, mark_y;
    bool mark_active;
    char search_query[256];
    bool searching;
    int current_buffer;
    char **buffers[2];
    int num_lines_buf[2];
    char *filenames[2];
    Language language;
    bool in_multiline_comment;
} Editor;

typedef void (*CommandFunc)(Editor *);

// Global commands array
CommandFunc commands[512] = {0};

// Function prototypes
void init_editor(Editor *e);
void cleanup_editor(Editor *e);
void draw(Editor *e);
void handle_input(Editor *e, int ch);
void load_file(Editor *e, const char *filename);
void save_file(Editor *e);
void add_undo(Editor *e, const char *action, int x, int y, char data, char *bulk_data, int line_count);
void undo(Editor *e);
void insert_char(Editor *e, char c, bool redraw);
void delete_char(Editor *e);
void delete_char_right(Editor *e);
void delete_word_left(Editor *e);
void delete_word_right(Editor *e);
void insert_newline(Editor *e, bool redraw);
void insert_lines(Editor *e, char **new_lines, int line_count, bool redraw);
void move_cursor_up(Editor *e);
void move_cursor_down(Editor *e);
void move_cursor_left(Editor *e);
void move_cursor_right(Editor *e);
void move_cursor_backward_word(Editor *e);
void move_cursor_forward_word(Editor *e);
void move_cursor_backward_paragraph(Editor *e);
void move_cursor_forward_paragraph(Editor *e);
void move_cursor_beginning_of_line(Editor *e);
void move_cursor_end_of_line(Editor *e);
void kill_line(Editor *e);
void yank(Editor *e);
void set_mark(Editor *e);
void delete_region(Editor *e);
void start_search(Editor *e);
void update_search(Editor *e, int c);
void switch_buffer(Editor *e);
void detect_language(Editor *e);
void show_info(Editor *e); 

// Syntax highlighting keywords
const char *html_keywords[] = {
    "html", "head", "body", "div", "span", "a", "img", "p", "h1", "h2", "h3",
    "h4", "h5", "h6", "ul", "li", "ol", "table", "tr", "td", "th", "form",
    "input", "button", "script", "style", "link", "meta", NULL
};

const char *css_keywords[] = {
    "color", "background", "margin", "padding", "border", "width", "height",
    "display", "position", "float", "clear", "font", "text-align", "overflow",
    "transition", "transform", "animation", NULL
};

const char *c_keywords[] = {
    "auto", "break", "case", "char", "const", "continue", "default", "do",
    "double", "else", "enum", "extern", "float", "for", "goto", "if", "int",
    "long", "register", "return", "short", "signed", "sizeof", "static",
    "struct", "switch", "typedef", "union", "unsigned", "void", "volatile",
    "while", NULL
};

const char *python_keywords[] = {
    "and", "as", "assert", "break", "class", "continue", "def", "del", "elif",
    "else", "except", "False", "finally", "for", "from", "global", "if",
    "import", "in", "is", "lambda", "None", "nonlocal", "not", "or", "pass",
    "raise", "return", "True", "try", "while", "with", "yield", NULL
};

// Color pairs
#define COLOR_KEYWORD 1
#define COLOR_STRING 2
#define COLOR_COMMENT 3
#define COLOR_NUMBER 4
#define COLOR_PREPROC 5

void init_colors() {
    start_color();
    init_pair(COLOR_KEYWORD, COLOR_CYAN, COLOR_BLACK);
    init_pair(COLOR_STRING, COLOR_GREEN, COLOR_BLACK);
    init_pair(COLOR_COMMENT, COLOR_YELLOW, COLOR_BLACK);
    init_pair(COLOR_NUMBER, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(COLOR_PREPROC, COLOR_RED, COLOR_BLACK);
}

void init_editor(Editor *e) {
    e->lines = malloc(MAX_LINES * sizeof(char *));
    e->lines[0] = malloc(MAX_LINE_LEN * sizeof(char));
    e->lines[0][0] = '\0';
    e->num_lines = 1;
    e->buffers[0] = e->lines;
    e->num_lines_buf[0] = e->num_lines;
    e->buffers[1] = malloc(MAX_LINES * sizeof(char *));
    e->buffers[1][0] = malloc(MAX_LINE_LEN * sizeof(char));
    e->buffers[1][0][0] = '\0';
    e->num_lines_buf[1] = 1;
    e->cursor_x = e->cursor_y = e->top_line = 0;
    e->filename = NULL;
    e->filenames[0] = e->filenames[1] = NULL;
    e->undo_stack = malloc(1000 * sizeof(*e->undo_stack));
    e->undo_count = 0;
    e->undo_size = 1000;
    e->message[0] = '\0';
    e->kill_ring[0] = NULL;
    e->mark_active = false;
    e->searching = false;
    e->search_query[0] = '\0';
    e->current_buffer = 0;
    e->language = LANG_NONE;
    e->in_multiline_comment = false;
    initscr();
    raw();
    noecho();
    keypad(stdscr, TRUE);
    getmaxyx(stdscr, e->max_y, e->max_x);
    signal(SIGINT, SIG_IGN);
    init_colors();
}

void cleanup_editor(Editor *e) {
    for (int i = 0; i < e->num_lines_buf[0]; i++) free(e->buffers[0][i]);
    for (int i = 0; i < e->num_lines_buf[1]; i++) free(e->buffers[1][i]);
    free(e->buffers[0]);
    free(e->buffers[1]);
    if (e->filenames[0]) free(e->filenames[0]);
    if (e->filenames[1]) free(e->filenames[1]);
    for (int i = 0; i < e->undo_count; i++) {
        free(e->undo_stack[i].action);
        if (e->undo_stack[i].bulk_data) free(e->undo_stack[i].bulk_data);
    }
    free(e->undo_stack);
    if (e->kill_ring[0]) free(e->kill_ring[0]);
    endwin();
}

void detect_language(Editor *e) {
    if (!e->filename) {
        e->language = LANG_NONE;
        return;
    }
    char *ext = strrchr(e->filename, '.');
    if (!ext) {
        e->language = LANG_NONE;
        return;
    }
    if (strcmp(ext, ".html") == 0) e->language = LANG_HTML;
    else if (strcmp(ext, ".css") == 0) e->language = LANG_CSS;
    else if (strcmp(ext, ".c") == 0 || strcmp(ext, ".cpp") == 0) e->language = LANG_C;
    else if (strcmp(ext, ".py") == 0) e->language = LANG_PYTHON;
    else e->language = LANG_NONE;
}

void highlight_line(Editor *e, const char *line, int y) {
    int i = 0, len = STRLEN(line);
    bool in_string = false;
    char string_delim = 0;
    bool in_comment = e->in_multiline_comment;
    bool in_line_comment = false;
    bool in_preproc = false;

    while (i < len) {
        if (in_comment) {
            attron(COLOR_PAIR(COLOR_COMMENT));
            while (i < len) {
                mvaddch(y, i + 6, line[i]);
                if (i + 1 < len && line[i] == '*' && line[i + 1] == '/') {
                    i += 2;
                    in_comment = false;
                    attroff(COLOR_PAIR(COLOR_COMMENT));
                    break;
                }
                i++;
            }
            if (i >= len) attroff(COLOR_PAIR(COLOR_COMMENT));
            continue;
        }

        if (in_line_comment) {
            attron(COLOR_PAIR(COLOR_COMMENT));
            while (i < len) {
                mvaddch(y, i + 6, line[i]);
                i++;
            }
            attroff(COLOR_PAIR(COLOR_COMMENT));
            break;
        }

        if (in_string) {
            attron(COLOR_PAIR(COLOR_STRING));
            mvaddch(y, i + 6, line[i]);
            if (line[i] == string_delim && (i == 0 || line[i - 1] != '\\')) {
                in_string = false;
                attroff(COLOR_PAIR(COLOR_STRING));
            }
            i++;
            continue;
        }

        if (in_preproc) {
            attron(COLOR_PAIR(COLOR_PREPROC));
            while (i < len && !ISSPACE(line[i])) {
                mvaddch(y, i + 6, line[i]);
                i++;
            }
            attroff(COLOR_PAIR(COLOR_PREPROC));
            in_preproc = false;
            continue;
        }

        if (e->language == LANG_HTML) {
            if (line[i] == '<' && (i + 1 < len && (isalpha(line[i + 1]) || line[i + 1] == '!'))) {
                attron(COLOR_PAIR(COLOR_KEYWORD));
                while (i < len && line[i] != '>') {
                    mvaddch(y, i + 6, line[i]);
                    i++;
                }
                if (i < len) {
                    mvaddch(y, i + 6, line[i]);
                    i++;
                }
                attroff(COLOR_PAIR(COLOR_KEYWORD));
                continue;
            }
        }

        if (e->language == LANG_C && i == 0 && line[i] == '#') {
            in_preproc = true;
            attron(COLOR_PAIR(COLOR_PREPROC));
            mvaddch(y, i + 6, line[i]);
            i++;
            continue;
        }

        if ((e->language == LANG_C || e->language == LANG_HTML) && i + 1 < len && line[i] == '/' && line[i + 1] == '*') {
            in_comment = true;
            attron(COLOR_PAIR(COLOR_COMMENT));
            mvaddch(y, i + 6, line[i]);
            i++;
            mvaddch(y, i + 6, line[i]);
            i++;
            continue;
        }

        if (e->language == LANG_C && i + 1 < len && line[i] == '/' && line[i + 1] == '/') {
            in_line_comment = true;
            attron(COLOR_PAIR(COLOR_COMMENT));
            mvaddch(y, i + 6, line[i]);
            i++;
            mvaddch(y, i + 6, line[i]);
            i++;
            continue;
        }

        if (e->language == LANG_PYTHON && line[i] == '#') {
            in_line_comment = true;
            attron(COLOR_PAIR(COLOR_COMMENT));
            mvaddch(y, i + 6, line[i]);
            i++;
            continue;
        }

        if (e->language == LANG_HTML && i + 3 < len && line[i] == '<' && line[i + 1] == '!' && line[i + 2] == '-' && line[i + 3] == '-') {
            in_line_comment = true;
            attron(COLOR_PAIR(COLOR_COMMENT));
            mvaddch(y, i + 6, line[i]);
            i++;
            mvaddch(y, i + 6, line[i]);
            i++;
            mvaddch(y, i + 6, line[i]);
            i++;
            mvaddch(y, i + 6, line[i]);
            i++;
            continue;
        }

        if ((e->language == LANG_C || e->language == LANG_PYTHON || e->language == LANG_CSS || e->language == LANG_HTML) &&
            (line[i] == '"' || line[i] == '\'')) {
            in_string = true;
            string_delim = line[i];
            attron(COLOR_PAIR(COLOR_STRING));
            mvaddch(y, i + 6, line[i]);
            i++;
            continue;
        }

        if (isdigit(line[i])) {
            attron(COLOR_PAIR(COLOR_NUMBER));
            while (i < len && (isdigit(line[i]) || line[i] == '.')) {
                mvaddch(y, i + 6, line[i]);
                i++;
            }
            attroff(COLOR_PAIR(COLOR_NUMBER));
            continue;
        }

        if (isalpha(line[i]) || line[i] == '_') {
            char word[MAX_LINE_LEN];
            int j = 0;
            while (i < len && (isalnum(line[i]) || line[i] == '_') && j < MAX_LINE_LEN - 1) {
                word[j++] = line[i++];
            }
            word[j] = '\0';
            bool is_keyword = false;
            const char **keywords = NULL;
            if (e->language == LANG_HTML) keywords = html_keywords;
            else if (e->language == LANG_CSS) keywords = css_keywords;
            else if (e->language == LANG_C) keywords = c_keywords;
            else if (e->language == LANG_PYTHON) keywords = python_keywords;

            if (keywords) {
                for (int k = 0; keywords[k]; k++) {
                    if (strcmp(word, keywords[k]) == 0) {
                        is_keyword = true;
                        break;
                    }
                }
            }

            if (is_keyword) {
                attron(COLOR_PAIR(COLOR_KEYWORD));
                for (int k = 0; k < j; k++) {
                    mvaddch(y, i - j + k + 6, word[k]);
                }
                attroff(COLOR_PAIR(COLOR_KEYWORD));
            } else {
                for (int k = 0; k < j; k++) {
                    mvaddch(y, i - j + k + 6, word[k]);
                }
            }
            continue;
        }

        mvaddch(y, i + 6, line[i]);
        i++;
    }

    e->in_multiline_comment = in_comment;
}

void draw(Editor *e) {
    clear();
    int display_lines = e->max_y - 1;
    if (e->top_line < 0) e->top_line = 0;
    if (e->top_line > e->num_lines - 1) e->top_line = e->num_lines - 1;
    for (int i = 0; i < display_lines && i + e->top_line < e->num_lines; i++) {
        int line_num = i + e->top_line + 1;
        mvprintw(i, 0, "%4d: ", line_num);
        highlight_line(e, e->lines[i + e->top_line], i);
    }
    mvprintw(e->max_y - 1, 0, "%.*s", e->max_x - 1, e->message);
    if (e->cursor_y >= e->top_line && e->cursor_y < e->top_line + display_lines) {
        move(e->cursor_y - e->top_line, e->cursor_x + 6);
    } else {
        if (e->cursor_y < e->top_line) e->top_line = e->cursor_y;
        if (e->cursor_y >= e->top_line + display_lines) e->top_line = e->cursor_y - display_lines + 1;
        move(e->cursor_y - e->top_line, e->cursor_x + 6);
    }
    refresh();
}

void load_file(Editor *e, const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        snprintf(e->message, sizeof(e->message), "Error: Cannot open %s", filename);
        return;
    }
    int buf = e->current_buffer;
    for (int i = 0; i < e->num_lines_buf[buf]; i++) free(e->buffers[buf][i]);
    e->num_lines_buf[buf] = 0;
    char line[MAX_LINE_LEN];
    while (fgets(line, MAX_LINE_LEN, f) && e->num_lines_buf[buf] < MAX_LINES) {
        line[strcspn(line, "\n")] = '\0';
        e->buffers[buf][e->num_lines_buf[buf]] = STRDUP(line);
        e->num_lines_buf[buf]++;
    }
    fclose(f);
    if (e->num_lines_buf[buf] == 0) {
        e->buffers[buf][0] = STRDUP("");
        e->num_lines_buf[buf] = 1;
    }
    e->filenames[buf] = strdup(filename);
    e->lines = e->buffers[buf];
    e->num_lines = e->num_lines_buf[buf];
    e->filename = e->filenames[buf];
    e->cursor_x = e->cursor_y = e->top_line = 0;
    detect_language(e);
    snprintf(e->message, sizeof(e->message), "Loaded %s", filename);
}

void save_file(Editor *e) {
    if (!e->filename) {
        snprintf(e->message, sizeof(e->message), "Enter filename to save: ");
        draw(e);
        echo();
        char filename[MAX_FILENAME_LEN];
        mvgetnstr(e->max_y - 1, strlen(e->message), filename, MAX_FILENAME_LEN - 1);
        noecho();
        filename[MAX_FILENAME_LEN - 1] = '\0';
        if (filename[0] == '\0' || strchr(filename, '\n')) {
            snprintf(e->message, sizeof(e->message), "Invalid filename");
            return;
        }
        e->filenames[e->current_buffer] = strdup(filename);
        e->filename = e->filenames[e->current_buffer];
        detect_language(e);
    }
    FILE *f = fopen(e->filename, "w");
    if (!f) {
        snprintf(e->message, sizeof(e->message), "Error: Cannot save %s", e->filename);
        return;
    }
    for (int i = 0; i < e->num_lines; i++) {
        fprintf(f, "%s\n", e->lines[i]);
    }
    fclose(f);
    snprintf(e->message, sizeof(e->message), "Saved %s", e->filename);
}

void add_undo(Editor *e, const char *action, int x, int y, char data, char *bulk_data, int line_count) {
    if (e->undo_count >= e->undo_size) return;
    e->undo_stack[e->undo_count].action = strdup(action);
    e->undo_stack[e->undo_count].x = x;
    e->undo_stack[e->undo_count].y = y;
    e->undo_stack[e->undo_count].data = data;
    e->undo_stack[e->undo_count].bulk_data = bulk_data ? STRDUP(bulk_data) : NULL;
    e->undo_stack[e->undo_count].line_count = line_count;
    e->undo_count++;
}

void undo(Editor *e) {
    if (e->undo_count == 0) {
        snprintf(e->message, sizeof(e->message), "Nothing to undo");
        return;
    }
    e->undo_count--;
    char *action = e->undo_stack[e->undo_count].action;
    int x = e->undo_stack[e->undo_count].x, y = e->undo_stack[e->undo_count].y;
    char data = e->undo_stack[e->undo_count].data;
    if (strcmp(action, "insert") == 0) {
        char *line = e->lines[y];
        memmove(line + x, line + x + 1, (STRLEN(line + x) + 1) * sizeof(char));
        e->cursor_x = x;
        e->cursor_y = y;
    } else if (strcmp(action, "delete") == 0 || strcmp(action, "delete_right") == 0) {
        char *line = e->lines[y];
        char *new_line = malloc((STRLEN(line) + 2) * sizeof(char));
        STRNCPY(new_line, line, x);
        new_line[x] = data;
        STRCPY(new_line + x + 1, line + x);
        free(line);
        e->lines[y] = new_line;
        e->cursor_x = x;
        e->cursor_y = y;
    } else if (strcmp(action, "newline") == 0) {
        char *line = e->lines[y];
        char *next_line = e->lines[y + 1];
        char *new_line = malloc((STRLEN(line) + STRLEN(next_line) + 1) * sizeof(char));
        STRCPY(new_line, line);
        STRCAT(new_line, next_line);
        free(line);
        free(next_line);
        e->lines[y] = new_line;
        memmove(&e->lines[y + 1], &e->lines[y + 2], (e->num_lines - y - 2) * sizeof(char *));
        e->num_lines--;
        e->cursor_x = x;
        e->cursor_y = y;
    } else if (strcmp(action, "bulk_insert") == 0 || strcmp(action, "delete_word") == 0) {
        int line_count = e->undo_stack[e->undo_count].line_count;
        for (int i = 0; i < line_count; i++) {
            free(e->lines[y + i]);
        }
        memmove(&e->lines[y], &e->lines[y + line_count], (e->num_lines - y - line_count) * sizeof(char *));
        e->num_lines -= line_count;
        e->cursor_x = x;
        e->cursor_y = y;
        free(e->undo_stack[e->undo_count].bulk_data);
        e->undo_stack[e->undo_count].bulk_data = NULL;
    }
    free(action);
    e->num_lines_buf[e->current_buffer] = e->num_lines;
    snprintf(e->message, sizeof(e->message), "Undo performed");
    draw(e);
}

void insert_char(Editor *e, char c, bool redraw) {
    if (!ISPRINT(c)) return;
    char *line = e->lines[e->cursor_y];
    if (STRLEN(line) >= MAX_LINE_LEN - 1) return;
    add_undo(e, "insert", e->cursor_x, e->cursor_y, c, NULL, 0);
    char *new_line = malloc((STRLEN(line) + 2) * sizeof(char));
    STRNCPY(new_line, line, e->cursor_x);
    new_line[e->cursor_x] = c;
    STRCPY(new_line + e->cursor_x + 1, line + e->cursor_x);
    free(line);
    e->lines[e->cursor_y] = new_line;
    e->cursor_x++;
    if (redraw) draw(e);
}

void delete_char(Editor *e) {
    char *line = e->lines[e->cursor_y];
    if (e->cursor_x == 0 && e->cursor_y == 0) return;
    if (e->cursor_x > 0) {
        add_undo(e, "delete", e->cursor_x - 1, e->cursor_y, line[e->cursor_x - 1], NULL, 0);
        memmove(line + e->cursor_x - 1, line + e->cursor_x, (STRLEN(line + e->cursor_x) + 1) * sizeof(char));
        e->cursor_x--;
    } else if (e->cursor_y > 0) {
        char *prev_line = e->lines[e->cursor_y - 1];
        e->cursor_x = STRLEN(prev_line);
        char *new_line = malloc((STRLEN(prev_line) + STRLEN(line) + 1) * sizeof(char));
        STRCPY(new_line, prev_line);
        STRCAT(new_line, line);
        free(prev_line);
        free(line);
        e->lines[e->cursor_y - 1] = new_line;
        memmove(&e->lines[e->cursor_y], &e->lines[e->cursor_y + 1], (e->num_lines - e->cursor_y - 1) * sizeof(char *));
        e->num_lines--;
        e->cursor_y--;
    }
    e->num_lines_buf[e->current_buffer] = e->num_lines;
    draw(e);
}

void delete_char_right(Editor *e) {
    char *line = e->lines[e->cursor_y];
    if (e->cursor_x < STRLEN(line)) {
        add_undo(e, "delete_right", e->cursor_x, e->cursor_y, line[e->cursor_x], NULL, 0);
        memmove(line + e->cursor_x, line + e->cursor_x + 1, (STRLEN(line + e->cursor_x + 1) + 1) * sizeof(char));
    } else if (e->cursor_y < e->num_lines - 1) {
        char *next_line = e->lines[e->cursor_y + 1];
        char *new_line = malloc((STRLEN(line) + STRLEN(next_line) + 1) * sizeof(char));
        STRCPY(new_line, line);
        STRCAT(new_line, next_line);
        free(line);
        free(next_line);
        e->lines[e->cursor_y] = new_line;
        memmove(&e->lines[e->cursor_y + 1], &e->lines[e->cursor_y + 2], (e->num_lines - e->cursor_y - 2) * sizeof(char *));
        e->num_lines--;
    } else {
        return;
    }
    e->num_lines_buf[e->current_buffer] = e->num_lines;
    draw(e);
}

void delete_word_left(Editor *e) {
    int orig_x = e->cursor_x, orig_y = e->cursor_y;
    char *line = e->lines[e->cursor_y];
    if (e->cursor_x == 0 && e->cursor_y == 0) return;

    int new_x = e->cursor_x, new_y = e->cursor_y;
    while (new_x > 0 && ISSPACE(line[new_x - 1])) new_x--;
    while (new_x > 0 && !ISALNUM(line[new_x - 1]) && !ISSPACE(line[new_x - 1])) new_x--;
    while (new_x > 0 && ISALNUM(line[new_x - 1])) new_x--;
    if (new_x == 0 && new_y > 0) {
        new_y--;
        line = e->lines[new_y];
        new_x = STRLEN(line);
        while (new_x > 0 && ISSPACE(line[new_x - 1])) new_x--;
        while (new_x > 0 && !ISALNUM(line[new_x - 1]) && !ISSPACE(line[new_x - 1])) new_x--;
    }

    char *deleted = NULL;
    int deleted_len = 0;
    if (new_y == orig_y) {
        deleted_len = orig_x - new_x;
        deleted = STRNDUP(line + new_x, deleted_len);
    } else {
        deleted_len = STRLEN(e->lines[new_y]) - new_x + 1 + orig_x;
        deleted = malloc(deleted_len + 1);
        deleted[0] = '\0';
        STRCAT(deleted, e->lines[new_y] + new_x);
        STRCAT(deleted, NEWLINE);
        STRCAT(deleted, e->lines[orig_y]);
    }

    if (new_y == orig_y) {
        memmove(line + new_x, line + orig_x, (STRLEN(line + orig_x) + 1) * sizeof(char));
        e->cursor_x = new_x;
    } else {
        char *new_line = malloc((new_x + STRLEN(e->lines[orig_y]) + 1) * sizeof(char));
        STRNCPY(new_line, e->lines[new_y], new_x);
        new_line[new_x] = '\0';
        STRCAT(new_line, e->lines[orig_y]);
        free(e->lines[new_y]);
        e->lines[new_y] = new_line;
        memmove(&e->lines[new_y + 1], &e->lines[orig_y], (e->num_lines - orig_y) * sizeof(char *));
        e->num_lines -= (orig_y - new_y);
        e->cursor_y = new_y;
        e->cursor_x = new_x;
    }

    add_undo(e, "delete_word", e->cursor_x, e->cursor_y, '\0', deleted, orig_y - new_y + 1);
    e->num_lines_buf[e->current_buffer] = e->num_lines;
    draw(e);
}

void delete_word_right(Editor *e) {
    int orig_x = e->cursor_x, orig_y = e->cursor_y;
    char *line = e->lines[e->cursor_y];
    if (e->cursor_y == e->num_lines - 1 && e->cursor_x == STRLEN(line)) return;

    int new_x = e->cursor_x, new_y = e->cursor_y;
    while (line[new_x] && ISALNUM(line[new_x])) new_x++;
    while (line[new_x] && !ISALNUM(line[new_x]) && !ISSPACE(line[new_x])) new_x++;
    while (line[new_x] && ISSPACE(line[new_x])) new_x++;
    if (!line[new_x] && new_y < e->num_lines - 1) {
        new_y++;
        new_x = 0;
        line = e->lines[new_y];
    }

    char *deleted = NULL;
    int deleted_len = 0;
    if (new_y == orig_y) {
        deleted_len = new_x - orig_x;
        deleted = STRNDUP(line + orig_x, deleted_len);
    } else {
        deleted_len = STRLEN(line) - orig_x + 1 + STRLEN(e->lines[new_y]);
        deleted = malloc(deleted_len + 1);
        deleted[0] = '\0';
        STRCAT(deleted, line + orig_x);
        STRCAT(deleted, NEWLINE);
        STRCAT(deleted, e->lines[new_y]);
    }

    if (new_y == orig_y) {
        memmove(line + orig_x, line + new_x, (STRLEN(line + new_x) + 1) * sizeof(char));
    } else {
        char *new_line = malloc((orig_x + STRLEN(e->lines[new_y]) + 1) * sizeof(char));
        STRNCPY(new_line, line, orig_x);
        new_line[orig_x] = '\0';
        STRCAT(new_line, e->lines[new_y]);
        free(e->lines[orig_y]);
        e->lines[orig_y] = new_line;
        memmove(&e->lines[orig_y + 1], &e->lines[new_y + 1], (e->num_lines - new_y - 1) * sizeof(char *));
        e->num_lines -= (new_y - orig_y);
    }

    add_undo(e, "delete_word", e->cursor_x, e->cursor_y, '\0', deleted, new_y - orig_y + 1);
    e->num_lines_buf[e->current_buffer] = e->num_lines;
    draw(e);
}

void insert_newline(Editor *e, bool redraw) {
    if (e->num_lines >= MAX_LINES) return;
    add_undo(e, "newline", e->cursor_x, e->cursor_y, '\0', NULL, 0);
    char *line = e->lines[e->cursor_y];
    char *new_line = malloc(MAX_LINE_LEN * sizeof(char));
    STRCPY(new_line, line + e->cursor_x);
    line[e->cursor_x] = '\0';
    memmove(&e->lines[e->cursor_y + 2], &e->lines[e->cursor_y + 1], (e->num_lines - e->cursor_y - 1) * sizeof(char *));
    e->lines[e->cursor_y + 1] = new_line;
    e->num_lines++;
    e->cursor_y++;
    e->cursor_x = 0;
    e->num_lines_buf[e->current_buffer] = e->num_lines;
    if (redraw) draw(e);
}

void insert_lines(Editor *e, char **new_lines, int line_count, bool redraw) {
    if (e->num_lines + line_count > MAX_LINES) return;
    char *bulk_data = malloc(MAX_LINE_LEN * line_count * sizeof(char));
    bulk_data[0] = '\0';
    for (int i = 0; i < line_count; i++) {
        STRCAT(bulk_data, new_lines[i]);
        if (i < line_count - 1) STRCAT(bulk_data, NEWLINE);
    }
    add_undo(e, "bulk_insert", e->cursor_x, e->cursor_y, '\0', bulk_data, line_count);
    char *line = e->lines[e->cursor_y];
    char *line_tail = malloc(MAX_LINE_LEN * sizeof(char));
    STRCPY(line_tail, line + e->cursor_x);
    line[e->cursor_x] = '\0';
    memmove(&e->lines[e->cursor_y + line_count + 1], &e->lines[e->cursor_y + 1], (e->num_lines - e->cursor_y - 1) * sizeof(char *));
    for (int i = 0; i < line_count; i++) {
        e->lines[e->cursor_y + i] = STRDUP(new_lines[i]);
    }
    e->lines[e->cursor_y + line_count] = line_tail;
    e->num_lines += line_count;
    e->cursor_y += line_count - 1;
    e->cursor_x = STRLEN(new_lines[line_count - 1]);
    if (line_count == 1 && e->cursor_x == 0) {
        e->cursor_y++;
        e->cursor_x = 0;
    }
    e->num_lines_buf[e->current_buffer] = e->num_lines;
    if (redraw) draw(e);
}

void move_cursor_up(Editor *e) {
    if (e->cursor_y > 0) {
        e->cursor_y--;
        e->cursor_x = e->cursor_x < STRLEN(e->lines[e->cursor_y]) ? e->cursor_x : STRLEN(e->lines[e->cursor_y]);
        if (e->cursor_y < e->top_line) e->top_line = e->cursor_y;
    }
    draw(e);
}

void move_cursor_down(Editor *e) {
    if (e->cursor_y < e->num_lines - 1) {
        e->cursor_y++;
        e->cursor_x = e->cursor_x < STRLEN(e->lines[e->cursor_y]) ? e->cursor_x : STRLEN(e->lines[e->cursor_y]);
        if (e->cursor_y >= e->top_line + e->max_y - 1) e->top_line = e->cursor_y - e->max_y + 2;
    }
    draw(e);
}

void move_cursor_left(Editor *e) {
    if (e->cursor_x > 0) e->cursor_x--;
    draw(e);
}

void move_cursor_right(Editor *e) {
    if (e->cursor_x < STRLEN(e->lines[e->cursor_y])) e->cursor_x++;
    draw(e);
}

void move_cursor_backward_word(Editor *e) {
    if (e->cursor_x == 0 && e->cursor_y == 0) return;
    while (e->cursor_x > 0 && ISSPACE(e->lines[e->cursor_y][e->cursor_x - 1])) {
        e->cursor_x--;
    }
    while (e->cursor_x > 0 && !ISALNUM(e->lines[e->cursor_y][e->cursor_x - 1]) && !ISSPACE(e->lines[e->cursor_y][e->cursor_x - 1])) {
        e->cursor_x--;
    }
    while (e->cursor_x > 0 && ISALNUM(e->lines[e->cursor_y][e->cursor_x - 1])) {
        e->cursor_x--;
    }
    if (e->cursor_x == 0 && e->cursor_y > 0) {
        e->cursor_y--;
        e->cursor_x = STRLEN(e->lines[e->cursor_y]);
        while (e->cursor_x > 0 && ISSPACE(e->lines[e->cursor_y][e->cursor_x - 1])) {
            e->cursor_x--;
        }
        while (e->cursor_x > 0 && !ISALNUM(e->lines[e->cursor_y][e->cursor_x - 1]) && !ISSPACE(e->lines[e->cursor_y][e->cursor_x - 1])) {
            e->cursor_x--;
        }
    }
    draw(e);
}

void move_cursor_forward_word(Editor *e) {
    char *line = e->lines[e->cursor_y];
    while (line[e->cursor_x] && ISALNUM(line[e->cursor_x])) {
        e->cursor_x++;
    }
    while (line[e->cursor_x] && !ISALNUM(line[e->cursor_x]) && !ISSPACE(line[e->cursor_x])) {
        e->cursor_x++;
    }
    while (line[e->cursor_x] && ISSPACE(line[e->cursor_x])) {
        e->cursor_x++;
    }
    if (!line[e->cursor_x] && e->cursor_y < e->num_lines - 1) {
        e->cursor_y++;
        e->cursor_x = 0;
    }
    draw(e);
}

void move_cursor_backward_paragraph(Editor *e) {
    while (e->cursor_y > 0) {
        e->cursor_y--;
        if (STRLEN(e->lines[e->cursor_y]) == 0) {
            e->cursor_x = 0;
            break;
        }
    }
    e->cursor_x = 0;
    if (e->cursor_y < e->top_line) e->top_line = e->cursor_y;
    draw(e);
}

void move_cursor_forward_paragraph(Editor *e) {
    while (e->cursor_y < e->num_lines - 1) {
        e->cursor_y++;
        if (STRLEN(e->lines[e->cursor_y]) == 0) {
            e->cursor_x = 0;
            break;
        }
    }
    e->cursor_x = 0;
    if (e->cursor_y >= e->top_line + e->max_y - 1) e->top_line = e->cursor_y - e->max_y + 2;
    draw(e);
}

void move_cursor_beginning_of_line(Editor *e) {
    e->cursor_x = 0;
    draw(e);
}

void move_cursor_end_of_line(Editor *e) {
    e->cursor_x = STRLEN(e->lines[e->cursor_y]);
    draw(e);
}

void kill_line(Editor *e) {
    if (e->mark_active) {
        delete_region(e);
        return;
    }
    char *line = e->lines[e->cursor_y];
    if (e->kill_ring[0]) free(e->kill_ring[0]);
    e->kill_ring[0] = STRDUP(line + e->cursor_x);
    line[e->cursor_x] = '\0';
    e->num_lines_buf[e->current_buffer] = e->num_lines;
    snprintf(e->message, sizeof(e->message), "Line cut to kill-ring");
    draw(e);
}

void yank(Editor *e) {
    if (!e->kill_ring[0]) {
        snprintf(e->message, sizeof(e->message), "Nothing to yank");
        return;
    }
    char *text = e->kill_ring[0];
    char **new_lines = malloc(MAX_LINES * sizeof(char *));
    int line_count = 0;
    char *start = text;
    char *end;
    while ((end = STRCHR(start, '\n')) && line_count < MAX_LINES) {
        new_lines[line_count] = STRNDUP(start, end - start);
        line_count++;
        start = end + 1;
    }
    if (*start && line_count < MAX_LINES) {
        new_lines[line_count] = STRDUP(start);
        line_count++;
    }
    if (line_count == 0) {
        new_lines[0] = STRDUP(text);
        line_count = 1;
    }
    insert_lines(e, new_lines, line_count, true);
    for (int i = 0; i < line_count; i++) free(new_lines[i]);
    free(new_lines);
    snprintf(e->message, sizeof(e->message), "Yanked from kill-ring");
}

void set_mark(Editor *e) {
    e->mark_x = e->cursor_x;
    e->mark_y = e->cursor_y;
    e->mark_active = true;
    snprintf(e->message, sizeof(e->message), "Mark set");
}

void delete_region(Editor *e) {
    if (!e->mark_active) {
        snprintf(e->message, sizeof(e->message), "No region selected");
        return;
    }
    int start_y, start_x, end_y, end_x;
    if (e->mark_y < e->cursor_y || (e->mark_y == e->cursor_y && e->mark_x <= e->cursor_x)) {
        start_y = e->mark_y;
        start_x = e->mark_x;
        end_y = e->cursor_y;
        end_x = e->cursor_x;
    } else {
        start_y = e->cursor_y;
        start_x = e->cursor_x;
        end_y = e->mark_y;
        end_x = e->mark_x;
    }
    if (e->kill_ring[0]) free(e->kill_ring[0]);
    e->kill_ring[0] = NULL;
    if (start_y == end_y) {
        char *line = e->lines[start_y];
        int len = end_x - start_x;
        e->kill_ring[0] = STRNDUP(line + start_x, len);
        memmove(line + start_x, line + end_x, (STRLEN(line + end_x) + 1) * sizeof(char));
        e->cursor_y = start_y;
        e->cursor_x = start_x;
    } else {
        char *first = e->lines[start_y] + start_x;
        size_t kill_len = STRLEN(first) + 1;
        for (int y = start_y + 1; y < end_y; y++) {
            kill_len += STRLEN(e->lines[y]) + 1;
        }
        char *last = STRNDUP(e->lines[end_y], end_x);
        kill_len += STRLEN(last);
        e->kill_ring[0] = malloc((kill_len + 1) * sizeof(char));
        e->kill_ring[0][0] = '\0';
        STRCAT(e->kill_ring[0], first);
        STRCAT(e->kill_ring[0], NEWLINE);
        for (int y = start_y + 1; y < end_y; y++) {
            STRCAT(e->kill_ring[0], e->lines[y]);
            STRCAT(e->kill_ring[0], NEWLINE);
        }
        STRCAT(e->kill_ring[0], last);
        free(last);
        char *new_line = malloc((STRLEN(e->lines[start_y]) - start_x + end_x + 1) * sizeof(char));
        STRNCPY(new_line, e->lines[start_y], start_x);
        new_line[start_x] = '\0';
        STRCAT(new_line, e->lines[end_y] + end_x);
        free(e->lines[start_y]);
        e->lines[start_y] = new_line;
        for (int y = start_y + 1; y <= end_y; y++) {
            free(e->lines[y]);
        }
        memmove(&e->lines[start_y + 1], &e->lines[end_y + 1], (e->num_lines - end_y - 1) * sizeof(char *));
        e->num_lines -= (end_y - start_y);
        e->cursor_y = start_y;
        e->cursor_x = start_x;
    }
    e->mark_active = false;
    e->num_lines_buf[e->current_buffer] = e->num_lines;
    snprintf(e->message, sizeof(e->message), "Region cut to kill-ring");
    draw(e);
}

void start_search(Editor *e) {
    e->searching = true;
    e->search_query[0] = '\0';
    snprintf(e->message, sizeof(e->message), "Search: ");
    draw(e);
}

void update_search(Editor *e, int c) {
    if (!e->searching) return;
    int len = strlen(e->search_query);
    if (c == 27 || c == '\n') {
        e->searching = false;
        e->search_query[0] = '\0';
        snprintf(e->message, sizeof(e->message), "Search ended");
        draw(e);
        return;
    }
    if (c == 127 || c == KEY_BACKSPACE) {
        if (len > 0) e->search_query[len - 1] = '\0';
    } else if (isprint(c) && len < sizeof(e->search_query) - 1) {
        e->search_query[len] = (char)c;
        e->search_query[len + 1] = '\0';
    }
    char buffer[MAX_LINE_LEN];
    for (int y = e->cursor_y; y < e->num_lines; y++) {
        snprintf(buffer, MAX_LINE_LEN, "%s", e->lines[y]);
        char *match = strstr(buffer + (y == e->cursor_y ? e->cursor_x : 0), e->search_query);
        if (match) {
            e->cursor_y = y;
            e->cursor_x = match - buffer;
            if (e->cursor_y >= e->top_line + e->max_y - 1) e->top_line = e->cursor_y - e->max_y + 2;
            break;
        }
    }
    snprintf(e->message, sizeof(e->message), "Search: %s", e->search_query);
    draw(e);
}

void switch_buffer(Editor *e) {
    e->current_buffer = 1 - e->current_buffer;
    e->lines = e->buffers[e->current_buffer];
    e->num_lines = e->num_lines_buf[e->current_buffer];
    e->filename = e->filenames[e->current_buffer];
    e->cursor_x = e->cursor_y = e->top_line = 0;
    detect_language(e);
    snprintf(e->message, sizeof(e->message), "Switched to buffer %d", e->current_buffer + 1);
    draw(e);
}

void show_info(Editor *e) {
    snprintf(e->message, sizeof(e->message), "Micrn Editor, Version 1.0, Created by Genius, 2025");
    draw(e);
}

void handle_input(Editor *e, int ch) {
    static bool expecting_alt = false;
    static time_t ctrl_x_time = 0;
    static bool expecting_ctrl_x = false;

    e->message[0] = '\0';

    if (e->searching) {
        update_search(e, ch);
        return;
    }

    if (expecting_alt) {
        expecting_alt = false;
        if (ch == 'b') {
            move_cursor_backward_word(e);
        } else if (ch == 'f') {
            move_cursor_forward_word(e);
        } else if (ch == '{') {
            move_cursor_backward_paragraph(e);
        } else if (ch == '}') {
            move_cursor_forward_paragraph(e);
        } else if (ch == KEY_BACKSPACE || ch == 127) {
            delete_word_left(e);
        } else if (ch == KEY_DC) {
            delete_word_right(e);
        } else {
            snprintf(e->message, sizeof(e->message), "Unknown Alt sequence: %d", ch);
            draw(e);
        }
        return;
    }

    if (expecting_ctrl_x) {
        if (time(NULL) - ctrl_x_time > CTRL_X_TIMEOUT) {
            expecting_ctrl_x = false;
            snprintf(e->message, sizeof(e->message), "Ctrl+X timeout");
            draw(e);
        } else if (ch == CTRL_KEY('s')) {
            save_file(e);
            expecting_ctrl_x = false;
        } else if (ch == CTRL_KEY('c')) {
            cleanup_editor(e);
            exit(0);
        } else if (ch == CTRL_KEY('x')) {
            switch_buffer(e);
            expecting_ctrl_x = false;
        } else {
            snprintf(e->message, sizeof(e->message), "Unknown Ctrl+X sequence: %d", ch);
            expecting_ctrl_x = false;
            draw(e);
        }
        return;
    }

    if (ch == 27) {
        expecting_alt = true;
        return;
    }

    if (ch == CTRL_KEY('x')) {
        expecting_ctrl_x = true;
        ctrl_x_time = time(NULL);
        return;
    }

    if (commands[ch]) {
        commands[ch](e);
        return;
    }

    if (ISPRINT(ch)) {
        insert_char(e, ch, true);
    } else if (ch == '\n' || ch == CTRL_KEY('j')) {
        insert_newline(e, true);
    } else if (ch == KEY_BACKSPACE || ch == 127) {
        delete_char(e);
    } else if (ch == KEY_DC) {
        delete_char_right(e);
    } else if (ch == KEY_UP || ch == CTRL_KEY('p')) {
        move_cursor_up(e);
    } else if (ch == KEY_DOWN || ch == CTRL_KEY('n')) {
        move_cursor_down(e);
    } else if (ch == KEY_LEFT || ch == CTRL_KEY('b')) {
        move_cursor_left(e);
    } else if (ch == KEY_RIGHT || ch == CTRL_KEY('f')) {
        move_cursor_right(e);
    } else if (ch == CTRL_KEY('d')) {
        delete_char(e);
    } else if (ch != ERR) {
        snprintf(e->message, sizeof(e->message), "Unknown key: %d", ch);
        draw(e);
    }
}

int main(int argc, char *argv[]) {
    Editor e = {0};
    init_editor(&e);
    commands[CTRL_KEY('u')] = undo;
    commands[CTRL_KEY('k')] = kill_line;
    commands[CTRL_KEY('y')] = yank;
    commands[0] = set_mark;
    commands[CTRL_KEY('w')] = delete_region;
    commands[CTRL_KEY('s')] = start_search;
    commands[CTRL_KEY('a')] = move_cursor_beginning_of_line;
    commands[CTRL_KEY('e')] = move_cursor_end_of_line;
    commands[CTRL_KEY('i')] = show_info; 
    if (argc > 1) load_file(&e, argv[1]);

    while (1) {
        draw(&e);
        int ch = getch();
        handle_input(&e, ch);
    }

    cleanup_editor(&e);
    return 0;
}
