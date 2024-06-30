#include "text_editor.h"

#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <errno.h>
#include "dialogs.h"
#include "colors.h"

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
    "Close file        ",
    "Quit              "
};

enum FileMenuOptions {
    FILE_MENU_NEW_FILE_OPTION,
    FILE_MENU_OPEN_FILE_OPTION,
    FILE_MENU_SAVE_FILE_OPTION,
    FILE_MENU_SAVE_FILE_AS_OPTION,
    FILE_MENU_CLOSE_TAB_OPTION,
    FILE_MENU_QUIT_OPTION
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

enum ClickPosition
{
    CLICK_MENU = -1,
    CLICK_PREV = -2,
    CLICK_NEXT = -3,
    CLICK_OUTSIDE = -4 
};
typedef enum ClickPosition ClickPosition;

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
 * @brief Update file menu options.
 * 
 * Check if options should be enabled or disabled for current editor context.
 * 
 * @param editor pointer to initialized TextEditor instance
 */
void text_editor_update_menu_options(TextEditor *editor);

/**
 * @brief Execute file menu action.
 * 
 * @param editor pointer to initialized TextEditor structure
 * @return int 1 if the program should close, 0 otherwise
 */
int text_editor_menu_action(TextEditor *editor);

/**
 * @brief Execute top bar click action.
 * 
 * This either changes active tab or handles special ClickPosition actions.
 * 
 * @param editor pointer to initialized TextEditor structure
 * @param action tab index or ClickPosition action
 * @return int 
 */
void text_editor_click_action(TextEditor *editor, int action);

/**
 * @brief Get clicked action on top bar.
 * 
 * @param editor pointer to initialized TextEditor structure
 * @param y pointer coordonate y (relative to top bar window)
 * @param x pointer coordonate x (relative to top bar window)
 * @return ClickPosition clicked tab or special element clicked
 */
ClickPosition text_editor_top_bar_click(TextEditor *editor, int y, int x);


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
            return E_INTERNAL_ERROR;
        }
    }
    else
    {
        FileView **new_tabs = realloc(editor->tabs, (editor->n_tabs + 1) * sizeof(FileView*));
        if (new_tabs == NULL)
        {
            return E_INTERNAL_ERROR;
        }

        editor->tabs = new_tabs;
    }

    editor->tabs[editor->n_tabs] = create_file_view(FILE_VIEW_HEIGHT, FILE_VIEW_WIDTH, FILE_VIEW_OFFSET_Y, FILE_VIEW_OFFSET_X);

    if (editor->tabs[editor->n_tabs] == NULL)
    {
        return E_INTERNAL_ERROR;
    }

    editor->current_tab = editor->n_tabs;
    editor->n_tabs++;

    return 0;
}

int text_editor_close_tab(TextEditor *editor)
{
    if (editor->tabs == NULL)
    {
        return E_SUCCESS;
    }

    FileView *current_view = text_editor_get_current_view(editor);
    FileViewStatus status = file_view_get_status(current_view);
    if (status == FILE_VIEW_STATUS_NEW_FILE || status == FILE_VIEW_STATUS_MODIFIED)
    {
        if (confirm_dialog(editor->dialog_panel, "Close file without saving changes?") == 0)
        {
            return E_SUCCESS;
        }
    }

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
        return E_SUCCESS;
    }

    FileView **new_tabs = realloc(editor->tabs, editor->n_tabs * sizeof(FileView*));

    if (new_tabs == NULL)
    {
        return E_INTERNAL_ERROR;
    }

    editor->tabs = new_tabs;
    editor->current_tab--;
    if (editor->current_tab < 0)
    {
        editor->current_tab = 0;
    }
    return E_SUCCESS;
}

int text_editor_load_file(TextEditor *editor)
{
    char buffer[PATH_INPUT_BUFFER_LEN];
    if (input_dialog(editor->dialog_panel, "Enter file path", NULL, buffer, PATH_INPUT_BUFFER_LEN) == 0)
    {
        // User canceled file path input
        return E_SUCCESS;
    }

    if (text_editor_new_tab(editor) < 0)
    {
        return E_INTERNAL_ERROR;
    }

    
    int ret = file_view_load_file(editor->tabs[editor->current_tab], buffer);
    if (ret < 0)
    {
        int load_errno = errno;

        if (text_editor_close_tab(editor) < 0)
        {
            return E_INTERNAL_ERROR;
        }
        
        if (ret == E_IO_ERROR)
        {
            alert_dialog(editor->dialog_panel, "Error while loading file", strerror(load_errno));
        }
        else
        {
            return E_INTERNAL_ERROR;
        }
    }

    return E_SUCCESS;
}

