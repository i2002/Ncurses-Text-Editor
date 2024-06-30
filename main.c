#include "text_editor.h"
#include "colors.h"

#include <stdlib.h>

TextEditor *editor = NULL;

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
    editor = create_text_editor();
    update_panels();
    doupdate();

    if (editor == NULL)
    {
        endwin();
        exit(EXIT_FAILURE);
    }

    // App runtime
    int ret = 0;
    while (!ret)
    {
        int ch = getch();
        ret = text_editor_handle_input(editor, ch);
    }

    // Free resources
    free_text_editor(editor);
    endwin();

    if (ret < 0)
    {
        fprintf(stderr, "Text Editor: internal error");
        exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}
