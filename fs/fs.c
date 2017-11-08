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
	printf("\ninside init_MBR_DBR(): MBR read in. The contents of MBR: \n");
	dump_block_512(MBR_buf);  // print the sector MBR
	#endif

	// 2. then get DBR and FSINFO from info in MBR:
	init_FSINFO();
	FSINFO_sec.base_addr = get_u32(MBR_buf + 446 + 8);  // the sector index of DBR
	// DBR is 0 sector; FSINFO is 1 sector, so "base_addr + 1".
	// but the data should not be put in global buffer, so cannot call read_512().
	read_blocks(FSINFO_sec.buf, FSINFO_sec.base_addr + 1, 1);
	#ifdef FS_DEBUG
	printf("\ninside init_MBR_DBR(): base_addr = %ld.\n", FSINFO_sec.base_addr);
	printf("inside init_MBR_DBR(): FSINFO read in. The contents of FSINFO: \n");
	dump_block_512(FSINFO_sec.buf);  // print the sector MBR
	#endif
	read_blocks(DBR_sec.buf, FSINFO_sec.base_addr, 1);  // read in DBR sector
	#ifdef FS_DEBUG
	printf("\ninside init_MBR_DBR(): DBR read in. The contents of DBR: \n");
	dump_block_512(DBR_sec.buf);  // print the sector MBR
	#endif

	// 3. set some attrs, and check if the values of some attrs are correct.
	// ===== 2 problems: ===== //
	// (1) sth wrong with alignment <- solved by "#pragma pack (1)"
	// (2) in read_blocks(): DBR_sec.attrs.bytes_per_sec, DBR_sec.attrs.total_secs
	//    have not been assigned.

	// check the values:
	if (DBR_sec.attrs.bytes_per_sec != 512)
	{
		printf("inside init_MBR_DBR(): incorrect value of \"bytes_per_sec\".");
		return 1; // error
	}
	if (DBR_sec.attrs.secs_per_clus != 8)
	{
		printf("inside init_MBR_DBR(): incorrect value of \"secs_per_clus\".");
		return 1; // error
	}
	if (DBR_sec.attrs.num_FAT != 2)
	{
		printf("inside init_MBR_DBR(): incorrect value of \"num_FAT\".");
		return 1; // error
	}
	if (DBR_sec.attrs.dummy1 != 0 || DBR_sec.attrs.dummy2 != 0 || DBR_sec.attrs.dummy3 != 0)
	{
		printf("inside init_MBR_DBR(): incorrect values of \"dummy attrs\".");
		return 1; // error
	}
	if (DBR_sec.attrs.root_clus != 2)
	{
		printf("inside init_MBR_DBR(): incorrect value of \"root_clus\".");
		return 1; // error
	}
	// get values of attrs of "FSINFO_sec":
	FSINFO_sec.empty_clusters = get_u32(FSINFO_sec.buf + 488);
	FSINFO_sec.next_avil_clus = get_u32(FSINFO_sec.buf + 492);
	FSINFO_sec.total_data_sectors = DBR_sec.attrs.total_secs - DBR_sec.attrs.reserved_secs 
									- 2 * DBR_sec.attrs.secs_per_FAT;
	FSINFO_sec.total_data_clusters = FSINFO_sec.total_data_sectors / DBR_sec.attrs.secs_per_clus;
	// first data sector: related to base_addr
	FSINFO_sec.first_data_sector = DBR_sec.attrs.reserved_secs + DBR_sec.attrs.secs_per_FAT * 2;
	#ifdef FS_DEBUG
	dump_DBR_info();  // print values of important attrs in DBR
	dump_FSINFO_info();
	#endif

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

// mainly calls "write_512()"
/* write a sector of FAT table
 * index: which buffer block
 * return: 1 if failed
 */
