#include "file_data.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#define TAB_SIZE 4

// ----------------------------- Private declarations -----------------------------

/**
 * @brief Insert new FileNode to FileData structure.
 * 
 * The newly inserted node is a succesor to the node given as parameter (NULL for the start node).
 * The node data is initialized with provided values.
 * If provided, the data from content buffer is copied to the data of the created node.
 * 
 * By default any new node is marked as end of line. If the previous node is on the same source file
 * line as the inserted node, the previous node will have its end of line flag removed.
 * 
 * @param file_data FileData structure in which the node should be added
 * @param node the node after which this new node should be inserted (NULL for the first node)
 * @param line the source file line number of the node data
 * @param col_start the index of the start character on the source file line
 * @param endl flag if the current display line is the last in the source file line
 * @param content_buffer buffer to copy line content (NULL if no copy is wanted)
 * @param len the length of the content buffer (should be 0 if content_buffer is NULL,
 *            should not exceed number of display columns configured in FileData structure)
 * @return FileNode* pointer to the new created node or NULL on error
 */
static FileNode* insert_node(FileData *file_data, FileNode *node, int line, int col_start, int endl, char *content_buffer, int len);

/**
 * @brief Delete node from FileData structure.
 * 
 * @param file_data pointer to FileData structure
 * @param node pointer to the node to be deleted
 */
static void delete_node(FileData *file_data, FileNode *node);

/**
 * @brief Find linkded list node by index.
 * 
 * @param file_data pointer to FileData structure
 * @param index node index
 * @return FileNode* pointer to found node or NULL if not found or invalid index
 */
static FileNode* find_node(const FileData *file_data, int index);

/**
 * @brief Shift contents of display lines to perserve structure.
 * 
 * Each source file line can be represented by multiple display lines.
 * Each display line corresponding to a source file line is completed,
 * with the exception of the last line which may have arbitrary length.
 * 
 * This function recursivelly normalizes content following an incomplete line.
 * 
 * If a line has been completely emptied, it will be removed.
 * 
 * @param file_data pointer to FileData structure
 * @param node node with line to be normalized
 * @return Node* the first node of the next source line
 */
static FileNode* normalize_line(FileData *file_data, FileNode *node);

/**
 * @brief Free node inner data structure.
 * 
 * @param node pointer to node with data to be freed
 */
static void free_node_data(FileNode *node);

/**
 * @brief Shift characters in buffer.
 * 
 * The caracter on position start is removed and an empty space is being made on position stop.
 * The other characters are shifted:
 * - to the left, if start < stop 
 * - to the right, if start > stop
 * The removed character is returned
 * 
 * @param buffer character buffer where the shifting occurs (should have at least length max(start, stop) + 1)
 * @param start the start index of the shifting (the character on this position will be removed)
 * @param stop the stop index of the shifting (an empty space will be created on this position)
 * @return char 
 */
static char shift_chars(char *buffer, int start, int stop);

/**
 * @brief Copy buffer to FileLine.
 * 
 * This function updates the inner structure of the FileLine (the size of the line),
 * and ensures proper null string termination.
 * 
 * The length should be less than the line size defined in FileData.
 * 
 * @param line pointer to target FileLine structure
 * @param buffer pointer to source character buffer
 * @param len number of chars to be copied from buffer
 */
static void write_line(FileLine *line, char *buffer, int len);

// /**
//  * @brief Update the col start of display lines starting from node
//  * 
//  * @param start the start node
//  * @param value the value to be added to col_start
//  */
// static void update_col_start(FileNode *start, int value);
static void update_line(FileNode *start, int value);

/**
 * @brief Update the line number of display lines starting from node.
 * 
 * @param start the start node
 * @param value the value to be added to line
 */
static char fget_next_char(FILE *f, int *tab_count);

/**
 * @brief Check if input character is valid for display.
 * 
 * @param c character
 * @return int 0 if false, 1 if true
 */
static int valid_character(int c);


// ------------------------- Public functions definitions -------------------------

