#include <panel.h>
#include <string.h>
#include <stdlib.h>
#include "file_data.h"

typedef struct
{
    WINDOW *win;
    PANEL *panel;
    char *label;
    FileData *data;
    int scroll_offset;
    int pos_x;
    int pos_y;
} TabData;

void render_tabs(WINDOW *win, char **labels, int len, int current);
void render_tab(TabData *tab);
void update_cursor_position(TabData *tab, int input);
void handle_input(TabData *tab, int input);

WINDOW* init_win(int id)
{
    WINDOW *win = newwin(LINES - 3, COLS, 3, 0);
    // mvwprintw(win, 0, 0, "Hello from %d", id);
    return win;
}

typedef struct {

} File;

int main()
{
    /* Initialize curses */
    initscr();
    start_color();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    /* Initialize all the colors */
    init_pair(1, COLOR_RED, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_BLUE, COLOR_BLACK);
    init_pair(4, COLOR_BLACK, COLOR_WHITE);

    WINDOW *tabs_win;
    PANEL *tabs_panel;
    TabData tabs[4];
    int ch;

    // Initialize window
    char *tab_labels[] = {
        "Tab 1",
        "Tab 2aaaaaaaaaaaa",
        "Tab 3bbbbbbbbbddbbbcc",
        "Tab 4"
    };
    tabs_win = newwin(3, COLS, 0, 0);
    for (int i = 0; i < 4; i++)    
    {
        tabs[i].win = init_win(i);
        tabs[i].data = (FileData *) malloc(sizeof(FileData));
        create_file_data(COLS - 1, tabs[i].data);
        load_file_data(tabs[i].data, "test.txt");
        tabs[i].label = tab_labels[i];
        tabs[i].pos_x = 0;
        tabs[i].pos_y = 0;
        tabs[i].scroll_offset = 0;

    }

    tabs_panel = new_panel(tabs_win);
    for (int i = 0; i < 4; i++)
    {
        tabs[i].panel = new_panel(tabs[i].win);
    }
    render_tabs(tabs_win, tab_labels, 4, 0);

    top_panel(tabs[0].panel);
    update_panels();

    FileData file;

    /* Show it on the screen */
    // attron(COLOR_PAIR(4));
    // mvprintw(LINES - 2, 0, "Use tab to browse through the windows (F1 to Exit)");
    // attroff(COLOR_PAIR(4));
    doupdate();


    // check_integrity(&file);


    int current = 0;
    int scroll_offset = 0;
    int pos_x = 0;
    int pos_y = 0;

    render_tab(tabs + current);
    update_panels();
    doupdate();
    while ((ch = getch()) != KEY_F(1))
    {
        switch (ch)
        {
        case KEY_UP:
        case KEY_DOWN:
        case KEY_LEFT:
        case KEY_RIGHT:
        case KEY_HOME:
        case KEY_END:
            update_cursor_position(tabs + current, ch);
            render_tab(tabs + current);
            break;

        case 9:
            current = (current + 1) % 4;
            top_panel(tabs[current].panel);
            render_tabs(tabs_win, tab_labels, 4, current);
            break;

        default:
            handle_input(tabs + current, ch);
            render_tab(tabs + current);
            break;
        }
        update_panels();
        doupdate();
    }
    endwin();
    return 0;
}

void render_tabs(WINDOW *win, char **labels, int len, int current)
{
    int pos, i, start_i;
    int height, width;
    getmaxyx(win, height, width);

    werase(win);
    pos = 0;
    start_i = 0;

    for (i = 0; i <= current; i++)
    {
        pos += strlen(labels[i]) + (i == 0 ? 2 : 3);
    }

    while (pos >= width - 4 && start_i < current)
    {
        pos -= strlen(labels[start_i]) + (start_i == 0 ? 2 : 3);
        start_i++;
    }

    pos = 2;
    if (start_i == 0)
    {
        wattron(win, COLOR_PAIR(3));
    }
    mvwaddch(win, 1, 0, '<');
    if (start_i == 0)
    {
        wattroff(win, COLOR_PAIR(3));
    }
    for (i = start_i; i < len && pos < width; i++)
    {
        pos++;
        if (i == current)
        {
            wattron(win, COLOR_PAIR(4));
        }

        mvwaddch(win, 1, pos - 1, ' ');
        mvwaddnstr(win, 1, pos, labels[i], width - 2 - pos);
        mvwaddch(win, 1, pos + strlen(labels[i]), ' ');
        if (i == current)
        {
            wattroff(win, COLOR_PAIR(4));
        }
        pos += strlen(labels[i]) + 2;

        if (i < len - 1)
        {
            mvwaddch(win, 1, pos - 1, '|');
        }
    }

    if (i == len)
    {
        wattron(win, COLOR_PAIR(3));
    }
    mvwaddch(win, 1, width - 1, '>');
    if (i == len)
    {
        wattroff(win, COLOR_PAIR(3));
    }
    mvwhline(win, 0, 0, ACS_S9, width);
    mvwhline(win, 2, 0, ACS_S1, width);
}

