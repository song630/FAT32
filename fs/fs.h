#ifndef _FS_H_
#define _FS_H_

#include "util.h"  // u8, u16, u32
#include "fscache.h"  // BUF_4K
#include <stdio.h>
#include <stdlib.h>

#define LOCAL_BUF_NUM 4  // in struct FILE
#define SD_PATH "D://virtual_disk.vhd"

#define FS_DEBUG

// some FAT32 featured data structures defined here:

#pragma pack (1)
// ===== NOTICE: __attribute__((__packed__)) does not work,
// so instead, use #pragma pack (1), which means aligning by 1 byte.
// w/o this, it will use natural alignment, which spares all members
// space of sizeof(the largest member).

struct /*__attribute__((__packed__))*/ DBR_struct {  // 512 bytes
	u8 jump_instr[3];  // 3 bytes, machine instruction
	// ===== ERROR: u32 edition[2]
	u8 edition[8];  // 8 bytes, edition of fs
	u16 bytes_per_sec; // 2 bytes, should be 512 =====
	u8 secs_per_clus;  // 1 byte, should be 8 =====
	u16 reserved_secs;  // 2 bytes, should be 38 =====
	u8 num_FAT;  // 1 byte, number of FAT tables =====
	u16 dummy1;  // 2 bytes, should be 0
	u16 dummy2;  // 2 bytes, should be 0
	u8 intermediate;  // 1 byte
	u16 dummy3;  // 2 bytes, should be 0
	u16 secs_per_track;  // 2 bytes, should be 63
	u16 heads;  // 2 bytes, should be 255
	u32 hidden_secs;  // 4 bytes, should be 8192
	u32 total_secs;  // 4 bytes, may be 7736320 =====
	u32 secs_per_FAT;  // 4 bytes, should be 7541 =====
	u16 mark;  // 2 bytes
	u16 FAT32_edition;  // 2 bytes
	u32 root_clus;  // 4 bytes, should be 2 =====
	u16 FSINFO_sec;  // 2 bytes, should be 1 =====
	u16 copy_DBR_SEC;  // 2 bytes, should be 6
	u8 extension[12]; // u32 extension[3];  // 12 bytes
	u8 dummy4;  // 1 byte
	u8 dummy5;  // 1 byte
	u8 ext_mark;  // 1 byte, should be 0x29
	u32 volume_sequence;  // 4 bytes
	u8 vol_mark[11];  // 11 bytes
	u8 fs_format[8];  // 8 bytes
	u8 unused[420];  // 410 bytes
	u8 signature[2]; // u16 signature;  // 2 bytes, should be 0x55AA
};  // important attrs have been marked "====="

#pragma pack ()  // return to default pack

typedef union DBR_union {  // 512 bytes
	u8 buf[512];  // stores raw data of DBR sector
	struct DBR_struct attrs;
} DBR_SEC;

typedef struct fsinfo_struct {
	u32 empty_clusters;  // 4 bytes, 489-492th bytes
	u32 next_avil_clus;  // 4 bytes, 493-496th bytes

	// other necessary info:
	u32 base_addr;
	u32 total_data_clusters;
	u32 total_data_sectors;
	u32 first_data_sector;

	u8 buf[512];  // stores raw data of FSINFO sector
} FSINFO_SEC;

#pragma pack (1)

struct /*__attribute__((__packed__))*/ short_dir_entry_struct {  // 32 bytes
	u8 fore_name[8];  // file name
	u8 ext_name[3];  // extension name
	u8 attr_byte;  // one-hot
	u8 reserved;
	u8 ms_10;  // 10ms bit
	u16 create_time;
	u16 create_date;
	u16 lastest_access_date;
	u16 start_clus_high_16;  // =====
	u16 latest_mod_time;
	u16 latest_mod_date;
	u16 start_clus_low_16;  // =====
	u32 length;  // ===== in bytes ?
};

#pragma pack ()

typedef union short_dir_entry_union {
	struct short_dir_entry_struct attrs;
	u8 entry[32];  // raw data
} SHORT_DIR_ENTRY;

typedef struct file_struct {
	unsigned char path[256];
	unsigned long loc;  // read & write pointer
	unsigned long dir_entry_pos;  // bytes offset of the dir-entry in sector =====
	// the sector including dir-entry (related to DBR sector) =====
	unsigned long dir_entry_sector;
	SHORT_DIR_ENTRY entry;  // current directory entry
	u8 clock_head;  // points to one of the buffer blocks
	BUF_4K data_buf[LOCAL_BUF_NUM];  // 4 local buffer blocks
} fs_FILE;

// the following structures are used as global in main process:
// ========================================== //
DBR_SEC DBR_sec;  // attrs and raw data
FSINFO_SEC FSINFO_sec;  // attrs and raw data
// ========================================== //

// ========== below are functions: ========== //

void init_FILE(fs_FILE *f_ptr);  // clear struct FILE

void init_FSINFO();  // clear struct FSINFO

void init_fat_buf();  // init global buffers

void init_dir_bufs();  // init global buffers

int init_MBR_DBR();

int write_FAT_sector(u8 index);  // write a sector of FAT table

int read_FAT_sector(u32 index);  // read a sector from FAT table
// iteratively deal with the names along the file path,
// convert them to the format that to be stored in short dir entry,
// and put the result in the given param.
int convert_name_between_slash(u8 *path, u32 *cur, u8 *short_name);
// accepts an absolute path, and find the file
// find every sub-dir by means of short dir entry
int find_file(fs_FILE *f_ptr);
// first find the file,
// and fill in info of "fs_FILE"
int open_file(fs_FILE *f_ptr, u8 *filename);
// write FSINFO back (2 times), and write back the global buffers
int flush_all();

int close_file(fs_FILE *f_ptr);

int read_file(fs_FILE *f_ptr, u8 *buf, u32 count);
// find, starting from "start", and store the result in "free_clus"
int find_free_clus(u32 start, u32 *free_clus);
// store the result in "clus"
int alloc_clus(u32 *clus);

int write_file(fs_FILE *f_ptr, u8 *buf, u32 count);

int lseek(fs_FILE *f_ptr, u32 offset);
// find a free dir entry, store the result in "free_entry"
int find_free_entry(u8 buf_index, u32 *free_entry);

int create_file(u8 *filename);
// given an dir entry, convert the short-dir-format name to normal,
// and save the result in "filename".
int get_file_name(u8 *entry, u8 *filename);
// store the result in "size"
int get_file_size(u8 *entry, u32 *size);

#endif