int create_file_data(int cols, FileData *file_data)
{
    if (file_data == NULL || cols < 1)
    {
        return E_INVALID_ARGS;
    }

    file_data->display_cols = cols;
    file_data->size = 0;
    file_data->start = NULL;
    file_data->end = NULL;
    file_data->current = NULL;
    file_data->current_index = -1;

    insert_node(file_data, NULL, 0, 0, 1, NULL, 0);
    return E_SUCCESS;
}

void free_file_data(FileData *file_data)
{
    FileNode *node = file_data->start;
    FileNode *del_node;

    while (node != NULL)
    {
        del_node = node;
        node = node->next;
        free_node_data(del_node);
        free(del_node);
    }

    file_data->start = NULL;
    file_data->end = NULL;
    file_data->current = NULL;
    file_data->current_index = -1;
    file_data->size = 0;
}

int load_file_data(FileData *file_data, const char *file_name)
{
    if (file_data == NULL)
    {
        return E_INVALID_ARGS;
    }

    free_file_data(file_data);

    FILE *fin = fopen(file_name, "r");
    if (fin == NULL)
    {
        return E_IO_ERROR;
    }

    char *buffer = (char*) malloc(file_data->display_cols * sizeof(char));
    if (buffer == NULL)
    {
        fclose(fin);
        return E_INTERNAL_ERROR;
    }

    int buffer_index = 0;
    int real_line = 0;
    int col_start = 0;
    int tab_count = 0;
    char ch;
    while ((ch = fget_next_char(fin, &tab_count)) != EOF)
    {
        // Append character to the buffer
        if (ch != '\n')
        {
            buffer[buffer_index++] = ch;
        }

        // Current display line finished (line full or newline character encountered and non null buffer)
        if (buffer_index == file_data->display_cols || (ch == '\n' && (buffer_index > 0 || col_start == 0)))
        {
            // Previous display line is no longer the last
            if (col_start != 0)
            {
                file_data->end->data.endl = 0;
            }

            // Insert node with the new display line
            FileNode *node = insert_node(file_data, file_data->end, real_line, col_start, 1, buffer, buffer_index);

            if (node == NULL)
            {
                free(buffer);
                fclose(fin);
                return E_INTERNAL_ERROR;
            }

            // Reset buffer index and update column start
            buffer_index = 0;
            col_start += file_data->display_cols;
        }

        // If newline character was encountered, reset column start and increment real line count
        if (ch == '\n')
        {
            col_start = 0;
            real_line++;
        }
    }

    // Handle any remaining characters in the buffer after EOF
    if (buffer_index > 0)
    {
        // Previous display line is no longer the last
        if (col_start != 0)
        {
            file_data->end->data.endl = 0;
        }

        // Insert node with the new display line
        FileNode *node = insert_node(file_data, file_data->end, real_line, col_start, 1, buffer, buffer_index);

        if (node == NULL)
        {
            free(buffer);
            fclose(fin);
            return E_INTERNAL_ERROR;
        }
    }

    free(buffer);
    fclose(fin);
    return E_SUCCESS;
}

int save_file_data(FileData *file_data, const char* file_path)
{
    if (file_data == NULL || file_path == NULL)
    {
        return E_INVALID_ARGS;
    }

    FILE *fout = fopen(file_path, "w");
    if (fout == NULL)
    {
        return E_IO_ERROR;
    }

    FileNode *c = file_data->start;
    while(c != NULL)
    {
        for (int i = 0; i < c->data.size; i++)
        {
            fputc(c->data.content[i], fout);
        }

        if (c->data.endl)
        {
            fputc('\n', fout);
        }

        c = c->next;
    }

    fclose(fout);
    return E_SUCCESS;
}

