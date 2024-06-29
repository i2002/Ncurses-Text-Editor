#include "file_view.h"

#include <stdlib.h>
#include <string.h>
#include <libgen.h>


// -------------------------- Configuration -------------------------

const char default_title[] = "Untitled";


// ------------------------------ Macros ----------------------------

/**
 * @brief Check if abort condition is satisfied and abort creation (free already created resources and return error status)
 * 
 * @param expr abort condition
 * @param view pointer to FileView instance to be destroyed on abort
 */
#define ABORT_CREATE(expr, view) if (expr) { free_file_view(view); return NULL; }


// ----------------------- Private declarations ---------------------

/**
 * @brief Change cursor current position after input.
 * 
 * @param view pointer to initialized FileView structure
 * @param input input character / key
 */
void update_cursor_position(FileView *view, int input);

/**
 * @brief Set file view path.
 * 
 * @param view pointer to initialized FileView structure
 * @param file_path file path
 * @return int 0 for succes, 1 for failure
 */
int file_view_set_file_path(FileView *view, const char* file_path);


// ----------------------- Public definitions -----------------------

FileView* create_file_view(int height, int width, int offset_y, int offset_x)
{
    FileView *view = (FileView*) malloc(sizeof(FileView));

    if (view == NULL)
    {
        return NULL;
    }

    memset(view, 0, sizeof(FileView));

    view->data = (FileData*) malloc(sizeof(FileData));
    ABORT_CREATE(view->data == NULL, view);

    ABORT_CREATE(create_file_data(width - 1, view->data) != 0, view);

    view->title = (char*) malloc((strlen(default_title) + 1) * sizeof(char));
    ABORT_CREATE(view->title == NULL, view);
    strcpy(view->title, default_title);

    view->win = newwin(height, width, offset_y, offset_x);
    ABORT_CREATE(view->win == NULL, view);

    view->panel = new_panel(view->win);
    ABORT_CREATE(view->panel == NULL, view);
    
    return view;
}

void free_file_view(FileView *view)
{
    if (view == NULL)
    {
        return;
    }

    if (view->panel != NULL)
    {
        del_panel(view->panel);
    }

    if (view->win != NULL)
    {
        delwin(view->win);
    }

    if (view->title != NULL)
    {
        free(view->title);
    }

    if (view->file_path != NULL)
    {
        free(view->file_path);
    }

    if (view->data != NULL)
    {
        free_file_data(view->data);
        free(view->data);
    }

    free(view);
}

int file_view_new_file(FileView *view)
{
    // Insert the first line into the file
    return file_data_insert_char(view->data, -1, 0, '\n');
}

int file_view_load_file(FileView *view, const char* file_path)
{
    // Get window dimensions
    int width = getmaxx(view->win);

    // Recreate file data structure
    create_file_data(width - 1, view->data);

    // Try to load file into data structure
    int res = load_file_data(view->data, file_path);

    if (res != 0)
    {
        return res;
    }

    // Update tab name and file path
    if (file_view_set_file_path(view, file_path) != 0)
    {
        return 1;
    }

    // Reset positions
    view->pos_x = 0;
    view->pos_y = 0;
    view->scroll_offset = 0;

    return 0;
}

int file_view_save_file(FileView *view, const char* file_path)
{
    if (file_path != NULL)
    {
        if (file_view_set_file_path(view, file_path) != 0)
        {
            return 1;
        }
    }

    if (view->file_path == NULL)
    {
        return 1;
    }

    int ret = save_file_data(view->data, view->file_path);

    if (ret != 0)
    {
        return ret;
    }

    // FIXME: set state to saved
    return 0;
}

void file_view_render(FileView *view)
{
    int height, width;
    getmaxyx(view->win, height, width);

    for (int i = 0; i < height; i++)
    {
        if (i + view->scroll_offset < view->data->size)
        {
            const FileLine *line = get_file_data_line(view->data, i + view->scroll_offset);
            mvwaddnstr(view->win, i, 0, line->content, line->size);

            if (!line->endl)
            {
                wattron(view->win, COLOR_PAIR(3));
                mvwaddch(view->win, i, width - 1, '~');
                wattroff(view->win, COLOR_PAIR(3));
            }
        }
        else
        {
            wattron(view->win, COLOR_PAIR(3));
            mvwaddch(view->win, i, 0, '~');
            wattroff(view->win, COLOR_PAIR(3));
        }
        wclrtoeol(view->win);
    }

    const FileLine *current_line = get_file_data_line(view->data, view->scroll_offset + view->pos_y);
    if (current_line != NULL)
    {
        mvwprintw(view->win, height - 1, 0, "(d x: %d, d y: %d, i: %d, s line: %d, s col: %d, size: %d, endl: %d)", view->pos_x, view->pos_y, view->pos_y + view->scroll_offset, current_line->line, current_line->col_start + view->pos_x, current_line->size, current_line->endl);
    }

    // Update cursor position
    wmove(view->win, view->pos_y, view->pos_x);
    wrefresh(view->win);
}

