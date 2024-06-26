#include "dialogs.h"

#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <ncurses.h>
#include <menu.h>
#include <form.h>
#include <panel.h>
#include "text_editor.h"
#include "colors.h"

#define KEY_ESC 27
#define KEY_RETURN '\n'

// ----------------------- Private declarations ---------------------

/**
 * @brief Trim spaces from ncurses field.
 * 
 * This is useful because ncurses fill fields blanks with spaces.
 * (adapted from https://gist.github.com/alan-mushi/c8a6f34d1df18574f643)
 * 
 * @param str buffer string
 * @return char* pointer to trimmed string (in the same memory space as input str)
 */
static char* trim_whitespaces(char *str);

/**
 * @brief Handle input for input dialog form
 * 
 * @param win_form pointer to form window
 * @param form pointer to form data
 * @param input_field pointer to the input field
 * @param ch input character
 * @return int 1 if processing done, 0 otherwise
 */
static int input_dialog_handler(WINDOW *win_form, FORM *form, FIELD *input_field, int ch);


// ----------------------- Public definitions -----------------------

int input_dialog(TextEditor *editor, PANEL *dialog_panel, const char *prompt, const char *initial_input, char *buffer, int buffer_len)
{
    top_panel(dialog_panel);
    update_panels();
    doupdate();

    int redraw = 1;
    buffer[0] = '\0';
    while (redraw)
    {
        WINDOW *dialog_win = panel_window(dialog_panel);
        render_dialog_window(dialog_win, prompt);
        FIELD *fields[2];
        
        fields[0] = new_field(1, getmaxx(dialog_win) - 4, 0, 0, 0, 0);
        fields[1] = NULL;

        if (initial_input != NULL && buffer[0] == '\0')
        {
            strncpy(buffer, initial_input, buffer_len);
            buffer[buffer_len - 1] = '\0';
        }
        
        set_field_opts(fields[0], O_VISIBLE | O_PUBLIC | O_EDIT | O_ACTIVE);
        set_field_buffer(fields[0], 0, buffer);
        set_field_back(fields[0], COLOR_PAIR(INTERFACE_COLOR) | A_UNDERLINE);
        
        FORM *form = new_form(fields);
        set_form_win(form, dialog_win);
        set_form_sub(form, derwin(dialog_win, 1, getmaxx(dialog_win) - 4, 2, 2));
        post_form(form);
        
        wrefresh(dialog_win);

        int c;
        for(c = getch(); c != KEY_RETURN && c != KEY_ESC && c != KEY_RESIZE; c = getch())
        {
            input_dialog_handler(dialog_win, form, fields[0], c);
        }

        form_driver(form, REQ_NEXT_FIELD);
        form_driver(form, REQ_PREV_FIELD);

        strncpy(buffer, trim_whitespaces(field_buffer(fields[0], 0)), buffer_len);
        buffer[buffer_len - 1] = '\0';

        WINDOW *old_form_win = form_sub(form);
        unpost_form(form);
        free_field(fields[0]);
        free_form(form);
        delwin(old_form_win);

        if (c == KEY_RESIZE)
        {
            // Handle text editor resize and redraw
            text_editor_handle_resize(editor);
            continue;
        }

        hide_panel(dialog_panel);
        update_panels();
        doupdate();

        // Cancel input on escape
        if (c == KEY_ESC)
        {
            return 0;
        }

        return buffer[0] != '\0';
    }

    return 0;
}

