#ifndef FAT32_H_
#define FAT32_H_

#include <wchar.h>
#include <stdint.h>

#include "work_with_file.h"

#define BOOT_SECTOR_SIZE    512
#define SHORT_NAME_SIZE     11
#define LONG_NAME_SIZE      256
#define DIR_ATTR            11
#define FAT_ENTRY_SIZE      32

#define LFN_FIRST_MASK      (0x40)
#define END_OF_DIRECTORY    (0x0)
#define FILE_DELETED        (0xE5)

/* LDIR names numerations start from zero. */
typedef enum {
    LDIR_Name1_start = 0,
    LDIR_Name2_start = 5,
    LDIR_Name3_start = 11,
    LDIR_Name_size = 13
}LDIR_Name;

/* Shifts in bytes to the LDIR names in
 *  32-byte entry for file with LFN. */
#define SHIFT_TO_LDIR_NAME1     1
#define SHIFT_TO_LDIR_NAME2     14
#define SHIFT_TO_LDIR_NAME3     28

/* FAT32 is represented in little endian byte order. */
typedef struct {
    uint16_t    BPB_BytsPerSec;
    uint8_t     BPB_SecPerClus;
    uint16_t    BPB_RsvdSecCnt;
    uint8_t     BPB_NumFATs;
    uint32_t    BPB_TotSec32;
    uint32_t    BPB_FATSz32;
    uint32_t    BPB_RootClus;
} bpb_t;

typedef struct {
    image_t *image;
    bpb_t bpb;
} fat_t;

uint8_t fat_init(fat_t *fat, image_t *image);

/* For FAT32 volumes, each FAT entry is 32 bits in length. */

typedef enum {
    CLUSTER_FREE = 0x0000000,
    CLUSTER_FINAL = 0xFFFFFFFF,
} CLUSTER;

uint32_t FAT_GetNextCluster(fat_t *fat, uint32_t current_cluster);

/* The first file (size is zero) in directory (directory id).
 * Without this file the FAT directory is considered damaged. */
#define FIRST_FILE_IN_DIR   ('.')

typedef enum {
    ATTR_READ_ONLY = 0x01,
    ATTR_HIDDEN = 0x02,
    ATTR_SYSTEM = 0x04,
    ATTR_VOLUME_ID = 0x08,
    ATTR_DIRECTORY = 0x10,
    ATTR_ARCHIVE = 0x20,
    ATTR_LONG_NAME =    ATTR_READ_ONLY |
                        ATTR_HIDDEN |
                        ATTR_SYSTEM |
                        ATTR_VOLUME_ID,
} ATTR;

typedef struct {
    uint8_t DIR_Name[SHORT_NAME_SIZE + 1];
    uint8_t DIR_Attr;
    uint32_t DIR_FstClus;
    uint32_t DIR_FileSize;
    wchar_t long_name[LONG_NAME_SIZE];
} dir_entry_t;

typedef struct {
    fat_t *fat;
    uint32_t start_cluster;
    uint32_t current_cluster;
    uint32_t current_offset;
} fat_dir_t;

uint8_t fat_dir_init(fat_dir_t *fat_dir, fat_t *fat, uint32_t start_cluster);

/* Returns 0 if entry was not read. */
uint8_t FatDir_GetNextEntry(fat_dir_t *fat_dir, dir_entry_t *dir_entry);

#endif /* FAT32_H_ */
