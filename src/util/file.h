/*
    turntable - client-to-server audio streaming
    Copyright (c) 2021-2022 Ally Sommers

    This code is licensed under the BSD 3-Clause License. A copy of this
    license is included with this code.
*/

#ifndef TURNTABLE_FILE_UTIL_H
#define TURNTABLE_FILE_UTIL_H

#include <stdlib.h>
#include <sys/types.h>

#define SIZE_KB 1000UL
#define SIZE_MB 1000000UL
#define SIZE_GB 1000000000UL

size_t get_file_size(char const *const path);
size_t load_from_file(char const *const path, char *data, size_t len);

#endif /* TURNTABLE_FILE_UTIL_H */
