/*
    turntable - client-to-server audio streaming
    Copyright (c) 2021-2022 Ally Sommers

    This code is licensed under the BSD 3-Clause License. A copy of this
    license is included with this code.
*/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

size_t get_file_size(char const *const path) {
    FILE *fp = fopen(path, "rb");
    if (fp == NULL) {
        printf("Failed to open file '%s'\n", path);
        return 0;
    }
    struct stat file_info;
    fstat(fileno(fp), &file_info);
    fclose(fp);
    return (size_t)file_info.st_size;
}

size_t load_from_file(char const *const path, char *data, size_t len) {
    FILE *fp = fopen(path, "rb");
    if (fp == NULL) {
        printf("Failed to open file '%s'\n", path);
        return 0;
    }
    struct stat file_info;
    fstat(fileno(fp), &file_info);
    off_t file_size = file_info.st_size;

    if (file_size < 0) {
        fclose(fp);
        return 0;
    }

    size_t result = fread(data, 1, (size_t)file_size > len ? len : (size_t)file_size, fp);
    fclose(fp);
    return result;
}
