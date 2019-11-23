#include <string.h>

#include "fat32.h"

static inline uint16_t le_buf_to_16(const uint8_t *buf)
{
    return (uint16_t)buf[0] |
          ((uint16_t)buf[1] << 8);
}

static inline uint32_t le_buf_to_32(const uint8_t *buf)
{
    return (uint32_t)buf[0] |
          ((uint32_t)buf[1] << 8) |
          ((uint32_t)buf[2] << 16)|
          ((uint32_t)buf[3] << 24);
}

/* The number of bytes per sector. */
static inline uint16_t parse_BPB_BytsPerSec(const uint8_t *bpb_buf)
{
    return le_buf_to_16(bpb_buf + 11);
}

/* The number of sectors per allocation unit. */
static inline uint8_t parse_BPB_SecPerClus(const uint8_t *bpb_buf)
{
    return bpb_buf[13];
}

/* The number of reserved sectors in the reserved region of the volume
 *  starting at the first sector of the volume. */
static inline uint16_t parse_BPB_RsvdSecCnt(const uint8_t *bpb_buf) {
    return le_buf_to_16(bpb_buf + 14);
}

/* The number of file allocation tables (FAT). */
static inline uint8_t parse_BPB_NumFATs(const uint8_t *bpb_buf)
{
    return bpb_buf[16];
}

/* Total number of blocks in the entire disk */
static inline uint32_t parse_BPB_TotSec32(const uint8_t *bpb_buf)
{
    return le_buf_to_32(bpb_buf + 32);
}

/* The FAT32 32-bit count of sectors occupied by one FAT. */
static inline uint32_t parse_BPB_FATSz32(const uint8_t *bpb_buf)
{
    return le_buf_to_32(bpb_buf + 36);
}

/* This is set to the cluster number of the first cluster of the root directory. */
static inline uint32_t parse_BPB_RootClus(const uint8_t *bpb_buf)
{
    return le_buf_to_32(bpb_buf + 44);
}

static uint8_t parse_bpb(bpb_t *bpb, const uint8_t *bpb_buf)
{
    bpb->BPB_BytsPerSec = parse_BPB_BytsPerSec(bpb_buf);
    bpb->BPB_SecPerClus = parse_BPB_SecPerClus(bpb_buf);
    bpb->BPB_RsvdSecCnt = parse_BPB_RsvdSecCnt(bpb_buf);
    bpb->BPB_NumFATs    = parse_BPB_NumFATs(bpb_buf);
    bpb->BPB_TotSec32   = parse_BPB_TotSec32(bpb_buf);
    bpb->BPB_FATSz32    = parse_BPB_FATSz32(bpb_buf);
    bpb->BPB_RootClus   = parse_BPB_RootClus(bpb_buf);
    return 0;
}

static uint32_t FatDir_ClusterToAddress(fat_t *fat, uint32_t cluster)
{
    return ( ((cluster - 2) * fat->bpb.BPB_SecPerClus + fat->bpb.BPB_RsvdSecCnt +
            fat->bpb.BPB_NumFATs * fat->bpb.BPB_FATSz32) * fat->bpb.BPB_BytsPerSec );
}

static uint8_t check_move_cluster(fat_dir_t *fat_dir, uint32_t *disk_offset)
{
    if(fat_dir->current_offset == fat_dir->fat->bpb.BPB_SecPerClus * fat_dir->fat->bpb.BPB_BytsPerSec) {
        uint32_t nextCluster = FAT_GetNextCluster(fat_dir->fat, fat_dir->current_cluster);
        if(nextCluster == CLUSTER_FINAL) {
            return 0;
        }
        else {
            fat_dir->current_cluster = nextCluster;
            fat_dir->current_offset = 0;
            *disk_offset = FatDir_ClusterToAddress(fat_dir->fat, fat_dir->current_cluster);
        }
    }
    return 1;
}

uint8_t fat_init(fat_t *fat, image_t *image)
{
    uint8_t bpb_buf[BOOT_SECTOR_SIZE];
    fat->image = image;
    image_file_read(fat->image, 0, bpb_buf, BOOT_SECTOR_SIZE);
    parse_bpb(&fat->bpb, bpb_buf);
    return 0;
}

