#ifndef _FSCACHE_H_
#define _FSCACHE_H_

#define FAT_BUF_512_NUM 4
#define DIR_BUF_512_NUM 4

typedef struct buf_512 {
	u32 sec;  // the num of sector stored in this buffer
	u8 state;  // it is like: 000000(ref bit)(dirty bit)
	u8 buf[512];
} BUF_512;

typedef struct buf_4k {  // 8 sectors per cluster
	u32 sec;  // the num of sector stored in this buffer
	u8 state;
	u8 buf[4096];
} BUF_4K;

// the following buffers are used as global:
BUF_512 fat_buf[FAT_BUF_512_NUM];
BUF_512 dir_buf[DIR_BUF_512_NUM];

u8 fat_clock_head;  // points to which block in the buffer,
u8 dir_clock_head;  // init to 0.

u8 victim_4k(BUF_4K *buf, u8 *clock_head);  // find a victim block to store data

int write_4k(BUF_4K *block);  // write the content of a block back to fs
// read 8 sectors from "start_sec" into the block:
int read_4k(BUF_4K *block, u32 start_sec, u8 *clock_head);

u8 victim_512(BUF_512 *buf, u8 *clock_head);  // almost the same as above

int write_512(BUF_512 *block);

int read_512(BUF_512 *block, u32 sec, u8 *clock_head);

// ===== some lower functions called by functions above: ===== //

// write the contents starting from "buf" to "dst_addr", write "num_blocks" blocks.
// can both write 8 sectors or just 1 sector, depending on "num_blocks".
int write_blocks(u8 *buf, u32 dst_addr, u8 num_blocks);

// read the contents starting from "src_addr" to "buf".
int read_blocks(u8 *buf, u32 src_addr, u8 num_blocks);

#endif
