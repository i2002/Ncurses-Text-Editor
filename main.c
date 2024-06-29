#include "text_editor.h"

int main()
{
    // Initialize ncurses
    initscr();
    start_color();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    // Initialize colors
    init_pair(1, COLOR_RED, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_BLUE, COLOR_BLACK);
    init_pair(4, COLOR_BLACK, COLOR_WHITE);
    init_pair(5, COLOR_WHITE, COLOR_BLUE);

    TextEditor *editor = create_text_editor();

    if (editor == NULL)
    {
        endwin();
        return 1;
    }

    int done = 0;
    while (!done)
    {
        int ch = getch();
        done = text_editor_handle_input(editor, ch);
    }

    free_text_editor(editor);
    endwin();
    return 0;
}