int resize_file_data_col(FileData *file_data, int cols)
{
    if (file_data == NULL || cols < 1)
    {
        return E_INVALID_ARGS;
    }

    FileNode *c = file_data->start;
    while(c != NULL)
    {
        FileLine *data = &(c->data);
        if (data->size > cols)
        {
            int len = data->size - cols;
            char *buffer = data->content + cols;

            // Insert new line
            FileNode *new_node = insert_node(file_data, c, data->line, data->col_start + cols, data->endl, buffer, len);
            
            if (new_node == NULL)
            {
                return E_INTERNAL_ERROR;
            }

            // Remove moved content from line
            data->size = cols;
            data->content[data->size] = '\0';
            data->endl = 0;
        }

        // Resize content buffer
        char *new_content = realloc(data->content, cols + 1);
        if (new_content == NULL)
        {
            return E_INTERNAL_ERROR;
        }

        data->content = new_content;

        c = c->next;
    }

    // Normalize lines
    file_data->display_cols = cols;
    FileNode *line_start = file_data->start;
    while(line_start != NULL)
    {
        line_start = normalize_line(file_data, line_start);
    }

    return E_SUCCESS;
}

int set_file_data_line(FileData *file_data, int index)
{
    if (file_data == NULL || index < 0 || index >= file_data->size)
    {
        return E_INVALID_ARGS;
    }

    FileNode* new_current = find_node(file_data, index);

    if (new_current == NULL)
    {
        return E_INTERNAL_ERROR;
    }

    file_data->current = new_current;
    file_data->current_index = index;
    return E_SUCCESS;
}

const FileLine* get_file_data_line(FileData *file_data, int index)
{
    if (file_data == NULL || index < 0 || index >= file_data->size)
    {
        return NULL;
    }

    FileNode *node = find_node(file_data, index);

    if (node == NULL)
    {
        return NULL;
    }

    return &node->data;
}

int file_data_insert_char(FileData *file_data, int line, int col, char ins)
{
    if (file_data == NULL || line < 0 || line >= file_data->size || col < 0)
    {
        return E_INVALID_ARGS;
    }

    if (!valid_character(ins))
    {
        return E_INVALID_CHAR;
    }

    FileNode *node = find_node(file_data, line);
    FileLine *data = &(node->data);

    // Edge case for inserting at the end of a source file line
    int max_col = data->endl ? data->size : data->size - 1;
    if (col > max_col)
    {
        return E_INVALID_ARGS;
    }

    if (ins != '\n')
    {
        int overflow = data->size == file_data->display_cols || col == file_data->display_cols;
        int start = overflow ? data->size - 1 : data->size + 1;
    
        char overflow_char = ins;
        
        if (col < file_data->display_cols)
        {
            overflow_char = shift_chars(data->content, start, col);
            data->content[col] = ins;
        }

        // Overflow character to be moved to next line
        if (overflow)
        {
            // Create new empty display line
            if (data->endl)
            {
                FileNode *new_node = insert_node(file_data, node, data->line, data->col_start + file_data->display_cols, 1, NULL, 0);

                if (new_node == NULL)
                {
                    return E_INTERNAL_ERROR;
                }

                // Previous display line is no longer the last
                node->data.endl = 0;
            }

            // Insert overflow character on the next line
            if (file_data_insert_char(file_data, line + 1, 0, overflow_char) < 0)
            {
                return E_INTERNAL_ERROR;
            }
        }
        else
        {
            data->size++;
        }
    }
    else
    {
        // Pointer to data to be copied
        int len = data->size - col;
        char *buffer = data->content + col;

        // Insert new line
        FileNode *new_node = insert_node(file_data, node, data->line + 1, 0, data->endl, buffer, len);
        if (new_node == NULL)
        {
            return E_INTERNAL_ERROR;
        }

        // Shift subsequent lines
        update_line(new_node->next, 1);

        // Remove moved content from line
        data->size = col;
        data->content[data->size] = '\0';
        data->endl = 1;

        // Shift content of subsequent lines
        (void) normalize_line(file_data, new_node);
    }

    return E_SUCCESS;
}