uint32_t FAT_GetNextCluster(fat_t *fat, uint32_t current_cluster)
{
    uint8_t buf[4];
    uint32_t FAT_offset = current_cluster * 4;
    uint32_t FAT_sec_num = fat->bpb.BPB_RsvdSecCnt + (FAT_offset / fat->bpb.BPB_BytsPerSec);
    uint32_t FAT_ent_offset = FAT_offset % fat->bpb.BPB_BytsPerSec;
    uint32_t SectorNumber = (fat->bpb.BPB_NumFATs * fat->bpb.BPB_FATSz32) + FAT_sec_num;
    image_file_read(fat->image, SectorNumber * fat->bpb.BPB_BytsPerSec + FAT_ent_offset, buf, 4);

    return (le_buf_to_32(buf) & 0x0FFFFFFF);
}

uint8_t fat_dir_init(fat_dir_t *fat_dir, fat_t *fat, uint32_t start_cluster)
{
    fat_dir->fat = fat;
    fat_dir->start_cluster = start_cluster;
    fat_dir->current_cluster = fat_dir->start_cluster;
    fat_dir->current_offset = 0;
    return 0;
}

uint8_t parse_long_dir_entry(dir_entry_t *dir_entry, const uint8_t *dir_entry_buf)
{
    uint8_t LDIR_Ord = (dir_entry_buf[0] & (~LFN_FIRST_MASK)) - 1;

    for(int i = LDIR_Name1_start; i < LDIR_Name_size; i++) {
        if(i >= LDIR_Name1_start && i < LDIR_Name2_start) {
            dir_entry->long_name[LDIR_Ord * LDIR_Name_size + i] =
                    le_buf_to_16(dir_entry_buf + SHIFT_TO_LDIR_NAME1 + i * 2);
        } else if (i >= LDIR_Name2_start && i < LDIR_Name3_start) {
            dir_entry->long_name[LDIR_Ord * LDIR_Name_size + i] =
                    le_buf_to_16(dir_entry_buf + SHIFT_TO_LDIR_NAME2 + (i - LDIR_Name2_start) * 2);
        } else if (i >= LDIR_Name3_start && i < LDIR_Name_size) {
            dir_entry->long_name[LDIR_Ord * LDIR_Name_size + i] =
                    le_buf_to_16(dir_entry_buf + SHIFT_TO_LDIR_NAME3 + (i - LDIR_Name3_start) * 2);
        }
    }

    if((dir_entry_buf[0] & LFN_FIRST_MASK) == LFN_FIRST_MASK) {
        dir_entry->long_name[LDIR_Ord * LDIR_Name_size + LDIR_Name_size] = '\0';
    }
    return 0;
}

uint8_t parse_dir_entry(dir_entry_t *dir_entry, const uint8_t *dir_entry_buf)
{
    memcpy(dir_entry->DIR_Name, dir_entry_buf, SHORT_NAME_SIZE);
    dir_entry->DIR_Name[SHORT_NAME_SIZE] = '\0';
    dir_entry->DIR_Attr = dir_entry_buf[DIR_ATTR];
    dir_entry->DIR_FstClus = ((uint32_t)le_buf_to_16(dir_entry_buf + 26)) | ((uint32_t)le_buf_to_16(dir_entry_buf + 20) << 16);
    dir_entry->DIR_FileSize = le_buf_to_32(dir_entry_buf + 28);
    return 0;
}

uint8_t FatDir_GetNextEntry(fat_dir_t *fat_dir, dir_entry_t *dir_entry)
{
    uint8_t buf[FAT_ENTRY_SIZE];
    uint32_t image_offset = FatDir_ClusterToAddress(fat_dir->fat, fat_dir->current_cluster);
    image_file_read(fat_dir->fat->image, image_offset + fat_dir->current_offset, buf, FAT_ENTRY_SIZE);
    fat_dir->current_offset += FAT_ENTRY_SIZE;

    if(!check_move_cluster(fat_dir, &image_offset)) {
        return 0;
    }

    if((buf[DIR_ATTR] & ATTR_LONG_NAME) == ATTR_LONG_NAME) {
        uint8_t LDIR_Ord = (buf[0] & (~LFN_FIRST_MASK));
        for(int i = LDIR_Ord; i >= 1; i--) {
            parse_long_dir_entry(dir_entry, buf);
            image_file_read(fat_dir->fat->image, image_offset + fat_dir->current_offset, buf, FAT_ENTRY_SIZE);
            fat_dir->current_offset += FAT_ENTRY_SIZE;

            if(!check_move_cluster(fat_dir, &image_offset)) {
                return 0;
            }
        }
    }
    parse_dir_entry(dir_entry, buf);

    return ( (dir_entry->DIR_Name[0] != FILE_DELETED) && (dir_entry->DIR_Name[0] != END_OF_DIRECTORY) );
}
