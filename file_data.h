#ifndef FILE_DATA_H
#define FILE_DATA_H

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

enum FileDataReturnType
{
    SUCCESS,
    UNINITIALIZED_FILE_DATA,
    // FIXME:
};

/**
 * @brief Create a file data
 * 
 * @param cols number of display columns
 * @param file_data pointer to FileData structure to be initialized
 * @return int 0 for success, 1 for failure
 */
int create_file_data(int cols, FileData *file_data);

/**
 * @brief Free an initialized FileData structure
 * 
 * @param file_data pointer to initialized FileData structure
 */
void free_file_data(FileData *file_data);

/**
 * @brief Load contents of a file into an initialized FileData structure
 * 
 * @param file_data pointer to initialized FileData structure
 * @param file_name name of the file to be loaded
 * @return int 0 for success, 1 for failure
 */
int load_file_data(FileData *file_data, char* file_name);

/**
 * @brief Resize the structure of the FileData number of columns
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
 * @brief Delete character at position in FileData
 * 
 * @param file_data pointer to initialized FileData structure
 * @param line position FileData line number
 * @param col position of deletion on the specified FileData line
 * @return int 0 for success, 1 for failure
 */
int file_data_delete_char(FileData *file_data, int line, int col);

#endif // FILE_DATA_H