void file_view_handle_input(FileView *view, int input)
{
    int res = 1;
    int cursor_move = 0;
    int temp_pos_x = view->pos_x, temp_pos_y = view->pos_y;

    if (input == KEY_BACKSPACE)
    {
        const FileLine *line = get_file_data_line(view->data, view->scroll_offset + view->pos_y);
        if (line == NULL)
        {
            return;
        }

        int source_line = line->line;
        int source_col = line->col_start + view->pos_x - 1;

        if (source_col == -1)
        {
            source_line--;
        }
        
        file_data_get_display_coords(view->data, source_line, source_col, &temp_pos_y, &temp_pos_x);
        temp_pos_y -= view->scroll_offset;
    }

    switch (input)
    {
        case KEY_UP:
        case KEY_DOWN:
        case KEY_LEFT:
        case KEY_RIGHT:
        case KEY_HOME:
        case KEY_END:
            cursor_move = input;
            res = 0;
            break;
        case KEY_ENTER:
        case '\n':
            res = file_data_insert_char(view->data, view->pos_y + view->scroll_offset, view->pos_x, '\n');
            cursor_move = KEY_ENTER;
            break;

        case KEY_BACKSPACE:
            res = file_data_delete_char(view->data, view->pos_y + view->scroll_offset, view->pos_x - 1);
            cursor_move = KEY_BACKSPACE;
            break;

        default:
            res = file_data_insert_char(view->data, view->pos_y + view->scroll_offset, view->pos_x, (char) input);
            cursor_move = KEY_RIGHT;
            break;
    }

    if (res == 0)
    {
        if (cursor_move == KEY_BACKSPACE)
        {
            view->pos_x = temp_pos_x;
            view->pos_y = temp_pos_y;
        }

        update_cursor_position(view, cursor_move);
    }

    // FIXME: temporary for debug
    file_data_check_integrity(view->data);
}

const char *file_view_get_title(FileView *view)
{
    if (view == NULL)
    {
        return NULL;
    }

    return view->title;
}

const char *file_view_get_file_path(FileView *view)
{
    if (view == NULL)
    {
        return NULL;
    }

    return view->file_path;
}


// ----------------------- Private definitions -----------------------

void update_cursor_position(FileView *view, int input)
{
    // Get window dimensions
    int height = getmaxy(view->win);

    const FileLine *current_line = get_file_data_line(view->data, view->scroll_offset + view->pos_y);

    if (current_line == 0)
    {
        view->pos_x = 0;
        view->pos_y = 0;
        return;
    }

    // Position in source file context
    int source_line = current_line->line;
    int source_col = current_line->col_start + view->pos_x;

    switch(input)
    {
        case KEY_UP:
            source_line--;
            break;

        case KEY_DOWN:
            source_line++;
            break;

        case KEY_LEFT:
            if (source_col > 0)
            {
                source_col--;
            }
            break;

        case KEY_RIGHT:
            source_col++;
            break;

        case KEY_ENTER:
            source_line++;
            source_col = 0;
            break;

        case KEY_HOME:
            source_col = 0;
            break;

        case KEY_END:
            source_col = -1;
            break;
    }

    // Get coresponding position in display file context
    int temp_x, temp_y;
    if (file_data_get_display_coords(view->data, source_line, source_col, &temp_y, &temp_x) == 0)
    {
        view->pos_y = temp_y - view->scroll_offset;
        view->pos_x = temp_x;
    }

    // Adjust scroll offset
    if (view->pos_y <= 0 && view->scroll_offset > 0)
    {
        view->scroll_offset += view->pos_y - 1;
        view->pos_y = 1;
    }

    if (view->pos_y >= height - 2) // FIXME: status bar output
    {
        view->scroll_offset += view->pos_y - height + 3;
        view->pos_y = height - 3;
    }
}

int file_view_set_file_path(FileView *view, const char* file_path)
{
    // Set view file path
    if (view->file_path != NULL)
    {
        free(view->file_path);
    }

    view->file_path = (char*) malloc((strlen(file_path) + 1) * sizeof(char));
    if (view->file_path == NULL)
    {
        return 1;
    }
    strcpy(view->file_path, file_path);

    // Set view title
    char *file_path_copy = (char*) malloc((strlen(file_path) + 1) * sizeof(char));
    if (file_path_copy == NULL)
    {
        return 1;
    }
    strcpy(file_path_copy, file_path);

    const char *title_temp = basename(file_path_copy);

    view->title = (char*) malloc((strlen(title_temp) + 1) * sizeof(char));
    if (view->title == NULL)
    {
        free(file_path_copy);
        return 1;
    }
    strcpy(view->title, title_temp);
    free(file_path_copy);

    return 0;
}
