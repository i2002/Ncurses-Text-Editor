#include "file_view.h"

#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include "colors.h"


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

/**
 * @brief Get file view selection range.
 * 
 * @param view pointer to initialized FileView structure
 * @param sel_start_line start source line
 * @param sel_start_col start source col
 * @param sel_stop_line stop source line
 * @param sel_stop_col stop source col
 */
void file_view_get_selection_ranges(FileView *view, int *sel_start_line, int *sel_start_col, int *sel_stop_line, int *sel_stop_col);


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
    
    view->status = FILE_VIEW_STATUS_NEW_FILE;
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

int file_view_load_file(FileView *view, const char* file_path)
{
    view->status = FILE_VIEW_STATUS_UNINITIALIZED;

    // Try to load file into data structure
    int res = load_file_data(view->data, file_path);

    if (res < 0)
    {
        return res;
    }

    // Update tab name and file path
    if (file_view_set_file_path(view, file_path) != 0)
    {
        return E_INTERNAL_ERROR;
    }

    // Reset positions
    view->pos_x = 0;
    view->pos_y = 0;
    view->scroll_offset = 0;

    // Update status
    view->status = FILE_VIEW_STATUS_SAVED;

    return E_SUCCESS;
}

int file_view_save_file(FileView *view, const char* file_path)
{
    const char *save_file_path = file_path != NULL ? file_path : view->file_path;

    if (save_file_path == NULL)
    {
        return E_INTERNAL_ERROR;
    }

    int ret = save_file_data(view->data, save_file_path);

    if (ret < 0)
    {
        return ret;
    }

    // Update file path and tab title if file path changed
    if (file_path != NULL)
    {
        if (file_view_set_file_path(view, file_path) != 0)
        {
            return E_INTERNAL_ERROR;
        }
    }

    view->status = FILE_VIEW_STATUS_SAVED;
    return E_SUCCESS;
}

void file_view_render(FileView *view)
{
    int height, width;
    getmaxyx(view->win, height, width);

    int start_sel = 0;
    int sel_start_line, sel_start_col, sel_stop_line, sel_stop_col;
    file_view_get_selection_ranges(view, &sel_start_line, &sel_start_col, &sel_stop_line, &sel_stop_col);

    for (int i = 0; i < height; i++)
    {
        if (i + view->scroll_offset < view->data->size)
        {
            const FileLine *line = get_file_data_line(view->data, i + view->scroll_offset);
            int source_line = line->line;
            int source_col = line->col_start;

            wmove(view->win, i, 0);
            for (int col = 0; col < line->size; col++)
            {
                if (source_line == sel_start_line && source_col + col == sel_start_col)
                {
                    start_sel = 1;
                }

                if (source_line == sel_stop_line && source_col + col == sel_stop_col)
                {
                    start_sel = 0;
                }

                int mod = start_sel ? A_STANDOUT : 0;
                waddch(view->win, line->content[col] | mod);
            }

            if (source_line == sel_start_line && source_col + line->size == sel_start_col)
            {
                start_sel = 1;
            }

            if (source_line == sel_stop_line && source_col + line->size == sel_stop_col)
            {
                start_sel = 0;
            }

            if (start_sel && line->size == 0)
            {
                waddch(view->win, ' ' | A_STANDOUT);
            }

            if (!line->endl)
            {
                wattron(view->win, COLOR_PAIR(MARKER_COLOR));
                mvwaddch(view->win, i, width - 1, '>');
                wattroff(view->win, COLOR_PAIR(MARKER_COLOR));
            }
        }
        else
        {
            wattron(view->win, COLOR_PAIR(MARKER_COLOR));
            mvwaddch(view->win, i, 0, '~');
            wattroff(view->win, COLOR_PAIR(MARKER_COLOR));
        }
        wclrtoeol(view->win);
    }

    const FileLine *current_line = get_file_data_line(view->data, view->scroll_offset + view->pos_y);
    if (current_line != NULL)
    {
        mvwprintw(view->win, height - 1, 0, "(d x: %d, d y: %d, i: %d, s line: %d, s col: %d, size: %d, endl: %d, status: %d, sel_start: %d, %d; sel_stop: %d, %d)", view->pos_x, view->pos_y, view->pos_y + view->scroll_offset, current_line->line, current_line->col_start + view->pos_x, current_line->size, current_line->endl, view->status, sel_start_line, sel_start_col, sel_stop_line, sel_stop_col);
    }

    // Update cursor position
    wmove(view->win, view->pos_y, view->pos_x);
    wrefresh(view->win);
}

