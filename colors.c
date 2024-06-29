#include "colors.h"

#include <ncurses.h>

void setup_colors()
{
    use_default_colors();
    init_pair(INTERFACE_COLOR, COLOR_BLACK, COLOR_WHITE);
    init_pair(INTERFACE_SELECTED_COLOR, COLOR_BLUE, COLOR_WHITE);
    init_pair(INTERFACE_DISABLED_COLOR, COLOR_MAGENTA, COLOR_WHITE);
    init_pair(MENU_COLOR, COLOR_WHITE, COLOR_BLUE);
    init_pair(MARKER_COLOR, COLOR_BLUE, -1);
}
