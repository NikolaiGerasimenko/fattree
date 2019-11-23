#include "work_with_file.h"

size_t image_file_read(const image_t *image, size_t address, void *data, size_t length)
{
    fseek(FILE(image->interface), address, SEEK_SET);
    return fread(data, 1, length, FILE(image->interface));
}

uint8_t image_file_init(image_t *image, const char *filename)
{
    if ((image->interface = fopen(filename, "r")) == NULL) {
        fprintf(stderr, "Unable to open file %s\n", filename);
        return -1;
    }

    fseek(FILE(image->interface), 0, SEEK_END);
    image->size = ftell(FILE(image->interface));
    if (image->size < 0) {
      fprintf(stderr, "Unable to open file %s\n", filename);
      return -1;
    }
    return 0;
}