// when it is called: after a buffer block has been modified,
// it should be written back
// ===== to be tested ===== //
int write_FAT_sector(u8 index)
{
	int isDirty = 0;
	if (fat_buf[index].sec == 0xFFFFFFFF)
		return 0;  // already initialized, no need to write
	// this function will check dirty bit and set it when done
	#ifdef FS_DEBUG
	printf("\ninside write_FAT_sector(): will call write_512(), ");
	printf("may write sector %ld and sector %ld.\n", 
		fat_buf[index].sec, fat_buf[index].sec + DBR_sec.attrs.secs_per_FAT);
	#endif

	if (fat_buf[index].state & 0x01 == 0x01)  // dirty
		isDirty = 1;
	write_512(&fat_buf[index]);  // after that, dirty bit must be 0
	// since there is a copy of FAT, write twice
	fat_buf[index].sec += DBR_sec.attrs.secs_per_FAT;
	if (isDirty)
		fat_buf[index].state |= 0x01;  // set dirty bit 1
	write_512(&fat_buf[index]);
	fat_buf[index].sec -= DBR_sec.attrs.secs_per_FAT;  // recover
}

// mainly calls "read_512()"
/* read a sector of FAT into fat_buf[]
 * index: related to base_addr
 * return: index of the block in buffer
 */
// NOTICE: as long as it is related to "read",
// always set ref bit 1.
int read_FAT_sector(u32 index)
{
	#ifdef FS_DEBUG
	printf("\ninside read_FAT_sector(): will call read_512(), ");
	printf("read sector %ld into buffer.\n", index);
	#endif
	return read_512(fat_buf, index, &fat_clock_head, FAT_BUF_512_NUM);
}

/* parse the path, convert names to short-dir format.
 * path:must end with '\0'
 * cur: points to "path", index of last '/' + 1
 * short_name: save the result, its length == 11
 * return: 0 if fail or reach the end
 */
// ===== to be tested ===== //
int convert_name_between_slash(u8 *path, u32 *cur, u8 *short_name)
{
	int count = 0;
	int i, j, k;
	char origin_name[13];  // forename: 8 bytes; ext: 3 bytes; '.': 1 byte; '\0': 1 byte
	#ifdef FS_DEBUG
	printf("inside convert_name_between_slash(): ");
	printf("path = %s, ", path);
	printf("cur = %d\n", *cur);
	#endif
	if (path[cur] == '\0')  // reach the end
		return 1;
	while (path[*cur] != '/' && path[*cur] != '\0')  // traverse
	{
		if (count == 12)
		{
			printf("inside convert_name_between_slash(): Error. The name is too long.\n");
			return 1;
		}
		// can only have 11 chars plus a '.'
		// convert to upper case
		origin_name[count++] = path[(*cur++)] - 'a' + 'A';
	}  // save the name to origin_name[]
	origin_name[count] = '\0';  // marking the end

	fs_memset(short_name, 0x20, 11);  // according to short-dir format, filling with ' '
	if (origin_name[0] == '.')
	{
		if (origin_name[1] == '\0')  // "."
		{
			printf("inside convert_name_between_slash(): Error. The file is \".\"\n");
			short_name[0] = '\0';  // marking that the name is invalid
			return 1;
		}
		else if (origin_name[1] == '.' && origin_name[2] == '\0')  // ".."
		{
			printf("inside convert_name_between_slash(): Error. The file is \"..\"\n");
			short_name[0] = '\0';  // marking that the name is invalid
			return 1;
		}
	}
	// valid name: then convert to short-dir format
	// forename: aligned to the left; ext: aligned to the right; remove '.'.
	// check if either of these is beyond the limit (3, 8)
	i = 0;
	while (origin_name[i] != '.' && origin_name[i] != '\0')
		i++;
	// 2 cases: (1) reach a '.', running out of forename;
	//          (2) this name only has forename.
	if (i >= 9)  // origin_name[9] == '.' or '\0'. forename is longer than 8
	{
		printf("inside convert_name_between_slash(): Error. Forename is longer than 8.\n");
		return 1;  // fail
	}
	if (origin_name[i] == '\0')  // only has forename
		return 0;
	else
		i++;  // pass '.', entering ext name zone
	// ===== another case should be added: e.g. "foo."

	// then traverse backwards, filling ext name:
	j = fs_strlen(origin_name) - 1;
	if (j - i + 1 > 3)  // the ext is longer than 3
	{
		printf("inside convert_name_between_slash(): Error. Ext is longer than 3.\n");
		return 1;  // fail
	}
	for (k = 10; j >= i; j--)  // copy ext name
		short_name[k--] = origin_name[j];
	return 0;
}

int find_file(fs_FILE *f_ptr)
{

}

int main(void)
{
	init_MBR_DBR();

	system("pause");
	return 0;
}
