#include "text_editor.h"

#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include "dialogs.h"

// -------------------------- Configuration -------------------------

#define MENU_HEIGHT 8
#define MENU_WIDTH 23
#define DIALOG_HEIGHT 5
#define DIALOG_WIDTH COLS / 2
#define FILE_VIEW_HEIGHT LINES - 1
#define FILE_VIEW_WIDTH COLS
#define FILE_VIEW_OFFSET_Y 1
#define FILE_VIEW_OFFSET_X 0

#define PATH_INPUT_BUFFER_LEN 256

static const char *menu_labels[MENU_ITEMS_SIZE] = {
    "New file          ",
    "Open file         ",
    "Save file         ",
    "Save file as      ",
    "Close             ",
    "Quit              "
};

// ------------------------------ Macros ----------------------------

/**
 * @brief Check if abort condition is satisfied and abort creation (free already created resources and return error status)
 * 
 * @param expr abort condition
 * @param editor pointer to TextEditor instance to be destroyed on abort
 */
#define ABORT_CREATE(expr, editor) if (expr) { free_text_editor(editor); return NULL; }


// ----------------------- Private declarations ---------------------

/**
 * @brief Render application File menu window.
 * 
 * @param editor pointer to initialized TextEditor instance
 */
void text_editor_render_main_menu(TextEditor *editor);

/**
 * @brief Render application about window.
 * 
 * @param editor pointer to initialized TextEditor instance
 */
void text_editor_render_about_window(TextEditor *editor);

/**
 * @brief Execute file menu action.
 * 
 * @param editor pointer to initialized TextEditor structure
 * @return int 1 if the program should close, 0 otherwise
 */
int text_editor_menu_action(TextEditor *editor);


// ----------------------- Public definitions -----------------------

TextEditor* create_text_editor()
{
    TextEditor *editor = (TextEditor*) malloc(sizeof(TextEditor));

    if (editor == NULL)
    {
        return NULL;
    }

    memset(editor, 0, sizeof(TextEditor));
    editor->current_tab = -1;

    // Top bar
    editor->top_bar_win = newwin(1, COLS, 0, 0);
    ABORT_CREATE(editor->top_bar_win == NULL, editor);
    text_editor_render(editor);

    editor->top_bar_panel = new_panel(editor->top_bar_win);
    ABORT_CREATE(editor->top_bar_panel == NULL, editor);

    // File menu
    editor->menu_win = newwin(MENU_HEIGHT, MENU_WIDTH, (LINES - MENU_HEIGHT) / 2, (COLS - MENU_WIDTH) / 2);
    ABORT_CREATE(editor->menu_win == NULL, editor);

    for (int i = 0; i < MENU_ITEMS_SIZE; i++)
    {
        editor->menu_items[i] = new_item(menu_labels[i], "");
        ABORT_CREATE(editor->menu_items[i] == NULL, editor);
    }

    editor->menu = new_menu(editor->menu_items);
    ABORT_CREATE(editor->menu == NULL, editor);
    text_editor_render_main_menu(editor);

    editor->menu_panel = new_panel(editor->menu_win);
    ABORT_CREATE(editor->menu_panel == NULL, editor);

    // Dialogs window
    editor->dialog_win = newwin(DIALOG_HEIGHT, DIALOG_WIDTH, (LINES - DIALOG_HEIGHT) / 2, (COLS - DIALOG_WIDTH) / 2);
    ABORT_CREATE(editor->dialog_win == NULL, editor);

    editor->dialog_panel = new_panel(editor->dialog_win);
    ABORT_CREATE(editor->dialog_panel == NULL, editor);

    // About window
    editor->about_win = newwin(FILE_VIEW_HEIGHT, FILE_VIEW_WIDTH, FILE_VIEW_OFFSET_Y, FILE_VIEW_OFFSET_X);
    ABORT_CREATE(editor->about_win == NULL, editor);
    text_editor_render_about_window(editor);

    editor->about_panel = new_panel(editor->about_win);
    ABORT_CREATE(editor->about_panel == NULL, editor);

    // Panel ordering
    hide_panel(editor->menu_panel);
    top_panel(editor->about_panel);
    update_panels();
    doupdate();

    return editor;
}

void free_text_editor(TextEditor *editor)
{
    if (editor == NULL)
    {
        return;
    }

    if (editor->tabs != NULL)
    {
        for (int i = 0; i < editor->n_tabs; i++)
        {
            free_file_view(editor->tabs[i]);
        }

        free(editor->tabs);
    }

    if (editor->top_bar_panel != NULL)
    {
        del_panel(editor->top_bar_panel);
    }

    if (editor->top_bar_win != NULL)
    {
        delwin(editor->top_bar_win);
    }

    if (editor->menu_panel != NULL)
    {
        del_panel(editor->menu_panel);
    }

    if (editor->menu != NULL)
    {
        unpost_menu(editor->menu);
        free_menu(editor->menu);
    }

    for (int i = 0; i < MENU_ITEMS_SIZE; i++)
    {
        if (editor->menu_items[i] != NULL)
        {
            free_item(editor->menu_items[i]);
        }
    }

    if (editor->menu_win != NULL)
    {
        delwin(editor->menu_win);
    }

    if (editor->dialog_panel != NULL)
    {
        del_panel(editor->dialog_panel);
    }

    if (editor->dialog_win != NULL)
    {
        delwin(editor->dialog_win);
    }

    if (editor->about_panel != NULL)
    {
        del_panel(editor->about_panel);
    }

    if (editor->about_win != NULL)
    {
        delwin(editor->about_win);
    }

    if (editor->clipboard != NULL)
    {
        free(editor->clipboard);
    }

    free(editor);
}

