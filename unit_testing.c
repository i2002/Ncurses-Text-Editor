#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "file_data.h"

void print_file_data(FileData *file_data)
{
    for(int i = 0; i < file_data->size; i++)
    {
        const FileLine *data = get_file_data_line(file_data, i);
        printf("(%d:%d - %d) %s %c\n", data->line, data->col_start, data->size, data->content, data->endl ? '$' : '>');
    }
    printf("File lines: %d, display lines: %d\n", file_data->end != NULL ? file_data->end->data.line + 1 : 0, file_data->size);
}

int main()
{
    FileData file;
    create_file_data(3, &file);
    file_data_check_integrity(&file);

    load_file_data(&file, "test2.txt");
    file_data_check_integrity(&file);
    // print_file_data(&file);

    assert(file_data_delete_char(&file, 0, 0) == 0);
    file_data_check_integrity(&file);

    assert(file_data_delete_char(&file, 3, 0) == 0);
    file_data_check_integrity(&file);

    assert(file_data_delete_char(&file, 3, 0) == 0);
    file_data_check_integrity(&file);

    assert(file_data_delete_char(&file, 3, 0) == 0);
    file_data_check_integrity(&file);

    assert(file_data_delete_char(&file, 2, 0) == 0);
    file_data_check_integrity(&file);

    assert(file_data_delete_char(&file, 2, 0) == 0);
    file_data_check_integrity(&file);

    assert(file_data_delete_char(&file, 2, 0) == 0);
    file_data_check_integrity(&file);

    assert(file_data_insert_char(&file, 2, 0, 'a') == 0);
    file_data_check_integrity(&file);

    assert(file_data_insert_char(&file, 2, 0, 'b') == 0);
    file_data_check_integrity(&file);

    assert(file_data_insert_char(&file, 2, 0, 'c') == 0);
    file_data_check_integrity(&file);

    assert(file_data_insert_char(&file, 2, 0, 'd') == 0);
    file_data_check_integrity(&file);

    assert(file_data_insert_char(&file, 2, 1, 'y') == 0);
    file_data_check_integrity(&file);

    print_file_data(&file);
    assert(file_data_insert_char(&file, 2, 1, '\n') == 0);
    print_file_data(&file);
    file_data_check_integrity(&file);

    assert(file_data_insert_char(&file, 1, 2, '\n') == 0);
    file_data_check_integrity(&file);
    print_file_data(&file);

    assert(file_data_delete_char(&file, 4, -1) == 0);
    print_file_data(&file);
    file_data_check_integrity(&file);

    assert(file_data_insert_char(&file, 1, 2, 'x') == 0);
    file_data_check_integrity(&file);

    assert(file_data_insert_char(&file, 1, 3, 'p') == 0);
    file_data_check_integrity(&file);
    print_file_data(&file);

    // file_data_delete_char(&file, 0, 1);
    // file_data_delete_char(&file, 0, 1);
    return 0;
}