int file_data_delete_char(FileData *file_data, int line, int col)
{
    FileNode *node = find_node(file_data, line);

    if (node == NULL || col >= node->data.size || col < -1)
    {
        return E_INVALID_ARGS;
    }

    if (col == -1)
    {
        if (node->prev != NULL)
        {
            if (node->prev->data.endl)
            {
                // Merge with previous line if it exits
                update_line(node, -1);
                node->prev->data.endl = 0;
                (void) normalize_line(file_data, node->prev);
                return E_SUCCESS;
            }
            else
            {
                // Delete last chracter on previous line
                return file_data_delete_char(file_data, line - 1, node->prev->data.size - 1);
            }
        }

        return E_INVALID_CHAR;
    }

    // Delete character on line
    (void) shift_chars(node->data.content, col, node->data.size);
    
    // Shift characters around display lines
    if (node->next != NULL && node->data.line == node->next->data.line)
    {
        node->data.content[node->data.size - 1] = node->next->data.content[0];
        return file_data_delete_char(file_data, line + 1, 0);
    }
    else
    {
        node->data.size--;
        node->data.content[node->data.size] = '\0';

        if (node->data.size == 0 && node->data.col_start != 0)
        {
            delete_node(file_data, node);
        }
    }

    return E_SUCCESS;
}

void file_data_check_integrity(FileData *file_data)
{
    // FileData assertions
    assert((file_data->current_index >= 0 && file_data->current_index < file_data->size) || (file_data->current_index == -1 && file_data->current == NULL)); // Current node should be part of internal structure
    assert(file_data->size >= 0); // Positive size
    assert(file_data->display_cols > 0); // Positive non 0 number of display columns

    // Node iteration
    int count = 0;
    FileNode *c = file_data->start;
    FileNode *prev = NULL;

    while(c != NULL)
    {
        // Linked list assertions
        assert((c == file_data->start && c->prev == NULL) || c->prev != NULL); // Prev navigation if not first node
        assert((c == file_data->end && c->next == NULL) || c->next != NULL); // Next navigation if not last node
        assert(c->prev == prev); // Check backward navigation
        assert((c == file_data->current && count == file_data->current_index) || count != file_data->current_index); // Current node index should correspond with position in structure and no other

        // FileLine assertions
        FileLine *data = &c->data;

        // - line integrity
        if (c->prev != NULL)
        {
            FileLine *lastData = &c->prev->data;
            if (data->line == lastData->line)
            {
                assert(lastData->size == file_data->display_cols); // Current display line should continue only a completed previous display line if on the same source file line
                assert(data->col_start == lastData->col_start + file_data->display_cols); // Col start should keep consistency
            }
            else
            {
                assert(data->line == lastData->line + 1); // No jumps in source file line number
                assert(data->col_start == 0); // First display line in source file line starts at character 0
            }
        }

        assert(data->endl == (c->next == NULL || c->next->data.line != data->line)); // Check end of line marked correctly
    
        // - content integrity
        assert(data->size <= file_data->display_cols && data->size >= 0); // Display line size should not exceed configuration in file_data
        assert((data->size == 0 && data->col_start == 0) || data->size != 0); // Only the beginning of the line can be empty
        assert(data->content[data->size] == '\0'); // Display line content should be null terminated at size

        for (int i = 0; i < data->size; i++)
        {
            assert(data->content[i] != '\0'); // No null characters inside display line content
        }

        // Next iteration
        prev = c;
        c = c->next;
        count++;
    }

    assert(count == file_data->size); // Number of display lines should correspond with iterated nodes
    assert(file_data->end == prev); // Last visited node should be the end one
}