void render_tab(TabData *tab)
{
    int height, width;
    getmaxyx(tab->win, height, width);

    for (int i = 0; i < height; i++)
    {
        if (i + tab->scroll_offset < tab->data->size)
        {
            const FileLine *line = get_file_data_line(tab->data, i + tab->scroll_offset);
            mvwaddnstr(tab->win, i, 0, line->content, line->size);

            if (!line->new_line)
            {
                wattron(tab->win, COLOR_PAIR(3));
                mvwaddch(tab->win, i, width - 1, '~');
                wattroff(tab->win, COLOR_PAIR(3));
            }
        }
        else
        {
            wattron(tab->win, COLOR_PAIR(3));
            mvwaddch(tab->win, i, 0, '~');
            wattroff(tab->win, COLOR_PAIR(3));
        }
        wclrtoeol(tab->win);
    }

    wmove(tab->win, height - 1, 0);
    const FileLine *current_line = get_file_data_line(tab->data, tab->scroll_offset + tab->pos_y);
    wprintw(tab->win, "(d x: %d, d y: %d, i: %d, s line: %d, s col: %d, size: %d, endl: %d)", tab->pos_x, tab->pos_y, tab->pos_y + tab->scroll_offset, current_line->line, current_line->col_start + tab->pos_x, current_line->size, current_line->new_line);
    wmove(tab->win, tab->pos_y, tab->pos_x);
}

void update_cursor_position(TabData *tab, int input)
{
    // Get window dimensions
    int height, width;
    getmaxyx(tab->win, height, width);

    const FileLine *current_line = get_file_data_line(tab->data, tab->scroll_offset + tab->pos_y);

    int temp_x, temp_y;

    int source_line = current_line->line;
    int source_col = current_line->col_start + tab->pos_x;

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

    if (file_data_get_display_coords(tab->data, source_line, source_col, &temp_y, &temp_x) == 0)
    {
        tab->pos_y = temp_y - tab->scroll_offset;
        tab->pos_x = temp_x;
    }

    // Adjust scroll offset
    if (tab->pos_y <= 0 && tab->scroll_offset > 0)
    {
        tab->scroll_offset += tab->pos_y - 1;
        tab->pos_y = 1;
    }

    if (tab->pos_y >= height - 2) // FIXME: status bar output
    {
        tab->scroll_offset += tab->pos_y - height + 3;
        tab->pos_y = height - 3;
    }
}

void handle_input(TabData *tab, int input)
{
    int res = 1;
    int cursor_move = 0;
    int temp_pos_x = tab->pos_x, temp_pos_y = tab->pos_y;

    if (input == KEY_BACKSPACE)
    {
        const FileLine *line = get_file_data_line(tab->data, tab->scroll_offset + tab->pos_y);
        int source_line = line->line;
        int source_col = line->col_start + tab->pos_x - 1;

        if (source_col == -1)
        {
            source_line--;
        }
        
        file_data_get_display_coords(tab->data, source_line, source_col, &temp_pos_y, &temp_pos_x);
        temp_pos_y -= tab->scroll_offset;
    }

    switch (input)
    {
        case KEY_ENTER:
        case '\n':
            res = file_data_insert_char(tab->data, tab->pos_y + tab->scroll_offset, tab->pos_x, '\n');
            cursor_move = KEY_ENTER;
            break;

        case KEY_BACKSPACE:
            res = file_data_delete_char(tab->data, tab->pos_y + tab->scroll_offset, tab->pos_x - 1);
            cursor_move = KEY_BACKSPACE;
            break;

        default:
            res = file_data_insert_char(tab->data, tab->pos_y + tab->scroll_offset, tab->pos_x, (char) input);
            cursor_move = KEY_RIGHT;
            break;
    }

    if (res == 0)
    {
        if (cursor_move == KEY_BACKSPACE)
        {
            tab->pos_x = temp_pos_x;
            tab->pos_y = temp_pos_y;
        }

        update_cursor_position(tab, cursor_move);
    }

    file_data_check_integrity(tab->data);
}