int file_view_handle_input(FileView *view, int input)
{
    int res = 1;
    int cursor_move = 0;
    int modified = 0;
    int temp_pos_x = view->pos_x, temp_pos_y = view->pos_y;

    if (input == KEY_BACKSPACE)
    {
        if (view->sel_active)
        {
            view->sel_active = 0;
            return file_view_delete_selection(view);
        }

        const FileLine *line = get_file_data_line(view->data, view->scroll_offset + view->pos_y);
        if (line == NULL)
        {
            return E_INTERNAL_ERROR;
        }

        int source_line = line->line;
        int source_col = line->col_start + view->pos_x - 1;

        if (source_col == -1)
        {
            source_line--;
        }
        
        if (file_data_get_display_coords(view->data, source_line, source_col, &temp_pos_y, &temp_pos_x) < 0)
        {
            return E_INTERNAL_ERROR;
        }
        temp_pos_y -= view->scroll_offset;
    }

    view->sel_active = 0;
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

        case KEY_SR:
        case KEY_SF:
        case KEY_SRIGHT:
        case KEY_SLEFT:
        case KEY_SHOME:
        case KEY_SEND:
            view->sel_active = 1;
            cursor_move = input;
            res = 0;
            break;

        case KEY_ENTER:
        case '\n':
            res = file_data_insert_char(view->data, view->pos_y + view->scroll_offset, view->pos_x, '\n');
            cursor_move = KEY_ENTER;
            modified = 1;
            break;

        case KEY_BACKSPACE:
            res = file_data_delete_char(view->data, view->pos_y + view->scroll_offset, view->pos_x - 1);
            cursor_move = KEY_BACKSPACE;
            modified = 1;
            break;

        case KEY_RESIZE:
            res = resize_file_data_col(view->data, getmaxx(view->win) - 1);
            break;

        default:
            res = file_data_insert_char(view->data, view->pos_y + view->scroll_offset, view->pos_x, (char) input);
            cursor_move = KEY_RIGHT;
            modified = 1;
            break;
    }

    if (res == E_SUCCESS)
    {
        if (cursor_move == KEY_BACKSPACE)
        {
            view->pos_x = temp_pos_x;
            view->pos_y = temp_pos_y;
        }

        update_cursor_position(view, cursor_move);

        const FileLine *line = get_file_data_line(view->data, view->scroll_offset + view->pos_y);
        if (!view->sel_active)
        {
            view->sel_start_line = line->line;
            view->sel_start_col = view->pos_x + line->col_start;
        }
        view->sel_stop_line = line->line;
        view->sel_stop_col = view->pos_x + line->col_start;

        if (modified)
        {
            view->status = FILE_VIEW_STATUS_MODIFIED;
        }
    }

    if (res < E_SUCCESS)
    {
        return E_INTERNAL_ERROR;
    }

    // FIXME: temporary for debug
    file_data_check_integrity(view->data);

    return E_SUCCESS;
}

int file_view_copy_selection(FileView *view, char **buffer, int *len)
{
    int sel_start_line, sel_start_col, sel_stop_line, sel_stop_col;
    int line_cols = getmaxx(view->win) - 1;
    file_view_get_selection_ranges(view, &sel_start_line, &sel_start_col, &sel_stop_line, &sel_stop_col);
    int size = (sel_stop_line - sel_start_line + 1) * line_cols;

    *buffer = (char*) malloc(size * sizeof(char));
    if (*buffer == NULL)
    {
        return E_INTERNAL_ERROR;
    }

    int start_sel = 0;
    *len = 0;
    for (int i = 0; i < view->data->size; i++)
    {
        const FileLine *line = get_file_data_line(view->data, i);
        int source_line = line->line;
        int source_col = line->col_start;

        for (int col = 0; col < line->size; col++)
        {
            if (source_line == sel_start_line && source_col + col == sel_start_col)
            {
                start_sel = 1;
            }

            if (source_line == sel_stop_line && source_col + col == sel_stop_col)
            {
                return E_SUCCESS;
            }

            // Append char to selection
            if (start_sel)
            {
                (*buffer)[*len] = line->content[col];
                (*len)++;
            }
        }

        if (source_line == sel_start_line && source_col + line->size == sel_start_col)
        {
            start_sel = 1;
        }

        if (start_sel && source_line == sel_stop_line && source_col + line->size == sel_stop_col)
        {
            return E_SUCCESS;
        }

        // Append new line to selection
        if (start_sel)
        {
            (*buffer)[*len] = '\n';
            (*len)++;
        }
    }

    return E_SUCCESS;
}