int file_data_get_display_coords(FileData *file_data, int source_line, int source_col, int *display_line, int *display_col)
{
    if (file_data == NULL || display_line == NULL || display_col == NULL)
    {
        return E_INVALID_ARGS;
    }

    FileNode* node = file_data->current != NULL ? file_data->current : file_data->start;
    int current_index = file_data->current != NULL ? file_data->current_index : 0;

    if (node == NULL)
    {
        return E_INVALID_ARGS;
    }

    // Set iteration direction
    int dir = 1;
    if (source_line < node->data.line || (source_line == node->data.line && source_col < node->data.col_start && source_col != -1))
    {
        dir = -1;
    }

    int i = current_index;
    while (node != NULL)
    {
        if (source_line == node->data.line)
        {
            int col_min = node->data.col_start;
            if (node->data.col_start == 0)
            {
                col_min--;
            }

            int col_min_ok = source_col > col_min;
            int col_max_ok = source_col <= node->data.col_start + node->data.size;

            // source column greater than source line length
            if (!col_max_ok && node->data.endl)
            {
                source_col = -1;
            }

            int col_end_ok = source_col == -1 && node->data.endl;

            // Found the display line
            if ((col_min_ok && col_max_ok) || col_end_ok)
            {
                *display_line = i;
                *display_col = (source_col == -1) ? node->data.size : source_col - node->data.col_start;
                return E_SUCCESS;
            }
        }

        // Go to the next node
        i += dir;
        node = (dir == 1) ? node->next : node->prev;
    }

    return E_INVALID_ARGS;
}


// ------------------------- Private functions definitions -------------------------

static FileNode* insert_node(FileData *file_data, FileNode *node, int line, int col_start, int endl, char *content_buffer, int len)
{
    if (file_data == NULL || (content_buffer == NULL && len != 0))
    {
        return NULL;
    }

    // Allocate memory for node
    FileNode *new_node = (FileNode*) malloc(sizeof(FileNode));

    if (new_node == NULL)
    {
        return NULL;
    }

    new_node->data.content = (char*) malloc((file_data->display_cols + 1) * sizeof(char));

    if (new_node->data.content == NULL)
    {
        free(new_node);
        return NULL;
    }

    // Initialize node data
    new_node->data.line = line;
    new_node->data.col_start = col_start;
    new_node->data.endl = endl;

    // Copy data from buffer into new line
    write_line(&new_node->data, content_buffer, len);

    // Update linked list structure
    new_node->next = node != NULL ? node->next : NULL;
    new_node->prev = node;

    // If the node to be inserted is not the last one
    if (node != NULL && node->next != NULL)
    {
        node->next->prev = new_node;
    }
    else
    {
        file_data->end = new_node;
    }

    // If the node to be inserted is not the first one
    if (node != NULL)
    {
        node->next = new_node;
    }
    else
    {
        file_data->start = new_node;
    }

    // Update file data size
    file_data->size++;

    return new_node;
}

static void delete_node(FileData *file_data, FileNode *node)
{
    if (node == NULL || file_data == NULL)
    {
        return;
    }

    // If the node to be deleted is the start node
    if (node->prev == NULL)
    {
        file_data->start = node->next;
        if (file_data->start != NULL)
        {
            file_data->start->prev = NULL;
        }
    }
    else
    {
        node->prev->next = node->next;
    }

    // If the node to be deleted is the last node
    if (node->next == NULL)
    {
        file_data->end = node->prev;
        if (file_data->end != NULL)
        {
            file_data->end->next = NULL;
        }
    }
    else
    {
        node->next->prev = node->prev;
    }

    // Update the current pointer if it points to the node being deleted
    if (file_data->current == node)
    {
        file_data->current = node->next != NULL ? node->next : node->prev;
        
        if (node->next == NULL)
        {
            file_data->current_index--;
        }
    }
    
    // Update line flags for previous node
    if (node->data.endl && node->prev != NULL && node->prev->data.line == node->data.line)
    {
        node->prev->data.endl = 1;
    }

    // Update file data size
    file_data->size--;

    // Free deleted node
    free_node_data(node);
    free(node);
}

static FileNode* find_node(const FileData *file_data, int index)
{
    // Ensure the index is within bounds
    if (file_data == NULL || index < 0 || index >= file_data->size)
    {
        return NULL;
    }

    FileNode* node = file_data->current != NULL ? file_data->current : file_data->start;
    int current_index = file_data->current != NULL ? file_data->current_index : 0;

    // Find the index by going forwards or backwards in the list starting with current node
    if (index > current_index)
    {
        for (int i = current_index; i < index && node != NULL; i++)
        {
            node = node->next;
        }
    }
    else if (index < current_index)
    {
        for (int i = current_index; i > index && node != NULL; i--)
        {
            node = node->prev;
        }
    }

    // // Check if the node is found
    // if (node != NULL)
    // {
    //     // Update the current index and pointer
    //     file_data->current = node;
    //     file_data->current_index = index;
    // }

    return node;
}

