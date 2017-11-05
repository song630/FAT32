#include "fs.h"
#include "fscache.h"
#include "fs_debug.h"
#include "util.h"

#include "fscache.c"
#include "util.c"

// init global buffers: fat_buf[], dir_buf[]
void init_fat_buf()
{
	int i;
	for (i = 0; i <= 3; i++)
	{
		fat_buf[i].state &= 0x00;
		fat_buf[i].sec = 0xFFFFFFFF;  // an invalid value
		fs_memset(fat_buf[i].buf, 0x00, 512);
	}
}

// init global buffers: fat_buf[], dir_buf[]
void init_dir_buf()
{
	int i;
	for (i = 0; i <= 3; i++)
	{
		dir_buf[i].state &= 0x00;
		dir_buf[i].sec = 0xFFFFFFFF;  // an invalid value
		fs_memset(dir_buf[i].buf, 0x00, 512);
	}
}

int init_MBR_DBR()
{
	u8 MBR_buf[512];  // a temp buf holding MBR
	// 1. first: read the MBR sector, and get the addr of DBR sector
	// notice: read_512() can only be used to read a sector which is after "base_addr",
	// since MBR is behind "base_addr", call read_blocks() instead.
	fs_memset(MBR_buf, 0x00, 512);
	read_blocks(MBR_buf, 0, 1);  // read MBR (at absolute 0 sector)
	#ifdef FS_DEBUG
	printf("inside init_MBR_DBR(): MBR read in. The contents of MBR: \n");
	dump_block_512(MBR_buf);  // print the sector MBR
	#endif

	// 2. then get DBR and FSINFO from info in MBR:
	init_FSINFO();
	FSINFO_sec.base_addr = get_u32(MBR_buf + 446 + 8);  // the sector index of DBR
	// DBR is 0 sector; FSINFO is 1 sector, so "base_addr + 1".
	// but the data should not be put in global buffer, so cannot call read_512().
	read_blocks(FSINFO_sec.buf, FSINFO_sec.base_addr + 1, 1);
	#ifdef FS_DEBUG
	printf("inside init_MBR_DBR(): FSINFO read in. The contents of FSINFO: \n");
	dump_block_512(FSINFO_sec.buf);  // print the sector MBR
	#endif
	read_blocks(DBR_sec.buf, FSINFO_sec.base_addr, 1);  // read in DBR sector
	#ifdef FS_DEBUG
	printf("inside init_MBR_DBR(): DBR read in. The contents of DBR: \n");
	dump_block_512(DBR_sec.buf);  // print the sector MBR
	#endif

	// 3. set some attrs, and check if the values of some attrs are correct.
	printf("bytes_per_sec: %x.\n", DBR_sec.attrs.intermediate);
	// ===== 2 problems: ===== //
	// 1. sth wrong with alignment
	// 2. in read_blocks(): DBR_sec.attrs.bytes_per_sec, DBR_sec.attrs.total_secs
	//    have not been assigned.

	return 0;
}

void init_FILE(fs_FILE *f_ptr)
{
	int i;
	fs_memset(f_ptr->path, 0x00, 256);  // clear path[]
	f_ptr->loc = 0xFFFFFFFF;
	f_ptr->dir_entry_pos = 0xFFFFFFFF;
	f_ptr->dir_entry_sector = 0xFFFFFFFF;
	fs_memset(f_ptr->entry.entry, 0x00, 32);
	f_ptr->clock_head = 0;
	for (i = 0; i <= LOCAL_BUF_NUM - 1; i++)  // init the local buffer within
	{
		f_ptr->data_buf[i].sec = 0xFFFFFFFF;
		f_ptr->data_buf[i].state &= 0x00;
		fs_memset(f_ptr->data_buf[i].buf, 0x00, 4096);
	}
}

void init_FSINFO()
{
	FSINFO_sec.empty_clusters = 0xFFFFFFFF;
	FSINFO_sec.next_avil_clus = 0xFFFFFFFF;
	FSINFO_sec.base_addr = 0xFFFFFFFF;
	FSINFO_sec.total_data_clusters = 0xFFFFFFFF;
	FSINFO_sec.total_data_sectors = 0xFFFFFFFF;
	FSINFO_sec.first_data_sector = 0xFFFFFFFF;
	fs_memset(FSINFO_sec.buf, 0x00, 512);
}

/*
int write_FAT_sector(u32 index)
{

}

int read_FAT_sector(u32 index)
{

}
*/

int main(void)
{
	init_MBR_DBR();

	system("pause");
	return 0;
}