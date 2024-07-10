#ifndef TEXT_EDITOR_H
#define TEXT_EDITOR_H

#include "file_view.h"
#include <ncurses.h>
#include <menu.h>

#define MENU_ITEMS_SIZE 6

#define E_SUCCESS         0
#define E_INTERNAL_ERROR -1

#define KEY_ALT_LEFT 1001
#define KEY_ALT_RIGHT 1002
#define KEY_ALT_SHIFT_LEFT 1003
#define KEY_ALT_SHIFT_RIGHT 1004
#define KEY_CTRL_LEFT 1005
#define KEY_CTRL_RIGHT 1006
#define KEY_CTRL_SHIFT_LEFT 1007
#define KEY_CTRL_SHIFT_RIGHT 1008

struct TextEditor
{
    FileView **tabs;
    int n_tabs;
    int current_tab;

    WINDOW *top_bar_win;
    PANEL *top_bar_panel;

    WINDOW *menu_win;
    PANEL *menu_panel;
    ITEM *menu_items[MENU_ITEMS_SIZE + 1];
    MENU *menu;

    WINDOW *dialog_win;
    PANEL *dialog_panel;

    WINDOW *about_win;
    PANEL *about_panel;

    char *clipboard;
    int clipboard_length;
};

typedef struct TextEditor TextEditor;

/**
 * @brief Create a text editor instance.
 * 
 * @return TextEditor* 
 */
TextEditor* create_text_editor();

/**
 * @brief Free text editor instance.
 * 
 * @param editor pointer to TextEditor instance
 */
void free_text_editor(TextEditor *editor);

/**
 * @brief Create new tab in text editor
 * 
 * @param editor pointer to TextEditor instance
 * @return int 0 for success, < 0 for failure
 */
int text_editor_new_tab(TextEditor *editor);

/**
 * @brief Close current text editor tab.
 * 
 * @param editor pointer to TextEditor instance
 * @return int 0 for success, < 0 for failure
 */
int text_editor_close_tab(TextEditor *editor);

/**
 * @brief Load file into text editor.
 * 
 * @param editor pointer to TextEditor instance
 * @return int 0 for success, < 0 for failure
 */
int text_editor_load_file(TextEditor *editor);

/**
 * @brief Save current text editor tab.
 * 
 * @param editor pointer to TextEditor instance
 * @param save_as whether to ask for a new file location
 * @return int 0 for succes, < 0 for failure
 */
int text_editor_save_file(TextEditor *editor, int save_as);

/**
 * @brief Set editor current tab.
 * 
 * @param editor pointer to TextEditor instance
 * @param index new tab index
 */
void text_editor_set_current_tab(TextEditor *editor, int index);

/**
 * @brief Render text editor tabs.
 * 
 * @param editor pointer to TextEditor instance
 */
void text_editor_render(TextEditor *editor);

/**
 * @brief Handle screen resize.
 * 
 * @param editor pointer to TextEditor instance
 */
int text_editor_handle_resize(TextEditor *editor);

/**
 * @brief Handle text editor input.
 * 
 * @param editor pointer to TextEditor instance
 * @param input keyboard input
 * @return int 0 for success, < 0 for failure
 */
int text_editor_handle_input(TextEditor *editor, int input);

/**
 * @brief Handle copy view selection to clipboard.
 * 
 * @param editor pointer to TextEditor instance
 * @param cut whether the text should be also deleted from source
 * @return int 0 for success, < 0 for failure
 */
int text_editor_copy_selection(TextEditor *editor, int cut);

/**
 * @brief Handle paste clipboard into view.
 * 
 * @param editor pointer to TextEditor instance
 * @return int 0 for success, < 0 for failure
 */
int text_editor_paste_selection(TextEditor *editor);

/**
 * @brief Handle delete view selection.
 * 
 * @param editor pointer to TextEditor instance
 * @return int 0 for success, < 0 for failure
 */
int text_editor_delete_selection(TextEditor *editor);

/**
 * @brief Get current text editor tab.
 * 
 * @param editor pointer to TextEditor instance
 * @return FileView* current tab FileView or NULL if no tab open
 */
FileView *text_editor_get_current_view(TextEditor *editor);

#endif // TEXT_EDITOR_H
