#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "file_data.h"

void check_integrity(FileData *file_data)
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

        assert(data->new_line == (c->next == NULL || c->next->data.line != data->line)); // Check end of line marked correctly
    
        // - content integrity
        assert(data->size <= file_data->display_cols && data->size >= 0); // Display line size should not exceed configuration in file_data
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

void print_file_data(FileData *file_data)
{
    for(int i = 0; i < file_data->size; i++)
    {
        const FileLine *data = get_file_data_line(file_data, i);
        printf("(%d:%d - %d) %s %c\n", data->line, data->col_start, data->size, data->content, data->new_line ? '$' : '~');
    }
    printf("File lines: %d, display lines: %d\n", file_data->end != NULL ? file_data->end->data.line + 1 : 0, file_data->size);
}

int main()
{
    FileData file;
    create_file_data(3, &file);
    // check_integrity(&file);

    load_file_data(&file, "test.txt");
    check_integrity(&file);
    // print_file_data(&file);

    assert(file_data_delete_char(&file, 0, 0) == 0);
    check_integrity(&file);
    assert(file_data_delete_char(&file, 3, 0) == 0);
    check_integrity(&file);
    assert(file_data_delete_char(&file, 3, 0) == 0);
    check_integrity(&file);
    assert(file_data_delete_char(&file, 3, 0) == 0);
    check_integrity(&file);
    assert(file_data_delete_char(&file, 2, 0) == 0);
    check_integrity(&file);
    assert(file_data_delete_char(&file, 2, 0) == 0);
    check_integrity(&file);
    assert(file_data_delete_char(&file, 2, 0) == 0);
    check_integrity(&file);
    assert(file_data_insert_char(&file, 2, 0, 'a') == 0);
    check_integrity(&file);
    assert(file_data_insert_char(&file, 2, 0, 'b') == 0);
    check_integrity(&file);
    assert(file_data_insert_char(&file, 2, 0, 'c') == 0);
    check_integrity(&file);
    assert(file_data_insert_char(&file, 2, 0, 'd') == 0);
    check_integrity(&file);
    assert(file_data_insert_char(&file, 2, 1, 'y') == 0);
    check_integrity(&file);
    assert(file_data_insert_char(&file, 2, 1, '\n') == 0);
    check_integrity(&file);
    assert(file_data_insert_char(&file, 1, 2, '\n') == 0);
    check_integrity(&file);
    print_file_data(&file);
    assert(file_data_delete_char(&file, 4, -1) == 0);
    print_file_data(&file);
    check_integrity(&file);
    assert(file_data_insert_char(&file, 1, 2, 'x') == 0);
    check_integrity(&file);
    assert(file_data_insert_char(&file, 1, 3, 'p') == 0);
    check_integrity(&file);
    print_file_data(&file);
    // file_data_delete_char(&file, 0, 1);
    // file_data_delete_char(&file, 0, 1);
    return 0;
}
