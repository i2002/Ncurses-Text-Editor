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
            update_cursor_position(tabs + current, ch);
            render_tab(tabs + current);
            break;

        case 9:
            current = (current + 1) % 4;
            top_panel(tabs[current].panel);
            render_tabs(tabs_win, tab_labels, 4, current);
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

    for (int i = tab->scroll_offset; i - tab->scroll_offset < height; i++)
    {
        if (i < tab->data->size)
        {
            const FileLine *line = get_file_data_line(tab->data, i);
            mvwaddnstr(tab->win, i, 0, line->content, line->size);
        }
        else
        {
            wattron(tab->win, COLOR_PAIR(3));
            mvwaddch(tab->win, i, 0, '~');
            wattroff(tab->win, COLOR_PAIR(3));
        }
    }
    wmove(tab->win, tab->pos_y, tab->pos_x);
}

void update_cursor_position(TabData *tab, int input)
{
    // Get window dimensions
    int height, width;
    getmaxyx(tab->win, height, width);

    const FileLine *prev_line = get_file_data_line(tab->data, tab->scroll_offset + tab->pos_y - 1);
    const FileLine *current_line = get_file_data_line(tab->data, tab->scroll_offset + tab->pos_y);
    const FileLine *next_line = get_file_data_line(tab->data, tab->scroll_offset + tab->pos_y + 1);

    switch(input)
    {
        case KEY_UP:
            if (tab->pos_y > 0)
            {
                tab->pos_y--;
                if (tab->pos_x >= prev_line->size)
                {
                    tab->pos_x = prev_line->size - 1;
                }
            }
            break;

        case KEY_DOWN:
            if (tab->pos_y < height - 1 && tab->pos_y + tab->scroll_offset < tab->data->size - 1)
            {
                tab->pos_y++;
                
                if (tab->pos_x >= next_line->size)
                {
                    tab->pos_x = next_line->size - 1;
                }
            }
            break;

        case KEY_LEFT:
            if (tab->pos_x > 0)
            {
                tab->pos_x--;
            }
            else if (tab->pos_x == 0 && prev_line != NULL && prev_line->line == current_line->line)
            {
                tab->pos_y--;
                tab->pos_x = prev_line->size - 1;
            }
            break;

        case KEY_RIGHT:
            if (tab->pos_x == width - 2 && next_line != NULL && next_line->line == current_line->line)
            {
                tab->pos_y++;
                tab->pos_x = 0;
            }
            else if (tab->pos_x < width - 1 && tab->pos_x < current_line->size - 1)
            {
                tab->pos_x++;
            }
            break;
    }

    // Adjust scroll offset
    if (tab->pos_y == 0 && tab->scroll_offset > 0)
    {
        tab->scroll_offset--;
        tab->pos_y++;
    }

    if (tab->pos_y == height - 1)
    {
        tab->scroll_offset++;
        tab->pos_y--;
    }
}