static FileNode* normalize_line(FileData *file_data, FileNode *node)
{
    // Stop conditions
    if (node == NULL)
    {
        return node;
    }
    else if (node->data.endl)
    {
        return node->next;
    }

    FileLine *line = &node->data;
    FileLine *nextLine = &node->next->data;

    if (nextLine->size == 0)
    {
        // Inherit the end of line flag from deleted node
        line->endl = nextLine->endl;

        // Next line has been emptied and can be removed
        delete_node(file_data, node->next);

        // Retry with next line
        return normalize_line(file_data, node);
    }

    if (node->data.size == file_data->display_cols)
    {
        nextLine->col_start = line->col_start + file_data->display_cols;
        return normalize_line(file_data, node->next);
    }

    int move_len = file_data->display_cols - line->size;
    if (move_len > nextLine->size)
    {
        // Next line is shorter than the needed number of characters
        move_len = nextLine->size;
    }

    // Move characters from next line into current line
    memcpy(line->content + line->size, nextLine->content, move_len * sizeof(char));
    line->size += move_len;
    line->content[line->size] = '\0';

    // Shift remaining characters on the next line and update length
    int left_len = nextLine->size - move_len;

    if (left_len > 0)
    {
        memmove(nextLine->content, nextLine->content + move_len, left_len * sizeof(char));
    }

    nextLine->content[left_len] = '\0';
    nextLine->col_start = line->col_start + file_data->display_cols;
    nextLine->size = left_len;


    // Go to the next node if current line completely filled and no deletion pending
    FileNode *nextNode = node;
    if (line->size == file_data->display_cols && left_len != 0)
    {
        nextNode = node->next;
    }
    return normalize_line(file_data, nextNode);
}

static void free_node_data(FileNode *node)
{
    free(node->data.content);
    node->data.content = NULL;
    node->data.size = 0;
    node->data.line = 0;
    node->data.col_start = 0;
}

static char shift_chars(char *buffer, int start, int stop)
{
    char overflow = buffer[start];
    int dir = start < stop ? 1 : -1;

    for (int i = start; dir * i < dir * stop; i += dir) 
    {
        buffer[i] = buffer[i + dir];
    }

    buffer[stop] = '\0';
    return overflow;
}

static void write_line(FileLine *line, char *buffer, int len)
{
    if (line == NULL || (buffer == NULL && len != 0) || len < 0)
    {
        return;
    }

    if (len > 0)
    {
        strncpy(line->content, buffer, len);
    }

    line->content[len] = '\0';
    line->size = len;
}

// static void update_col_start(FileNode *start, int value)
// {
//     FileNode *c = start;

//     while (c != NULL && start->data.line == c->data.line)
//     {
//         c->data.col_start += value;
//         c = c->next;
//     }
// }

static void update_line(FileNode *start, int value)
{
    FileNode *c = start;

    while (c != NULL)
    {
        c->data.line += value;
        c = c->next;
    }
}

static char fget_next_char(FILE *f, int *tab_count)
{
    char ch;

    // Normal input
    if (*tab_count == 0)
    {
        ch = fgetc(f);
    
        // Start of tab input
        if (ch == '\t')
        {
            ch = ' ';
            *tab_count = 1;
        }

        // Skip invalid character input
        if (!valid_character(ch))
        {
            return fget_next_char(f, tab_count);
        }
    }
    // Tab input
    else
    {
        (*tab_count)++;
        ch = ' ';

        if (*tab_count == TAB_SIZE)
        {
            *tab_count = 0;
        }
    }

    return ch;
}

static int valid_character(int c)
{
    if (c == EOF || c == '\n')
    {
        return 1;
    }

    return c >= 32 && c < 128;
}
