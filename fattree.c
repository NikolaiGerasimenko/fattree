#include <stdio.h>
#include <string.h>

#include "fat32.h"
#include "work_with_file.h"

void list_dir_recurse(fat_dir_t *dir, uint32_t current_depth)
{
    dir_entry_t dir_entry;
    fat_dir_t fat_dir;

    while(FatDir_GetNextEntry(dir, &dir_entry)) {
        if(dir_entry.DIR_Name[0] == FIRST_FILE_IN_DIR || (dir_entry.DIR_Attr & ATTR_VOLUME_ID) == ATTR_VOLUME_ID) {
            continue;
        }

        for(int i = 0; i < current_depth; ++i) {
            wprintf(L" | ");
        }
        wprintf(L"%ls", dir_entry.long_name);
        if((dir_entry.DIR_Attr & ATTR_DIRECTORY) == ATTR_DIRECTORY) {
            fat_dir_init(&fat_dir, dir->fat, dir_entry.DIR_FstClus);
            wprintf(L" <DIR>\n");
            list_dir_recurse(&fat_dir, current_depth + 1);
        } else {
            wprintf(L"\n");
        }
    }
}

void print_usage(char *program)
{
    printf("Program to show image as file and folder tree. \n"
           "To show image as file and folder tree run the program with next param:\n"
           "'image.img' - Path to an image file \n\n"

           "Usage: %s [image.img]\n", program);
}

int main(int argc, char *argv[])
{
    image_t image;
    fat_t fat;
    fat_dir_t fat_dir;
    const char *image_name = argv[1];

    if (argc == 2  && (!strcmp(argv[1], "-h"))) {
        print_usage(argv[0]);
        return -1;
    }
    else if ( argc != 2 ) {
        printf("To prints program usage run it with param '-h': './fattree -h'\n");
        return -1;
    }

    image_file_init(&image, image_name);
    fat_init(&fat, &image);

    fat_dir_init(&fat_dir, &fat, fat.bpb.BPB_RootClus);
    list_dir_recurse(&fat_dir, 0);

    return 0;
}