int text_editor_new_tab(TextEditor *editor)
{
    if (editor->tabs == NULL)
    {
        editor->tabs = (FileView**) malloc(sizeof(FileView*));

        if (editor->tabs == NULL)
        {
            return 1;
        }
    }
    else
    {
        FileView **new_tabs = realloc(editor->tabs, (editor->n_tabs + 1) * sizeof(FileView*));
        if (new_tabs == NULL)
        {
            return 1;
        }

        editor->tabs = new_tabs;
    }

    editor->tabs[editor->n_tabs] = create_file_view(FILE_VIEW_HEIGHT, FILE_VIEW_WIDTH, FILE_VIEW_OFFSET_Y, FILE_VIEW_OFFSET_X);

    if (editor->tabs[editor->n_tabs] == NULL)
    {
        return 1;
    }

    editor->current_tab = editor->n_tabs;
    editor->n_tabs++;

    return 0;
}

int text_editor_close_tab(TextEditor *editor)
{
    if (editor->tabs == NULL)
    {
        return 0;
    }

    FileView *current_view = text_editor_get_current_view(editor);
    free_file_view(current_view);

    // Shift remaining tabs
    for (int i = editor->current_tab; i < editor->n_tabs - 1; i++)
    {
        editor->tabs[i] = editor->tabs[i + 1];
    }
    editor->n_tabs--;

    if (editor->n_tabs == 0)
    {
        free(editor->tabs);
        editor->tabs = NULL;
        editor->current_tab = -1;
        return 0;
    }

    FileView **new_tabs = realloc(editor->tabs, editor->n_tabs * sizeof(FileView*));

    if (new_tabs == NULL)
    {
        return 1;
    }

    editor->tabs = new_tabs;
    editor->current_tab--;
    if (editor->current_tab < 0)
    {
        editor->current_tab = 0;
    }
    return 0;
}

int text_editor_load_file(TextEditor *editor)
{
    if (text_editor_new_tab(editor) != 0)
    {
        return 1;
    }


    char buffer[PATH_INPUT_BUFFER_LEN];
    if (input_dialog(editor->dialog_panel, "Enter file path", NULL, buffer, PATH_INPUT_BUFFER_LEN) == 0)
    {
        // User canceled file path input
        return text_editor_close_tab(editor);
    }

    if (file_view_load_file(editor->tabs[editor->current_tab], buffer) != 0)
    {
        // Error while loading file
        alert_dialog(editor->dialog_panel, "Error while loading file");
        return text_editor_close_tab(editor);
    }

    return 0;
}

void text_editor_save_file(TextEditor *editor, int save_as)
{
    FileView *current_view = text_editor_get_current_view(editor);
    
    if (!save_as && file_view_get_file_path(current_view) == NULL)
    {
        text_editor_save_file(editor, 1);
        return;
    }

    char *save_path = NULL;
    if (save_as)
    {
        char buffer[PATH_INPUT_BUFFER_LEN];
        const char *file_path = file_view_get_file_path(current_view);
        const char *initial_data = file_path != NULL ? file_path : "";

        if (input_dialog(editor->dialog_panel, "Enter file path", initial_data, buffer, PATH_INPUT_BUFFER_LEN) == 0)
        {
            // User canceled file path input
            return;
        }

        save_path = buffer;
    }
    // file_view_save_file(current_view, save_path);
    // cu tratare erori
}

