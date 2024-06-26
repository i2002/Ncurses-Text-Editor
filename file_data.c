#include "file_data.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

// ----------------------------- Private declarations -----------------------------

static FileNode* insert_node(FileData *file_data, FileNode *node, int line, int col_start, char *content_buffer, int len);
static void delete_node(FileData *file_data, FileNode *node);
static FileNode* find_node(const FileData *file_data, int index);
static FileNode* normalize_line(FileData *file_data, FileNode *node);
static void free_node_data(FileNode *node);
static void write_line(FileLine *line, char *buffer, int len);
static char shift_chars(char *buffer, int start, int stop);
static void update_col_start(FileNode *start, int value);
static void update_line(FileNode *start, int value);
static int last_in_line(FileNode *node);


// ------------------------- Public functions definitions -------------------------

int create_file_data(int cols, FileData *file_data)
{
    if (file_data == NULL || cols < 1)
    {
        return 1;
    }

    file_data->display_cols = cols;
    file_data->size = 0;
    file_data->start = NULL;
    file_data->end = NULL;
    file_data->current = NULL;
    file_data->current_index = -1;

    return 0;
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

int load_file_data(FileData *file_data, char *file_name)
{
    if (file_data == NULL || file_data->start != NULL)
    {
        return 1;
    }

    FILE *fin = fopen(file_name, "r");
    if (fin == NULL)
    {
        perror("Unable to open file"); // FIXME: file opening error handling
        return 1;
    }

    char *buffer = (char*) malloc(file_data->display_cols * sizeof(char));
    if (buffer == NULL)
    {
        fclose(fin);
        return 1;
    }

    int buffer_index = 0;
    int real_line = 0;
    int col_start = 0;
    char ch;

    while ((ch = fgetc(fin)) != EOF)
    {
        // Append character to the buffer
        if (ch != '\n')
        {
            buffer[buffer_index++] = ch;
        }

        // Current display line finished (line full or newline character encountered and non null buffer)
        if (buffer_index == file_data->display_cols || ch == '\n')
        {
            // Insert node with the new display line
            FileNode *node = insert_node(file_data, file_data->end, real_line, col_start, buffer, buffer_index);

            if (node == NULL)
            {
                free(buffer);
                fclose(fin);
                return 1;
            }

            // Reset buffer index and update column start
            buffer_index = 0;
            col_start += file_data->display_cols;

            // If newline character was encountered, reset column start and increment real line count
            if (ch == '\n')
            {
                col_start = 0;
                real_line++;
                node->data.new_line = 1;
            }
        }
    }

    // Handle any remaining characters in the buffer after EOF
    if (buffer_index > 0)
    {
        // Insert node with the new display line
        FileNode *node = insert_node(file_data, file_data->end, real_line, col_start, buffer, buffer_index);

        if (node == NULL)
        {
            free(buffer);
            fclose(fin);
            return 1;
        }
    }

    file_data->end->data.new_line = 1;

    free(buffer);
    fclose(fin);
    return 0;
}

// int resize_file_data_col(FileData *file_data, int cols)
// {
//     if (file_data == NULL || cols < 1)
//     {
//         return 1;
//     }

//     FileNode *current = file_data->start;
//     FileData new_file_data;
//     create_file_data(cols, )
// }

int set_file_data_line(FileData *file_data, int index)
{
    if (file_data == NULL || index < 0 || index >= file_data->size)
    {
        return 1;
    }

    FileNode* new_current = find_node(file_data, index);

    if (new_current == NULL)
    {
        return 1;
    }

    file_data->current = new_current;
    file_data->current_index = index;
    return 0;
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
        return 1;
    }

    FileNode *node = find_node(file_data, line);
    FileLine *data = &(node->data);

    // Edge case for inserting at the end of a source file line
    int max_col = last_in_line(node) ? data->size : data->size - 1;
    if (col > max_col)
    {
        return 1;
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
            if (last_in_line(node))
            {
                FileNode *new_node = insert_node(file_data, node, data->line, data->col_start + file_data->display_cols, NULL, 0);

                if (new_node == NULL)
                {
                    return 1;
                }

                data->new_line = 0;
                new_node->data.new_line = 1;
            }

            // Insert overflow character on the next line
            int ret = file_data_insert_char(file_data, line + 1, 0, overflow_char);

            if (ret == 1)
            {
                return 1;
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

        // Insert new node
        FileNode *new_node = insert_node(file_data, node, data->line, 0, buffer, len);

        if (new_node == NULL)
        {
            return 1;
        }

        new_node->data.new_line = 1;

        // Remove moved content from line
        data->size = col;
        data->content[data->size] = '\0';
        data->new_line = 1;

        // Shift content of subsequent lines
        (void) normalize_line(file_data, new_node);

        // Update line 
        update_line(new_node, 1);
    }

    return 0;
}

int file_data_delete_char(FileData *file_data, int line, int col)
{
    FileNode *node = find_node(file_data, line);

    if (node == NULL || col >= node->data.size || col < -1)
    {
        return 1;
    }

    // Merge with previous line if it exits
    if (col == -1)
    {
        if (node->prev != NULL && node->prev->data.new_line)
        {
            update_line(node, -1);
            node->prev->data.new_line = 0;
            (void) normalize_line(file_data, node->prev);
        }

        return 0;
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

    return 0;
}


// ------------------------- Private functions definitions -------------------------

/**
 * @brief Insert new FileNode to FileData structure.
 * 
 * The newly inserted node is a succesor to the node given as parameter (NULL for the start node).
 * The node data is initialized with provided values.
 * If provided, the data from content buffer is copied to the data of the created node
 * 
 * @param file_data FileData structure in which the node should be added
 * @param node the node after which this new node should be inserted (NULL for the first node)
 * @param line the source file line number of the node data
 * @param col_start the index of the start character on the source file line
 * @param content_buffer buffer to copy line content (NULL if no copy is wanted)
 * @param len the length of the content buffer (should be 0 if content_buffer is NULL,
 *            should not exceed number of display columns configured in FileData structure)
 * @return FileNode* pointer to the new created node or NULL on error
 */
static FileNode* insert_node(FileData *file_data, FileNode *node, int line, int col_start, char *content_buffer, int len)
{
    if (file_data == NULL || (node == NULL && file_data->size != 0) || (content_buffer == NULL && len != 0)) // FIXME: node can be NULL
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
    new_node->data.new_line = 0;

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

/**
 * @brief Delete node from FileData structure.
 * 
 * @param file_data pointer to FileData structure
 * @param node pointer to the node to be deleted
 */
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
    if (node->data.new_line && node->prev != NULL && node->prev->data.line == node->data.line)
    {
        node->prev->data.new_line = 1;
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
static FileNode* normalize_line(FileData *file_data, FileNode *node)
{
    // Stop conditions
    if (node == NULL || 
        node->next == NULL || 
        node->data.line != node->next->data.line)
    {
        return node;
    }

    FileLine *line = &node->data;
    FileLine *nextLine = &node->next->data;

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

    if (move_len == nextLine->size)
    {
        // Next line has been emptied and can be removed
        delete_node(file_data, node->next);
    }
    else
    {
        // Shift remaining characters on the next line and update length
        int left_len = nextLine->size - move_len;

        memmove(nextLine->content, nextLine->content + move_len, (left_len) * sizeof(char));
        nextLine->content[left_len] = '\0';

        nextLine->col_start = line->col_start + file_data->display_cols;
        nextLine->size = left_len;
    }

    // Update line flags
    line->new_line = node->next == NULL || node->next->data.line != line->line;

    // Recursive call for next line, or current line if not completelly filled
    FileNode *nextNode = line->size != file_data->display_cols ? node : node->next;
    return normalize_line(file_data, nextNode);
}

/**
 * @brief Free node inner data structure.
 * 
 * @param node pointer to node with data to be freed
 */
static void free_node_data(FileNode *node)
{
    free(node->data.content);
    node->data.content = NULL;
    node->data.size = 0;
    node->data.line = 0;
    node->data.col_start = 0;
}

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

/**
 * @brief Update the col start of display lines starting from node
 * 
 * @param start the start node
 * @param value the value to be added to col_start
 */
static void update_col_start(FileNode *start, int value)
{
    FileNode *c = start;

    while (c != NULL && start->data.line == c->data.line)
    {
        c->data.col_start += value;
        c = c->next;
    }
}

/**
 * @brief Update the line number of display lines starting from node.
 * 
 * @param start the start node
 * @param value the value to be added to line
 */
static void update_line(FileNode *start, int value)
{
    FileNode *c = start;

    while (c != NULL)
    {
        c->data.line += value;
        c = c->next;
    }
}

/**
 * @brief Checks if the given node is the last display line in its source file line
 * 
 * @param node pointer to node
 * @return int 0 for false 1 for true
 */
static int last_in_line(FileNode *node)
{
    if (node == NULL)
    {
        return 0;
    }

    return node->next == NULL || node->data.line != node->next->data.line;
}
