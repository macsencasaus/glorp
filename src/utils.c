#include "utils.h"

#include <stdlib.h>
#include <stdio.h>

extern const char *program_name;

char *read_file(const char *file_name) {
    FILE *file = fopen(file_name, "r");
    if (file == NULL) {
        fprintf(stderr, "%s: %s: No such file or directory\n", program_name,
                file_name);
        exit(1);
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    char *buffer = (char *)malloc(file_size + 1);

    size_t bytes_read = fread(buffer, 1, file_size, file);
    if ((long)bytes_read != file_size) {
        fprintf(stderr, "%s: %s: Failed to read file\n", program_name,
                file_name);
        free(buffer);
        fclose(file);
        exit(1);
    }

    buffer[file_size] = 0;

    fclose(file);
    return buffer;
}

char *read_stdin(void) {
    int c;
    size_t size = 1024;
    size_t len = 0;
    char *buffer = (char *)malloc(size);
    if (buffer == NULL) {
        fprintf(stderr, "%s: Failed to read from stdin\n", program_name);
        exit(1);
    }

    while ((c = fgetc(stdin)) != EOF) {
        buffer[len++] = c;

        if (len == size) {
            size *= 2;

            char *temp = buffer;
            buffer = realloc(buffer, size);
            if (buffer == NULL) {
                free(temp);
                fprintf(stderr, "%s: Failed to read from stdin\n",
                        program_name);
                exit(1);
            }
        }
    }

    buffer[len] = 0;

    return buffer;
}