void text_editor_render(TextEditor *editor)
{
    WINDOW *win = editor->top_bar_win;
    int menu_offset = 11;
    int pos, i, start_i;
    int width = getmaxx(win);

    werase(win);
    wbkgd(win, COLOR_PAIR(4));

    // File menu
    wattron(win, COLOR_PAIR(5));
    mvwaddstr(win, 0, 0, " File (F1) ");
    wattroff(win, COLOR_PAIR(5));
    
    // Compute start position
    pos = menu_offset + 1;
    start_i = 0;

    for (i = 0; i <= editor->current_tab; i++)
    {
        const char *title = file_view_get_title(editor->tabs[i]);
        pos += strlen(title) + (i == 0 ? 2 : 3);
    }

    while (pos >= width - 5 && start_i < editor->current_tab)
    {
        const char *title = file_view_get_title(editor->tabs[start_i]);
        pos -= strlen(title) + (start_i == 0 ? 2 : 3);
        start_i++;
    }

    // Prev indicator
    wattron(win, start_i == 0 ? A_DIM : A_BOLD);
    mvwaddch(win, 0, menu_offset + 1, '<');
    wattroff(win, start_i == 0 ? A_DIM : A_BOLD);

    // Show tabs
    pos = menu_offset + 3;
    for (i = start_i; i < editor->n_tabs && pos < width - 3; i++)
    {
        pos++;
        if (i == editor->current_tab)
        {
            wattron(win, A_STANDOUT);
        }

        const char *title = file_view_get_title(editor->tabs[i]);
        mvwaddch(win, 0, pos - 1, ' ');
        mvwaddnstr(win, 0, pos, title, width - 3 - pos);
        mvwaddch(win, 0, pos + strlen(title), ' ');
        wattroff(win, A_STANDOUT);
        pos += strlen(title) + 2;

        if (i < editor->n_tabs - 1 && pos < width - 2)
        {
            mvwaddch(win, 0, pos - 1, ACS_VLINE);
        }
    }

    // Next indicator
    wattron(win, i == editor->n_tabs ? A_DIM : A_BOLD);
    mvwaddch(win, 0, width - 2, '>');
    wattroff(win, i == editor->n_tabs ? A_DIM : A_BOLD);
}

int text_editor_handle_input(TextEditor *editor, int input)
{
    int ret = 0;
    FileView *current_view = text_editor_get_current_view(editor);

    if (!panel_hidden(editor->menu_panel))
    {
        wrefresh(editor->menu_win);
        switch (input)
        {
            case KEY_DOWN:
                menu_driver(editor->menu, REQ_DOWN_ITEM);
                wrefresh(editor->menu_win);
                break;

            case KEY_UP:
                menu_driver(editor->menu, REQ_UP_ITEM);
                wrefresh(editor->menu_win);
                break;

            case 10:
            case KEY_ENTER:
                hide_panel(editor->menu_panel);
                ret = text_editor_menu_action(editor);
                break;

            case KEY_F(1):
                hide_panel(editor->menu_panel);
                break;
        }
    }
    else
    {
        switch (input)
        {
            case KEY_F(1):
                show_panel(editor->menu_panel);
                top_panel(editor->menu_panel);
                break;

            case 9:
                if (editor->n_tabs > 0)
                {
                    editor->current_tab = (editor->current_tab + 1) % editor->n_tabs;
                    text_editor_render(editor);
                    current_view = text_editor_get_current_view(editor);
                }

                if (current_view != NULL)
                {
                    top_panel(current_view->panel);
                    file_view_render(current_view);
                }
                break;

            default:
                if (current_view != NULL)
                {
                    file_view_handle_input(current_view, input);
                    file_view_render(current_view);
                }
                break;
        }
    }

    update_panels();
    doupdate();
    // wrefresh(editor->menu_win);
    // wrefresh(editor->dialog_win);
    // wrefresh(editor->top_bar_win);
    // refresh();
    return ret;
}

FileView *text_editor_get_current_view(TextEditor *editor)
{
    if (editor->tabs == NULL)
    {
        return NULL;
    }

    return editor->tabs[editor->current_tab];
}


// ---------------------- Private definitions -----------------------

void text_editor_render_main_menu(TextEditor *editor)
{
    render_dialog_window(editor->menu_win, "File menu");
    set_menu_win(editor->menu, editor->menu_win);
    set_menu_sub(editor->menu, derwin(editor->menu_win, MENU_HEIGHT - 2, MENU_WIDTH - 3, 1, 2));
    menu_opts_off(editor->menu, O_SHOWDESC);
    set_menu_mark(editor->menu, ">");
    post_menu(editor->menu);
}

void text_editor_render_about_window(TextEditor *editor)
{
    wattron(editor->about_win, A_BOLD);
    mvwprintw(editor->about_win, 1, 1, "Ncurses Text Editor");
    wattroff(editor->about_win, A_BOLD);
    mvwprintw(editor->about_win, 2, 1, "(c) 2024 Butufei Tudor-David");
    mvwprintw(editor->about_win, 4, 1, "Open file menu to get started.");
}

int text_editor_menu_action(TextEditor *editor)
{
    int ret = 0;
    switch (item_index(current_item(editor->menu)))
    {
        case 0:
            ret = text_editor_new_tab(editor);
            break;

        case 1:
            ret = text_editor_load_file(editor);
            break;

        case 2:
            text_editor_save_file(editor, 0);
            break;

        case 3:
            text_editor_save_file(editor, 1);
            break;

        case 4:
            ret = text_editor_close_tab(editor);
            break;

        case 5:
            return 1;
    }

    if (ret == 0)
    {
        text_editor_render(editor);

        FileView *current_view = text_editor_get_current_view(editor);
        if (current_view != NULL)
        {
            top_panel(current_view->panel);
            file_view_render(current_view);
        }
    }
    else
    {
        fprintf(stderr, "Text editor: unexpected error\n");
    }

    return ret;
}
