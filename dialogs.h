#ifndef DIALOGS_H
#define DIALOGS_H

#include <ncurses.h>
#include <panel.h>

/**
 * @brief Show text input dialog.
 * 
 * @param dialog_panel panel of the dialog
 * @param prompt title of the dialog
 * @param initial_input initial input of the dialog
 * @param buffer output buffer
 * @param buffer_len output buffer length
 * @return int 1 for success 0 if cancelled
 */
int input_dialog(PANEL *dialog_panel, const char *prompt, const char *initial_input, char *buffer, int buffer_len);

/**
 * @brief Show confirm (Yes / No) dialog.
 * 
 * @param dialog_panel panel of the dialog
 * @param prompt title of the dialog
 * @return int 1 if confirmed, 0 if cancelled
 */
int confirm_dialog(PANEL *dialog_panel, const char *prompt);

/**
 * @brief Show dismissable alert dialog.
 * 
 * @param dialog_panel panel of the dialog
 * @param message title of the dialog
 */
void alert_dialog(PANEL *dialog_panel, const char *message);

/**
 * @brief Render dialog window with border, title and background color.
 * 
 * @param win window of the dialog
 * @param title title of the dialog
 */
void render_dialog_window(WINDOW *win, const char *title);

#endif // DIALOGS_H