int text_editor_save_file(TextEditor *editor, int save_as)
{
    FileView *current_view = text_editor_get_current_view(editor);

    if (current_view == NULL)
    {
        return E_SUCCESS;
    }
    
    if (!save_as && file_view_get_file_path(current_view) == NULL)
    {
        return text_editor_save_file(editor, 1);
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
            return E_SUCCESS;
        }

        save_path = buffer;
    }
    
    int ret;
    if ((ret = file_view_save_file(current_view, save_path)) < 0)
    {
        if (ret == E_IO_ERROR)
        {
            int save_errno = errno;
            alert_dialog(editor->dialog_panel, "Error while saving file", strerror(save_errno));
        }
        else
        {
            return E_INTERNAL_ERROR;
        }
    }

    return E_SUCCESS;
}

void text_editor_set_current_tab(TextEditor *editor, int index)
{
    if (editor->n_tabs == 0)
    {
        return;
    }

    if (index >= editor->n_tabs)
    {
        editor->current_tab = 0;
    }
    else if (index < 0)
    {
        editor->current_tab = editor->n_tabs - 1;
    }
    else
    {
        editor->current_tab = index;
    }
    
    text_editor_render(editor);
    FileView *current_view = text_editor_get_current_view(editor);
    top_panel(current_view->panel);
    file_view_render(current_view);
}

void text_editor_render(TextEditor *editor)
{
    WINDOW *win = editor->top_bar_win;
    int width = getmaxx(win);
    int menu_len = 11;
    int indicator_len = 2;
    int tabs_len = width - menu_len - 2 * indicator_len;
    int prev_offset = menu_len;
    int tabs_offset = menu_len + indicator_len;
    int next_offset = menu_len + indicator_len + tabs_len;

    werase(win);
    wbkgd(win, COLOR_PAIR(INTERFACE_COLOR));

    // File menu
    wattron(win, COLOR_PAIR(MENU_COLOR));
    mvwaddstr(win, 0, 0, " File (F1) ");
    wattroff(win, COLOR_PAIR(MENU_COLOR));
    
    // Compute start position
    int tabs_pos = 0;
    for (int i = 0; i <= editor->current_tab; i++)
    {
        const char *title = file_view_get_title(editor->tabs[i]);
        tabs_pos += strlen(title) + (i == 0 ? 2 : 3);
    }

    int start_i = 0;
    while (tabs_pos > tabs_len && start_i < editor->current_tab)
    {
        const char *title = file_view_get_title(editor->tabs[start_i]);
        tabs_pos -= strlen(title) + 3;
        start_i++;
    }

    // Prev indicator
    wattron(win, start_i == 0 ? INTERFACE_DISABLED : A_BOLD);
    mvwaddstr(win, 0, prev_offset, " <");
    wattroff(win, start_i == 0 ? INTERFACE_DISABLED : A_BOLD);

    // Show tabs
    tabs_pos = 0;
    int i;
    wmove(win, 0, tabs_offset);
    for (i = start_i; i < editor->n_tabs; i++)
    {
        const char *title = file_view_get_title(editor->tabs[i]);

        // Not enough space for tab
        if (tabs_pos + strlen(title) + 2 > tabs_len && i != start_i)
        {
            break;
        }

        // Add separator
        if (i != start_i)
        {
            waddch(win, ACS_VLINE);
            tabs_pos++;
        }

        // Highlight current tab
        if (i == editor->current_tab)
        {
            wattron(win, A_STANDOUT);
        }

        // Print tab 
        waddch(win,' ');
        waddnstr(win, title, tabs_len - tabs_pos - 1);

        if (tabs_pos + strlen(title) + 2 <= tabs_len)
        {
            waddch(win, ' ');
        }

        wattroff(win, A_STANDOUT);

        tabs_pos += strlen(title) + 2;
    }

    // Next indicator
    wattron(win, i == editor->n_tabs ? INTERFACE_DISABLED : A_BOLD);
    mvwaddstr(win, 0, next_offset, "> ");
    wattroff(win, i == editor->n_tabs ? INTERFACE_DISABLED : A_BOLD);
}

