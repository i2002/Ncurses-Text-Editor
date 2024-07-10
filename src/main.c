/**
 * ------------- Ncurses Text Editor --------------
 * Proiect Sisteme de operare, anul 3, semestrul 2
 * - Student: Butufei Tudor-David
 * - Grupa 361
 * 
 * Comments throughout the project are in english in order to perserve consistency.
 * 
 * Build instructions:
 * - make
 * - ./main
 * - make clean
 * 
 * Application components:
 * - FileData: represents the file as a linked list of display lines
 * - FileView: handles the view of a file tab (rendering and file input)
 * - TextEditor: renders the whole application and manages file tabs and application menu
 * - Dialogs: utilities to display dialogs (text input, confirm and alert)
 * - Colors: utilities related to terminal colors
 */
#include "text_editor.h"
#include "colors.h"

#include <stdlib.h>
#include <string.h>

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
        const char* kname = keyname(ch);

        if (strcmp(kname, "kLFT3") == 0)
        {
            ch = KEY_ALT_LEFT;
        }
        else if (strcmp(kname, "kRIT3") == 0)
        {
            ch = KEY_ALT_RIGHT;
        }
        else if (strcmp(kname, "kLFT4") == 0)
        {
            ch = KEY_ALT_SHIFT_LEFT;
        }
        else if (strcmp(kname, "kRIT4") == 0)
        {
            ch = KEY_ALT_SHIFT_RIGHT;
        }
        else if (strcmp(kname, "kLFT5") == 0)
        {
            ch = KEY_CTRL_LEFT;
        }
        else if (strcmp(kname, "kRIT5") == 0)
        {
            ch = KEY_CTRL_RIGHT;
        }
        else if (strcmp(kname, "kLFT6") == 0)
        {
            ch = KEY_CTRL_SHIFT_LEFT;
        }
        else if (strcmp(kname, "kRIT6") == 0)
        {
            ch = KEY_CTRL_SHIFT_RIGHT;
        }

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
