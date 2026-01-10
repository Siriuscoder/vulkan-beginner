#include "shader_io.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>

int read_file_to_memory(const char* path, void *buffer, size_t* size)
{
    FILE* file = fopen(path, "rb");
    if (!file)
    {
        fprintf(stderr, "Failed to open file %s: %s\n", path, strerror(errno));
        return 0;
    }

    if (!buffer)
    {
        if (fseek(file, 0, SEEK_END) != 0)
        {
            fprintf(stderr, "Failed to get file size: %s\n", strerror(errno));
        }

        *size = ftell(file);
        fseek(file, 0, SEEK_SET);
        return 1;
    } 

    if (fread(buffer, 1, *size, file) != *size)
    {
        if (feof(file))
            fprintf(stderr, "Failed to read %s: unexpected end of file\n", path);
        else if (ferror(file))
            fprintf(stderr, "Failed to read %s: %s", path, strerror(errno));

        fclose(file);
        return 0;
    }

    fclose(file);
    return 1;
}
