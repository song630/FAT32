#ifndef _UTIL_H_
#define _UTIL_H_

// extern struct buf_4k;
typedef struct buf_512 BUF_512;

typedef unsigned char u8;  // 1 byte
typedef unsigned short u16;  // 2 bytes
typedef unsigned long u32;  // 4 bytes

int read_FAT_sector(u32 index);  // to be called by FAT_entry_from_clus()

u16 get_u16(u8 *addr);
u32 get_u32(u8 *addr);

// below are some operations defined for strings:

int fs_strlen(u8 *str);
int fs_strcmp(u8 *str1, u8 *str2);  // only has two results: equal & not equal
void fs_strcpy(u8 *dst_str, u8 *src_str);
// starting from the addr of "buf", set the (count) bytes "c":
void fs_memset(void *buf, u8 c, u32 count);
// copy (count) bytes from "src" to "dst":
void fs_memcpy(void *dst, void *src, u32 count);

// below are some functions to compute addr:

u32 sec_addr_from_clus(u32 clus_num);  // addr convertion
u32 clus_num_from_sec(u32 sec_num);  // addr convertion
u32 get_file_start_clus(u16 offset, BUF_512 *buf);  // from a directory entry
u32 FAT_entry_from_clus(u32 clus);

#endif