int text_editor_handle_input(TextEditor *editor, int input)
{
    int ret = 0;
    FileView *current_view = text_editor_get_current_view(editor);

    MEVENT event;
    if (!panel_hidden(editor->menu_panel))
    {
        wrefresh(editor->menu_win);
        switch (input)
        {
            // Change menu options
            case KEY_DOWN:
                menu_driver(editor->menu, REQ_DOWN_ITEM);
                wrefresh(editor->menu_win);
                break;

            case KEY_UP:
                menu_driver(editor->menu, REQ_UP_ITEM);
                wrefresh(editor->menu_win);
                break;

            // Apply action
            case 10: // \n
            case KEY_ENTER:
                ret = text_editor_menu_action(editor);
                break;

            // Exit menu
            case KEY_F(1):
            case 27: // Escape
                hide_panel(editor->menu_panel);
                break;
        }
    }
    else
    {
        switch (input)
        {
            // Mouse input
            case KEY_MOUSE:
                if(getmouse(&event) == OK && wmouse_trafo(editor->top_bar_win, &event.y, &event.x, FALSE) == TRUE)
			    {
                    ClickPosition clicked_item = text_editor_top_bar_click(editor, event.y, event.x);
                    text_editor_click_action(editor, clicked_item);
                }
                break;

            // Open menu
            case KEY_F(1):
                show_panel(editor->menu_panel);
                top_panel(editor->menu_panel);
                text_editor_update_menu_options(editor);
                break;

            // Cycle tabs
            case 9: // Tab
                text_editor_set_current_tab(editor, editor->current_tab + 1);
                break;

            case KEY_BTAB:
                text_editor_set_current_tab(editor, editor->current_tab - 1);
                break;

            // Clipboard shortcuts
            case 3: // Ctrl + C
                ret = text_editor_copy_selection(editor, 0);
                break;

            case 22: // Ctrl + V
                ret = text_editor_paste_selection(editor);
                break;

            case 24: // Ctrl + X
                ret = text_editor_copy_selection(editor, 1);
                break;

            case 25: // Ctrl + Y
                ret = text_editor_delete_selection(editor);
                break;

            // Input for file view
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
    return ret;
}

int text_editor_copy_selection(TextEditor *editor, int cut)
{
    FileView *current_view = text_editor_get_current_view(editor);

    if (current_view == NULL)
    {
        return E_SUCCESS;
    }

    if (editor->clipboard != NULL)
    {
        free(editor->clipboard);
        editor->clipboard_length = 0;
    }

    if (file_view_copy_selection(current_view, &editor->clipboard, &editor->clipboard_length) < 0)
    {
        return E_INTERNAL_ERROR;
    }

    if (cut)
    {
        if (file_view_delete_selection(current_view) < 0)
        {
            return E_INTERNAL_ERROR;
        }

        file_view_render(current_view);
    }

    return E_SUCCESS;
}

int text_editor_paste_selection(TextEditor *editor)
{
    FileView *current_view = text_editor_get_current_view(editor);

    if (current_view == NULL || editor->clipboard == NULL)
    {
        return E_SUCCESS;
    }

    for (int i = 0; i < editor->clipboard_length; i++)
    {
        if (file_view_handle_input(current_view, editor->clipboard[i]) < 0)
        {
            return E_INTERNAL_ERROR;
        }
    }

    file_view_render(current_view);
    return E_SUCCESS;
}

int text_editor_delete_selection(TextEditor *editor)
{
    FileView *current_view = text_editor_get_current_view(editor);

    if (current_view == NULL)
    {
        return E_SUCCESS;
    }
    
    if (file_view_delete_selection(current_view) < 0)
    {
        return E_INTERNAL_ERROR;
    }

    file_view_render(current_view);
    return E_SUCCESS;
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
    set_menu_back(editor->menu, COLOR_PAIR(INTERFACE_COLOR));
    set_menu_fore(editor->menu, INTERFACE_SELECTED);
    set_menu_grey(editor->menu, INTERFACE_DISABLED);
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

void text_editor_update_menu_options(TextEditor *editor)
{
    set_current_item(editor->menu, editor->menu_items[FILE_MENU_NEW_FILE_OPTION]);

    FileView *current_view = text_editor_get_current_view(editor);

    if (current_view == NULL)
    {
        item_opts_off(editor->menu_items[FILE_MENU_SAVE_FILE_OPTION], O_SELECTABLE);
        item_opts_off(editor->menu_items[FILE_MENU_SAVE_FILE_AS_OPTION], O_SELECTABLE);
        item_opts_off(editor->menu_items[FILE_MENU_CLOSE_TAB_OPTION], O_SELECTABLE);
    }
    else
    {
        if (file_view_get_status(current_view) != FILE_VIEW_STATUS_SAVED)
        {
            item_opts_on(editor->menu_items[FILE_MENU_SAVE_FILE_OPTION], O_SELECTABLE);
        }
        else
        {
            item_opts_off(editor->menu_items[FILE_MENU_SAVE_FILE_OPTION], O_SELECTABLE);
        }
        item_opts_on(editor->menu_items[FILE_MENU_SAVE_FILE_AS_OPTION], O_SELECTABLE);
        item_opts_on(editor->menu_items[FILE_MENU_CLOSE_TAB_OPTION], O_SELECTABLE);
    }
}

int text_editor_menu_action(TextEditor *editor)
{
    int ret = 0;

    ITEM *item = current_item(editor->menu);

    // Disabled item
    if (!(item_opts(item) & O_SELECTABLE))
    {
        return E_SUCCESS;
    }

    // Close menu
    hide_panel(editor->menu_panel);

    // Handle selected menu action
    switch (item_index(item))
    {
        case FILE_MENU_NEW_FILE_OPTION:
            ret = text_editor_new_tab(editor);
            break;

        case FILE_MENU_OPEN_FILE_OPTION:
            ret = text_editor_load_file(editor);
            break;

        case FILE_MENU_SAVE_FILE_OPTION:
            text_editor_save_file(editor, 0);
            break;

        case FILE_MENU_SAVE_FILE_AS_OPTION:
            text_editor_save_file(editor, 1);
            break;

        case FILE_MENU_CLOSE_TAB_OPTION:
            ret = text_editor_close_tab(editor);
            break;

        case 5:
            return 1;
    }

    // Rerender views
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

    return ret;
}

void text_editor_click_action(TextEditor *editor, int action)
{
    switch(action)
    {
        case CLICK_MENU:
            ungetch(KEY_F(1));
            break;

        case CLICK_NEXT:
            text_editor_set_current_tab(editor, editor->current_tab + 1);
            break;
        
        case CLICK_PREV:
            text_editor_set_current_tab(editor, editor->current_tab - 1);
            break;

        case CLICK_OUTSIDE:
            break;
    
        default:
            text_editor_set_current_tab(editor, action);
            break;
    }
}

ClickPosition text_editor_top_bar_click(TextEditor *editor, int y, int x)
{
    WINDOW *win = editor->top_bar_win;
    int width = getmaxx(win);
    int menu_len = 11;
    int indicator_len = 2;
    int tabs_len = width - menu_len - 2 * indicator_len;
    int prev_offset = menu_len;
    int tabs_offset = menu_len + indicator_len;
    int next_offset = menu_len + indicator_len + tabs_len;

    // Click in menu region
    if (x <= menu_len)
    {
        return CLICK_MENU;
    }

    // Click in prev region
    if (x >= prev_offset && x < prev_offset + indicator_len)
    {
        return CLICK_PREV;
    }

    // Click in next region
    if (x >= next_offset && x < next_offset + indicator_len)
    {
        return CLICK_NEXT;
    }

    int tabs_pos = 0;
    for (int i = 0; i <= editor->current_tab; i++)
    {
        const char *title = file_view_get_title(editor->tabs[i]);
        tabs_pos += strlen(title) + (i == 0 ? 2 : 3);
    }

    int start_i = 0;
    while (tabs_pos > tabs_len && start_i < editor->current_tab)
    {
        const char *title = file_view_get_title(editor->tabs[start_i]);
        tabs_pos -= strlen(title) + 3;
        start_i++;
    }

    // Click in tabs region
    if (x >= tabs_offset && x < tabs_offset + tabs_len)
    {
        tabs_pos = 0;
        for (int i = start_i; i < editor->n_tabs && x >= tabs_offset + tabs_pos; i++)
        {
            const char *title = file_view_get_title(editor->tabs[i]);

            // Add separator
            if (i != start_i)
            {
                tabs_pos++;
            }

            // Click inside tab
            if (x < tabs_offset + tabs_pos + strlen(title) + 2)
            {
                return i;
            }
            tabs_pos += strlen(title) + 2;
        }
    }

    return CLICK_OUTSIDE;
}
