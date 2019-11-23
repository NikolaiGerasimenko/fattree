#ifndef WORK_WITH_FILE_H
#define WORK_WITH_FILE_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define FILE(interface)     ((FILE *) interface)

typedef struct {
    void *interface;
    size_t size;
} image_t;

size_t image_file_read(const image_t *image, size_t address, void *data, size_t length);
uint8_t image_file_init(image_t *image, const char *filename);

#endif // WORK_WITH_FILE_H