int confirm_dialog(TextEditor *editor, PANEL *dialog_panel, const char *prompt)
{
    top_panel(dialog_panel);
    update_panels();
    doupdate();

    int redraw = 1;
    int selected = 0;
    while (redraw)
    {
        WINDOW *dialog_win = panel_window(dialog_panel);
        render_dialog_window(dialog_win, prompt);

        ITEM *items[3];
        items[0] = new_item("Cancel", "");
        items[1] = new_item("Yes", "");
        items[2] = NULL;

        MENU *menu = new_menu(items);
        set_menu_win(menu, dialog_win);
        set_menu_sub(menu, derwin(dialog_win, 1, 15, 2, (getmaxx(dialog_win) - 16) / 2));

        menu_opts_off(menu, O_SHOWDESC);
        set_menu_format(menu, 1, 2);
        set_menu_mark(menu, ">");
        set_menu_back(menu, COLOR_PAIR(INTERFACE_COLOR));
        set_menu_fore(menu, INTERFACE_SELECTED);
        set_current_item(menu, items[selected]);

        post_menu(menu);
        wrefresh(dialog_win);

        int c;
        for (c = getch(); c != KEY_RETURN && c != KEY_ESC && c != KEY_RESIZE; c = getch())
        {
            switch (c)
            {
                case KEY_LEFT:
                    menu_driver(menu, REQ_LEFT_ITEM);
                    wrefresh(dialog_win);
                    break;

                case KEY_RIGHT:
                    menu_driver(menu, REQ_RIGHT_ITEM);
                    wrefresh(dialog_win);
                    break;
            }
        }

        selected = item_index(current_item(menu));

        WINDOW *old_form_win = menu_sub(menu);
        unpost_menu(menu);
        free_item(items[0]);
        free_item(items[1]);
        free_menu(menu);
        delwin(old_form_win);

        if (c == KEY_RESIZE)
        {
            // Handle text editor resize and redraw
            text_editor_handle_resize(editor);
            continue;
        }

        hide_panel(dialog_panel);
        update_panels();
        doupdate();

        // Cancel input on escape
        if (c == KEY_ESC)
        {
            return 0;
        }

        return selected;
    }

    return 0;
}

void alert_dialog(TextEditor *editor, PANEL *dialog_panel, const char *title, const char *message)
{
    top_panel(dialog_panel);
    update_panels();
    doupdate();

    int redraw = 1;
    while (redraw)
    {
        WINDOW *dialog_win = panel_window(dialog_panel);
        render_dialog_window(dialog_win, title);

        if (message != NULL)
        {
            mvwprintw(dialog_win, 1, 2, message);
        }

        int pos_x = (getmaxx(dialog_win) - 4) / 2;
        mvwaddch(dialog_win, 2, pos_x, '>');
        wattron(dialog_win, INTERFACE_SELECTED);
        mvwprintw(dialog_win, 2, pos_x + 1, "Ok");
        wattroff(dialog_win, INTERFACE_SELECTED);
        wmove(dialog_win, 2, pos_x);
        wrefresh(dialog_win);

        int c;
        for (c = getch(); c != KEY_RETURN && c != KEY_ESC && c != KEY_RESIZE; c = getch());

        if (c == KEY_RESIZE)
        {
            text_editor_handle_resize(editor);
            continue;
        }

        hide_panel(dialog_panel);
        update_panels();
        doupdate();
        return;
    }
}

void render_dialog_window(WINDOW *win, const char *title)
{
    wclear(win);
    wbkgd(win, COLOR_PAIR(INTERFACE_COLOR));
    box(win, 0, 0);
    mvwprintw(win, 0, 2, " %s ", title);
}


// ----------------------- Private definitions -----------------------

static char* trim_whitespaces(char *str)
{
    char *end;

    // trim leading space
    while (isspace(*str))
    {
        str++;
    }

    // all spaces
    if (*str == 0)
    {
        return str;
    }

    // trim trailing space
    end = str + strnlen(str, 128) - 1;

    while (end > str && isspace(*end))
    {
        end--;
    }

    // write new null terminator
    *(end + 1) = '\0';

    return str;
}

static int input_dialog_handler(WINDOW *win_form, FORM *form, FIELD *input_field, int ch)
{
    switch (ch) {
        // case KEY_F(2):
        case KEY_ENTER:
        case 10:
            form_driver(form, REQ_NEXT_FIELD);
			form_driver(form, REQ_PREV_FIELD);
            return 1;

        case KEY_DOWN:
            form_driver(form, REQ_NEXT_FIELD);
            form_driver(form, REQ_END_LINE);
            break;

        case KEY_UP:
            form_driver(form, REQ_PREV_FIELD);
            form_driver(form, REQ_END_LINE);
            break;

        case KEY_LEFT:
            form_driver(form, REQ_PREV_CHAR);
            break;

        case KEY_RIGHT:
            form_driver(form, REQ_NEXT_CHAR);
            break;

        // Delete the char before cursor
        case KEY_BACKSPACE:
        case 127:
            form_driver(form, REQ_DEL_PREV);
            if (form_driver(form, REQ_PREV_CHAR) == 0)
            {
                form_driver(form, REQ_NEXT_CHAR);
            }
            break;

        // Delete the char under the cursor
        case KEY_DC:
            form_driver(form, REQ_DEL_CHAR);
            break;

        case KEY_RESIZE:
            wresize(win_form, 5, COLS / 2);
            
            break;

        default:
            form_driver(form, ch);
            break;
    }

    wrefresh(win_form);
    return 0;
}
