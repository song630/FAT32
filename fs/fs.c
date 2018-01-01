#include "fs.h"
#include "fs_debug.h"

#include "fscache.c"
#include "util.c"

u8 short_name[12];  // used in find_file(), stores the name in short-dir format

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
	if (DBR_sec.attrs.bytes_per_sec != 512) {
		printf("inside init_MBR_DBR(): incorrect value of \"bytes_per_sec\".");
		return 1; // error
	}
	if (DBR_sec.attrs.secs_per_clus != 8) {
		printf("inside init_MBR_DBR(): incorrect value of \"secs_per_clus\".");
		return 1; // error
	}
	if (DBR_sec.attrs.num_FAT != 2) {
		printf("inside init_MBR_DBR(): incorrect value of \"num_FAT\".");
		return 1; // error
	}
	if (DBR_sec.attrs.dummy1 != 0 || DBR_sec.attrs.dummy2 != 0 || DBR_sec.attrs.dummy3 != 0) {
		printf("inside init_MBR_DBR(): incorrect values of \"dummy attrs\".");
		return 1; // error
	}
	if (DBR_sec.attrs.root_clus != 2) {
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
	for (i = 0; i <= LOCAL_BUF_NUM - 1; i++) {  // init the local buffer within
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
// index: fat_buf[index] is the block to be written back
// ===== to be tested ===== //
int write_FAT_sector(u8 index)
{
	int isDirty = 0;
	if (fat_buf[index].sec == 0xFFFFFFFF)
		return 0;  // already initialized, no need to write
	// this function will check dirty bit and set it when done
	#ifdef FS_DEBUG
	printf("\ninside write_FAT_sector(): will call write_512(), ");
	printf("may write sector %ld (RELATIVE) and sector %ld (RELATIVE).\n", 
		fat_buf[index].sec, fat_buf[index].sec + DBR_sec.attrs.secs_per_FAT);
	#endif

	if ((fat_buf[index].state & 0x01) == 0x01)  // dirty
		isDirty = 1;
	write_512(&fat_buf[index]);  // after that, dirty bit must be 0
	// since there is a copy of FAT, write twice:
	fat_buf[index].sec += DBR_sec.attrs.secs_per_FAT;
	if (isDirty)
		fat_buf[index].state |= 0x01;  // set dirty bit 1
	write_512(&fat_buf[index]);
	fat_buf[index].sec -= DBR_sec.attrs.secs_per_FAT;  // recover
	return 0;
}

// mainly calls "read_512()"
/* read a sector of FAT into fat_buf[]
 * index: related to base_addr
 * return: index of the block in buffer
 */
// NOTICE: as long as it is related to "read",
// always remember to set ref bit 1.
int read_FAT_sector(u32 index)
{
	#ifdef FS_DEBUG
	printf("\ninside read_FAT_sector(): will call read_512(), ");
	printf("read sector %ld into buffer.\n", index);
	#endif
	return read_512(fat_buf, index, &fat_clock_head, FAT_BUF_512_NUM);
}

/* parse the path, convert names to short-dir format.
 * path: must end with '\0'
 * cur: points to "path", index of last '/' + 1
 * short_name: save the result, its length == 11
 * return: 2 if reach the end;
 		   1 if failed;
		   0 if succeed.
 */
// ===== only absolute path accepted
// ===== tested ===== //
int convert_name_between_slash(u8 *path, u32 *cur, u8 *short_name)
{
	int count = 0;
	int i, j, k;
	u8 origin_name[13];  // forename: 8 bytes; ext: 3 bytes; '.': 1 byte; '\0': 1 byte
	#ifdef FS_DEBUG
	printf("inside convert_name_between_slash(): ");
	printf("path = %s, ", path);
	printf("cur = %ld\n", *cur);
	#endif
	if (*cur == 0) {  // the first time to parse
		if (path[*cur] != '/') {
			printf("inside convert_name_between_slash(): Error. not absolute path.\n");
			return 1;
		}
		else (*cur)++;
	}
	if (path[*cur] == '\0')  // reach the end
		return 2;
	while (path[*cur] != '/' && path[*cur] != '\0')  // traverse
	{
		if (count == 12) {
			printf("inside convert_name_between_slash(): Error. The name is too long.\n");
			return 1;
		}
		// can only have 11 chars plus a '.'
		// convert to upper case
		if (path[*cur] >= 'a' && path[*cur] <= 'z')
			origin_name[count++] = path[(*cur)++] - 'a' + 'A';
		else if (path[*cur] == '_' || path[*cur] == '.' ||  // '.' or '_'
			(path[*cur] >= '0' && path[*cur] <= '9') ||  // numbers
			(path[*cur] >= 'A' && path[*cur] <= 'Z'))  // already in upper case
			origin_name[count++] = path[(*cur)++];
	}  // save the name to origin_name[]
	origin_name[count] = '\0';  // marking the end
	#ifdef FS_DEBUG
	printf("origin_name = %s\n", origin_name);
	#endif

	(*cur)++;
	fs_memset(short_name, 0x20, 11);  // according to short-dir format, filling with ' '
	if (origin_name[0] == '.')
	{
		if (origin_name[1] == '\0') {  // "."
			printf("inside convert_name_between_slash(): Error. The file is \".\"\n");
			short_name[0] = '\0';  // marking that the name is invalid
			return 1;
		}
		else if (origin_name[1] == '.' && origin_name[2] == '\0') {  // ".."
			printf("inside convert_name_between_slash(): Error. The file is \"..\"\n");
			short_name[0] = '\0';  // marking that the name is invalid
			return 1;
		}
		i = 1;  // pass the first '.'
	}
	// valid name: then convert to short-dir format
	// forename: aligned to the left; ext: aligned to the right; remove '.'.
	// check if either of these is beyond the limit (3, 8)
	else  // does not start with '.'
	{
		i = 0;
		while (origin_name[i] != '.' && origin_name[i] != '\0')
			i++;
		// 2 cases: (1) reach a '.', running out of forename;
		//          (2) this name only has forename.
		if (i >= 9) {  // origin_name[9] == '.' or '\0'. forename is longer than 8
			printf("inside convert_name_between_slash(): Error. Forename is longer than 8.\n");
			return 1;  // fail
		}
		for (j = 0; j < i; j++)
			short_name[j] = origin_name[j];
		if (origin_name[i] == '\0')  // only has forename
			return 0;
		else
			i++;  // pass '.', entering ext name zone
		// ===== another case should be added: e.g. "foo."
	}

	// then traverse backwards, filling ext name:
	j = fs_strlen(origin_name) - 1;
	if (j - i + 1 > 3) {  // the ext is longer than 3
		printf("inside convert_name_between_slash(): Error. Ext is longer than 3.\n");
		return 1;  // fail
	}
	for (k = 10; j >= i; j--)  // copy ext name
		short_name[k--] = origin_name[j];
	short_name[11] = 0x00;  // end with '\0'
	return 0;
}

/* the "fs_FILE" is empty except for "path".
 * fill in the attrs of "fs_FILE" along the way finding the file
 */
int find_file(fs_FILE *f_ptr)
{
	if (f_ptr->path[0] != '/') {
		printf("inside find_file(): ERROR, invalid path.\n");
		return 1;
	}
	u32 cur_path = 0;  // points to "path"
	int path_cvt_rst;  // the result of calling convert_name_between_slash()
	int cur_clus_buf;  // data_buf[cur_clus_buf] stores the current data cluster
	u32 cur_clus_num = DBR_sec.attrs.root_clus;  // number of current cluster, init: 2.
	u8 name_temp[12];  // store filename to compare
	name_temp[11] = 0x00;  // end with '\0'
	u32 entry_offset;  // offset in a cluster
	int flag_clus_trans = 0;  // immediately jump to the cluster after find a file
	int dir_buf_index;  // used before get_file_start_clus()
	int i, j;
	// read in No.2 data cluster:
	cur_clus_buf = read_4k(f_ptr->data_buf, FSINFO_sec.first_data_sector, 
		&(f_ptr->clock_head), LOCAL_BUF_NUM);
	#ifdef FS_DEBUG
	printf("inside find_file(): read in No.2 cluster.\n");
	#endif
	while (1) {
		path_cvt_rst = convert_name_between_slash(f_ptr->path, &cur_path, short_name);
		if (path_cvt_rst == 2) {  // reach the end of path
			#ifdef FS_DEBUG
			printf("inside find_file(): reach the end of path.\n");
			#endif
			return 0;
		}
		if (path_cvt_rst == 1) {
			printf("inside find_file(): Error. when calling convert_name_between_slash().\n");
			return 1;
		}
		for (i = 0; i < DBR_sec.attrs.secs_per_clus; i++) {  // traverse every sectors in the cluster
			for (j = 0; j < DBR_sec.attrs.bytes_per_sec; j += 32) {  // traverse every dir-entries in one sector
				entry_offset = i * DBR_sec.attrs.bytes_per_sec + j;
				fs_memcpy(name_temp, f_ptr->data_buf[cur_clus_buf].buf + entry_offset, 11);  // get file name
				if (name_temp[0] == 0x00) {  // empty dir entry
					// no more dir entries, not found
					printf("inside find_file(): Error. not found.\n");
					return 1;
				}
				if (fs_strcmp(name_temp, short_name)) {  // found
					if (cur_path > fs_strlen(f_ptr->path)) {  // reach the end of path
						// record info in fs_FILE:
						fs_memcpy(f_ptr->entry.entry, f_ptr->data_buf[cur_clus_buf].buf + entry_offset, 32);  // copy raw data
						f_ptr->dir_entry_sector = sec_addr_from_clus(cur_clus_num) + i;
						f_ptr->dir_entry_pos = j;
						#ifdef FS_DEBUG
						printf("inside find_file(): found file, also reached end of path.\n");
						dump_fs_FILE(f_ptr);
						#endif
						return 0;
					}
					else {  // break and get the start cluster of the file or folder
						// first read in the sector containing dir-entry of the file
						dir_buf_index = read_512(dir_buf, sec_addr_from_clus(cur_clus_num) + i, 
							&dir_clock_head, DIR_BUF_512_NUM);
						cur_clus_num = get_file_start_clus(j, &dir_buf[dir_buf_index]);
						flag_clus_trans = 1;  // break 2 loops
						#ifdef FS_DEBUG
						printf("inside find_file(): a mid-way file found, jump to cluster %ld\n", cur_clus_num);
						#endif
						break;
					}
				}
			}  // inner for loop
			if (flag_clus_trans == 1) break;
		}  // outer for loop
		// read in next cluster: 2 cases.
		if (flag_clus_trans == 1)  // case1: after a file has been found
			cur_clus_buf = read_4k(f_ptr->data_buf, sec_addr_from_clus(cur_clus_num), 
				&(f_ptr->clock_head), LOCAL_BUF_NUM);
		else {  // case2: after all 8 sectors have been traversed
			cur_clus_num = FAT_entry_from_clus(cur_clus_num);  // get the next linked cluster of this file
			#ifdef FS_DEBUG
			printf("inside find_file(): get next cluster %ld from FAT.\n", cur_clus_num);
			#endif
			if (cur_clus_num == 0x0FFFFFFF) {  // marking the end
				printf("inside find_file(): Error. not found.\n");
					return 1;
			}
			cur_clus_buf = read_4k(f_ptr->data_buf, sec_addr_from_clus(cur_clus_num), 
				&(f_ptr->clock_head), LOCAL_BUF_NUM);
		}
		flag_clus_trans = 0;  // reset
	}
	return 0;
}

int open_file(fs_FILE *f_ptr, u8 *filename)
{
	init_FILE(f_ptr);  // clear the buffer in fs_FILE
	int len = fs_strlen(filename);
	fs_memcpy(f_ptr->path, filename, len);  // copy "filename" to "path[]"
	if (find_file(f_ptr)) {
		printf("inside open_file(): Error. Cannot find file.\n");
		return 1;
	}
	return 0;
}

// write FSINFO back (2 times), and write back the global buffers
int flush_all()
{
	int i;
	// first write FSINFO back: (twice)
	if (write_blocks(FSINFO_sec.buf, DBR_sec.attrs.FSINFO_sec, 1)) {
		// DBR_sec.attrs.FSINFO_sec == 1
		printf("inside flush_all(): Error. write_blocks(FSINFO) failed.\n");
		return 1;
	}
	if (write_blocks(FSINFO_sec.buf, DBR_sec.attrs.copy_DBR_SEC + 1, 1)) {
		// copy of FSINFO sector: sector 7
		printf("inside flush_all(): Error. write_blocks(FSINFO) failed.\n");
		return 1;
	}
	// second write fat_buf[], dir_buf[]:
	for (i = 0; i < FAT_BUF_512_NUM; i++)
		if (write_FAT_sector(i)) {
			printf("inside flush_all(): Error. write_blocks(fat_buf[%d]) failed.\n", i);
			return 1;
		}
	for (i = 0; i < DIR_BUF_512_NUM; i++)
		if (write_512(&dir_buf[i])) {
			printf("inside flush_all(): Error. write_blocks(dir_buf[%d]) failed.\n", i);
			return 1;
		}
	return 0;
}

int close_file(fs_FILE *f_ptr)
{
	// read the sector containing dir entry of the file in
	int buf_index = read_512(dir_buf, f_ptr->dir_entry_sector, &dir_clock_head, DIR_BUF_512_NUM);
	dir_buf[buf_index].state = 0x03;  // both ref bit and dirty bit set 1
	u32 file_size = 1024;  // "length" attr in dir-entry struct
	int i;
	u32 cur_clus = get_file_start_clus(f_ptr->dir_entry_pos, &dir_buf[buf_index]);  // get starting cluster
	// then traverse all clusters of the file and get file size: ===== this step can be removed
	// ===== later: add more attrs operations
	while(1) {
		cur_clus = FAT_entry_from_clus(cur_clus);
		if (cur_clus == 0x0FFFFFFF) break;  // reaching end
		else file_size += 1024;  // add by 1K
	}
	// then update the dir-entry of the file:
	fs_memcpy(f_ptr->entry.entry + 28, &file_size, 4);  // write offset 28, 29, 30, 31, which is "length"
	fs_memcpy(dir_buf[buf_index].buf + f_ptr->dir_entry_pos, f_ptr->entry.entry, 32);  // write back
	// at last clear the buffers:
	if (flush_all() == 1) {
		printf("inside close_file(): Error. when calling flush_all().\n");
		return 1;
	}
	for (i = 0; i < LOCAL_BUF_NUM; i++)
		write_4k(f_ptr->data_buf[i]);
	return 0;
}

int read_file(fs_FILE *f_ptr, u8 *buf, u32 count)
{
	u32 size = f_ptr->entry.attrs.length;  // get file size (in bytes)
	int buf_index = read_512(dir_buf, f_ptr->dir_entry_sector, &dir_clock_head, DIR_BUF_512_NUM);
	u32 cur_clus = get_file_start_clus(f_ptr->dir_entry_pos, &dir_buf[buf_index]);  // get starting cluster
	u32 buf_cursor = 0;  // a cursor to write "buf"
	u32 clus_size = DBR_sec.attrs.secs_per_clus * DBR_sec.attrs.bytes_per_sec;  // 512 bytes * 8 = 4096 bytes
	// first deal with parameter "count":
	if (count <= 0) return 0;
	if (f_ptr->loc + count >= size)
		count = size - f_ptr->loc - 1;
	// get virtual starting cluster & byte and ending cluster & byte: assuming base is 0:
	u32 v_start_clus = f_ptr->loc / clus_size;
	u32 v_start_byte = f_ptr->loc % clus_size; // within a clus
	u32 v_end_clus = (f_ptr->loc + count) / clus_size;
	u32 v_end_byte = (f_ptr->loc + count) % clus_size;
	#ifdef FS_DEBUG
	printf("inside read_file(): 4 temp variables:\n");
	printf("\tv_start_clus: %ld, v_start_byte: %ld,\n", v_start_clus, v_start_byte);
	printf("\tv_end_clus: %ld, v_end_byte: %ld.\n", v_end_clus, v_end_byte);
	#endif
	int counter = 0;
	// get real starting cluster to be read:
	while (counter < v_start_clus) {
		cur_clus = FAT_entry_from_clus(cur_clus);  // get next linked cluster via FAT
		if (cur_clus == 0x0FFFFFFF) {  // end, not possible
			printf("inside read_file(): Error. cannot find the starting cluster to be read.\n");
			return 1;
		}
		counter++;
	}
	// begin to copy data into "buf"
	if (v_start_clus == v_end_clus) {  // within one cluster
		#ifdef FS_DEBUG
		printf("inside read_file(): contents to be read within one cluster.\n");
		#endif
		buf_index = read_4k(f_ptr->data_buf, sec_addr_from_clus(cur_clus), &f_ptr->clock_head, LOCAL_BUF_NUM);
		fs_memcpy(buf, f_ptr->data_buf[buf_index].buf + v_start_byte, count);
		return 0;
	}
	while (counter <= v_end_clus) {  // special: first cluster and last one
		// read in the next cluster into buffer
		buf_index = read_4k(f_ptr->data_buf, sec_addr_from_clus(cur_clus), &f_ptr->clock_head, LOCAL_BUF_NUM);
		if (counter == v_start_clus) {
			count = clus_size - v_start_byte;  // decrease "count"
			// from "v_start_byte" and get the rest bytes of this cluster
			fs_memcpy(buf, f_ptr->data_buf[buf_index].buf + v_start_byte, count);
			buf_cursor += count;  // update cursor
		}
		else if (counter == v_end_clus) {
			fs_memcpy(buf + buf_cursor, f_ptr->data_buf[buf_index].buf, v_end_byte);
			break;
		}
		else {  // in between
			fs_memcpy(buf + buf_cursor, f_ptr->data_buf[buf_index].buf, clus_size);
			buf_cursor += clus_size;  // update cursor
		}
		cur_clus = FAT_entry_from_clus(cur_clus);  // get next linked cluster via FAT
		counter++;
	}

	return 0;
}

int find_free_clus(u32 start, u32 *free_clus)
{
	
}

/*
how to use winhex:
e.g. get the offset of DBR:
base_addr (128) * 512 = 65536 = 10000 (hex). (ABSOLUTE)
=====
the first 8 bytes of FAT is always F8FFFF0F.
=====
e.g. 1st sector in FAT:
DBR_sec.attrs.reserved_secs = 6170, 6170 * 512 = 303400 (hex). (RELATIVE)
=====
128 + reserved_secs = 6298, 6298 * 512 = 313400 (hex) (ABSOLUTE)
313400, 313410 already have data.
=====
2nd sector of FAT:
6299 * 512 = 313600 (hex) (ABSOLUTE)
*/

int main(void)
{
	/*
	// ============ test "init_MBR_DBR()" ============ //
	// init_MBR_DBR();
	*/

	
	// ===== test "convert_name_between_slash()" ===== //
	u8 short_name[12];
	short_name[11] = '\0';
	u8 path[256] = "/ADIR/abC_k.bs/de1.out";
	u32 cur = 0;
	int rst = convert_name_between_slash(path, &cur, short_name);
	printf("original file name: %s\n", path);
	printf("short_name: %s[end]\n, rst = %d\n", short_name, rst);
	rst = convert_name_between_slash(path, &cur, short_name);
	printf("short_name: %s[end]\n, rst = %d\n", short_name, rst);
	rst = convert_name_between_slash(path, &cur, short_name);
	printf("short_name: %s[end]\n, rst = %d\n", short_name, rst);
	rst = convert_name_between_slash(path, &cur, short_name);
	printf("short_name: %s[end]\n, rst = %d\n", short_name, rst);
	printf("cur = %ld\n", cur);

	/*
	// ========== test "write_FAT_sector()" ========== //
	init_MBR_DBR();
	init_dir_buf();
	init_fat_buf();
	fat_buf[0].state |= 0x01;  // dirty
	fat_buf[0].sec = 6171;  // 2st sector of FAT, RELATIVE
	u8 content[50] = "test write_FAT_sector() here";
	fs_memcpy(fat_buf[0].buf, content, 50);
	int rst = write_FAT_sector(0);  // fat_buf[0]
	printf("result: %d\n", rst);
	printf("state: %02X\n", fat_buf[0].state);
	*/
	system("pause");
	return 0;
}
