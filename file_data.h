#ifndef FILE_DATA_H
#define FILE_DATA_H

#define E_SUCCESS         0
#define E_INVALID_CHAR    1
#define E_INTERNAL_ERROR -1
#define E_IO_ERROR       -2
#define E_INVALID_ARGS   -3


typedef struct FileLine FileLine;
typedef struct FileNode FileNode;
typedef struct FileData FileData;

/**
 * @brief File data structure.
 */
struct FileData
{
    int size;
    int display_cols;
    FileNode *start;
    FileNode *end;
    FileNode *current;
    int current_index;
};

/**
 * @brief FileLine structure that contains information about a line in FileData.
 */
struct FileLine
{
    int size;
    int line;
    int col_start;
    int endl;
    char *content;
};


/**
 * @brief FileNode stucture that contains FileLines in linked list.
 */
struct FileNode
{
    FileLine data;
    FileNode *prev;
    FileNode *next;
}; 

/**
 * @brief Create a file data.
 * 
 * @param cols number of display columns
 * @param file_data pointer to FileData structure to be initialized
 * @return int 0 for success, 1 for failure
 */
int create_file_data(int cols, FileData *file_data);

/**
 * @brief Free an initialized FileData structure.
 * 
 * @param file_data pointer to initialized FileData structure
 */
void free_file_data(FileData *file_data);

/**
 * @brief Load contents of a file into an initialized FileData structure.
 * 
 * @param file_data pointer to initialized FileData structure
 * @param file_name name of the file to be loaded
 * @return int 0 for success, 1 for failure
 */
int load_file_data(FileData *file_data, const char* file_name);

/**
 * @brief Save FileData to file
 * 
 * @param file_data pointer to initialized FileData structure
 * @param file_path path of the output file
 * @return int 0 for success, 1 for failure
 */
int save_file_data(FileData *file_data, const char* file_path);

/**
 * @brief Resize the structure of the FileData number of columns.
 * 
 * @param file_data pointer to initialized FileData structure
 * @param cols the new number of display columns
 * @return int 0 for success, 1 for failure
 */
int resize_file_data_col(FileData *file_data, int cols);

/**
 * @brief Set the current file data line.
 * 
 * This is used for faster data access of a specific line.
 * 
 * @param file_data pointer to initialized FileData structure
 * @param index FileData line number
 * @return 0 for success, 1 for failure
 */
int set_file_data_line(FileData *file_data, int index);

/**
 * @brief Get file data line.
 * 
 * @param file_data pointer to initialized FileData structure
 * @param index FileData line number
 * @return pointer to line data or NULL if index invalid
 */
const FileLine* get_file_data_line(FileData *file_data, int index);

/**
 * @brief Insert character at position in FileData.
 * 
 * @param file_data pointer to initialized FileData structure
 * @param line position FileData line number
 * @param col position of insertion on the specified FileData line
 * @param ins character to be inserted
 * @return int 0 for success, 1 for failure
 */
int file_data_insert_char(FileData *file_data, int line, int col, char ins);

/**
 * @brief Delete character at position in FileData.
 * 
 * @param file_data pointer to initialized FileData structure
 * @param line position FileData line number
 * @param col position of deletion on the specified FileData line
 * @return int 0 for success, 1 for failure
 */
int file_data_delete_char(FileData *file_data, int line, int col);

/**
 * @brief Assert FileData structure integrity.
 * 
 * Check if linked list structure and internal data is correct.
 * The function uses asserts, so the program will end if any condition is not satisfied.
 * 
 * @param file_data pointer to initialized FileData structure
 */
void file_data_check_integrity(FileData *file_data);

/**
 * @brief Get display coords corresponding to source file info
 * 
 * @param file_data pointer to initialized FileData structure
 * @param source_line index of source file line
 * @param source_col index of source file column (can be -1 for last column)
 * @param display_line output parameter for corresponding display line
 * @param display_col output parameter for corresponding display column
 * @return int 0 for success, 1 for failure
 */
int file_data_get_display_coords(FileData *file_data, int source_line, int source_col, int *display_line, int *display_col);

#endif // FILE_DATA_H
