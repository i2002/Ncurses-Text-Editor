#ifndef FILE_VIEW_H
#define FILE_VIEW_H

#include <panel.h>
#include "file_data.h"

typedef struct FileView FileView;
typedef enum FileViewStatus FileViewStatus;

enum FileViewStatus
{
    FILE_VIEW_STATUS_UNINITIALIZED,
    FILE_VIEW_STATUS_NEW_FILE,
    FILE_VIEW_STATUS_MODIFIED,
    FILE_VIEW_STATUS_SAVED
};

struct FileView
{
    WINDOW *win;
    PANEL *panel;

    char *title;
    char *file_path;
    FileData *data;
    FileViewStatus status;

    int scroll_offset;
    int pos_x;
    int pos_y;

    // TODO: selection info
};

/**
 * @brief Create FileView structure.
 * 
 * In order to be used, the FileView needs to be initialized with
 * @ref file_view_new_file() or @ref file_view_load_file()
 * 
 * @param height the height of the view window
 * @param width the width of the view window
 * @param offset_y the vertical offest of the view window
 * @param offset_x the horizontal offset of the view window
 * @return FileView* newly created FileView or NULL on error
 */
FileView* create_file_view(int height, int width, int offset_y, int offset_x);

/**
 * @brief Free FileView instance.
 * 
 * @param view pointer to initialized FileView structure
 */
void free_file_view(FileView *view);

/**
 * @brief Load contents of file into data.
 * 
 * @param view pointer to initialized FileView structure
 * @param file_name name of the file to be loaded
 * @return int 0 for success, 1 for failure
 */
int file_view_load_file(FileView *view, const char *file_name);

/**
 * @brief Save file data into file.
 * 
 * @param view pointer to initialized FileView structure
 * @param file_path path of the output file
 * @return int 0 for success, 1 for failure
 */
int file_view_save_file(FileView *view, const char *file_path);

/**
 * @brief Render data into view.
 * 
 * @param view pointer to initialized FileView structure
 */
void file_view_render(FileView *view);

/**
 * @brief Update view status after keyboard input.
 * 
 * @param view pointer to initialized FileView structure
 * @param input input character / key
 */
void file_view_handle_input(FileView *view, int input);

/**
 * @brief Get the title of the current file view
 * 
 * @param view pointer to initialized FileView structure
 * @return const char* pointer to title data
 */
const char *file_view_get_title(FileView *view);

/**
 * @brief Get the path of the file loaded into the file view
 * 
 * @param view pointer to initialized FileView structure
 * @return const char* pointer to file path (may be NULL if file was never saved)
 */
const char* file_view_get_file_path(FileView *view);

/**
 * @brief Get the status of the file view
 * 
 * @param view pointer to initialized FileView structure
 * @return FileViewStatus
 */
FileViewStatus file_view_get_status(FileView *view);

#endif // FILE_VIEW_H