int file_view_delete_selection(FileView *view)
{
    int sel_start_line, sel_start_col, sel_stop_line, sel_stop_col;
    file_view_get_selection_ranges(view, &sel_start_line, &sel_start_col, &sel_stop_line, &sel_stop_col);

    int source_line = sel_stop_line, source_col = sel_stop_col;
    int pos_x, pos_y;
    if (file_data_get_display_coords(view->data, source_line, source_col, &pos_y, &pos_x) < 0)
    {
        return E_INTERNAL_ERROR;
    }

    view->pos_x = pos_x;
    view->pos_y = pos_y - view->scroll_offset;
    update_cursor_position(view, KEY_BACKSPACE);

    while(source_line != sel_start_line || source_col != sel_start_col)
    {
        file_view_handle_input(view, KEY_BACKSPACE);
        file_view_render(view);
        const FileLine *line = get_file_data_line(view->data, view->pos_y + view->scroll_offset);
        source_line = line->line;
        source_col = line->col_start + view->pos_x;
    }

    return E_SUCCESS;
}

const char *file_view_get_title(FileView *view)
{
    if (view == NULL)
    {
        return NULL;
    }

    return view->title;
}

const char* file_view_get_file_path(FileView *view)
{
    if (view == NULL)
    {
        return NULL;
    }

    return view->file_path;
}

FileViewStatus file_view_get_status(FileView *view)
{
    if (view == NULL)
    {
        return FILE_VIEW_STATUS_UNINITIALIZED;
    }

    return view->status;
}


// ----------------------- Private definitions -----------------------

void update_cursor_position(FileView *view, int input)
{
    // Get window dimensions
    int height = getmaxy(view->win);

    const FileLine *current_line = get_file_data_line(view->data, view->scroll_offset + view->pos_y);

    if (current_line == NULL)
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
        case KEY_SR:
            source_line--;
            break;

        case KEY_DOWN:
        case KEY_SF:
            source_line++;
            break;

        case KEY_LEFT:
        case KEY_SLEFT:
            if (source_col > 0)
            {
                source_col--;
            }
            break;

        case KEY_RIGHT:
        case KEY_SRIGHT:
            source_col++;
            break;

        case KEY_ENTER:
            source_line++;
            source_col = 0;
            break;

        case KEY_HOME:
        case KEY_SHOME:
            source_col = 0;
            break;

        case KEY_END:
        case KEY_SEND:
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

    if (view->file_path != NULL)
    {
        free(view->file_path);
    }

    view->file_path = (char*) malloc((strlen(file_path) + 1) * sizeof(char));
    if (view->file_path == NULL)
    {
        return E_INTERNAL_ERROR;
    }
    strcpy(view->file_path, file_path);

    // Set view title
    char *file_path_copy = (char*) malloc((strlen(file_path) + 1) * sizeof(char));
    if (file_path_copy == NULL)
    {
        return E_INTERNAL_ERROR;
    }
    strcpy(file_path_copy, file_path);

    const char *title_temp = basename(file_path_copy);

    if (view->title != NULL)
    {
        free(view->title);
    }

    view->title = (char*) malloc((strlen(title_temp) + 1) * sizeof(char));
    if (view->title == NULL)
    {
        free(file_path_copy);
        return E_INTERNAL_ERROR;
    }
    strcpy(view->title, title_temp);
    free(file_path_copy);

    return E_SUCCESS;
}

void file_view_get_selection_ranges(FileView *view, int *sel_start_line, int *sel_start_col, int *sel_stop_line, int *sel_stop_col)
{
    *sel_start_line = view->sel_start_line;
    *sel_start_col = view->sel_start_col;
    *sel_stop_line = view->sel_stop_line;
    *sel_stop_col = view->sel_stop_col;

    // swap if start > stop
    if ((*sel_stop_line < *sel_start_line) || (*sel_stop_line == *sel_start_line && *sel_stop_col < *sel_start_col))
    {
        *sel_start_line = view->sel_stop_line;
        *sel_start_col = view->sel_stop_col + 1;
        *sel_stop_line = view->sel_start_line;
        *sel_stop_col = view->sel_start_col + 1;
    }
}
