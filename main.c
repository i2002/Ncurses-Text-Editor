#include "text_editor.h"
#include "colors.h"

#include <stdlib.h>

int main()
{
    // Initialize ncurses
    initscr();
    start_color();
    raw();
    noecho();
    keypad(stdscr, TRUE);
    mousemask(BUTTON1_CLICKED, NULL);
    ESCDELAY = 10;

    // Initialize colors
    setup_colors();

    // Initialized text editor
    TextEditor *editor = create_text_editor();

    if (editor == NULL)
    {
        endwin();
        exit(EXIT_FAILURE);
    }

    // App runtime
    int done = 0;
    while (!done)
    {
        int ch = getch();
        done = text_editor_handle_input(editor, ch);
    }

    // Free resources
    free_text_editor(editor);
    endwin();

    return EXIT_SUCCESS;
}
