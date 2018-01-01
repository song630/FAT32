#include "fs.h"  // SD_PATH
#include "fscache.h"
#include "fs_debug.h"

/* buf: buffer (4 blocks)
 * clock_head: currently pointes to which block (the addr passed)
 * buf_size: in case of overflow
 * return: the index of block
 */
// ===== tested ===== //
u8 victim_4k(BUF_4K *buf, u8 *clock_head, u8 buf_size)
{
	int index;
	// notice: only use clock_head to refer to the selected block is not enough,
	// since it may be changed if reaching the end, so introduce another "index".
	for (; *clock_head <= buf_size - 1; (*clock_head)++)
	{  // the 1st sweep
		if ((buf[*clock_head].state & 0x02) == 0)  // ref bit == 0
		{
			if ((buf[*clock_head].state & 0x01) == 0x00)  // dirty bit == 0
			{
				#ifdef FS_DEBUG
				printf("inside victim_4k(): found buf[%d] in 1st sweep.\n", *clock_head);
				#endif
				index = *clock_head;
				if (*clock_head + 1 == buf_size)  // reach the last block
					*clock_head = 0;  // return to the head of the queue
				return index;  // found
			}
		}
		else
			buf[*clock_head].state &= 0x01;  // set ref bit 0
	}
	for (*clock_head = 0; *clock_head <= buf_size - 1; (*clock_head)++)
	{  // the 2nd sweep
		if ((buf[*clock_head].state & 0x02) == 0x02)  // ref bit != 0
			continue;
		else if ((buf[*clock_head].state & 0x01) == 0) // ref bit == 0, dirty bit == 0
		{
			#ifdef FS_DEBUG
			printf("inside victim_4k(): found buf[%d] in 2st sweep.\n", *clock_head);
			dump_buf_4k_states(buf, buf_size); // print all "state" of buf
			#endif
			return index = *clock_head;  // found
		}
	}
	// not found, reaching the end during the 2nd sweep
	#ifdef FS_DEBUG
	printf("inside victim_4k(): not found in 2 sweeps.\n");
	dump_buf_4k_states(buf, buf_size); // print all "state" of buf
	#endif
	*clock_head = 0;
	return index = buf_size - 1;  // just return the last block (its ref bit must == 0)
}

/* block: one block in buffer
 * FSINFO_sec.base_addr: the absolute DBR sector index
 * block.sec: the absolute sector index (related to base_addr)
 * base_addr + block.sec: the absolute dst sector index
 * use info of the global buffer: "FSINFO_sec", "DBR_sec"
 */
void write_4k(BUF_4K *block)
{
	unsigned int start, end;
	if ((block->state & 0x01) == 0)  // dirty bit == 0, do nothing
		return;  // success
	else  // write
	{
		write_blocks(block->buf, FSINFO_sec.base_addr + block->sec, DBR_sec.attrs.secs_per_clus);
		#ifdef FS_DEBUG
		start = FSINFO_sec.base_addr + block->sec;
		end = FSINFO_sec.base_addr + block->sec + DBR_sec.attrs.secs_per_clus - 1;
		printf("inside write_4k(): ");
		printf("sector %d (ABSOLUTE, offset is %X) to ", start, start * 512);
		printf("sector %d (ABSOLUTE, offset is %X) of FS modified.\n", end, end * 512);
		#endif
		block->state &= 0x02;  // set dirty bit 0
	}
}

/* buf: the buffer blocks
 * start_sec: the absolute first src sector index (related to base_addr)
 * clock_head: to find a block in buffer
 * return: index of the block which has the content read in
 */
int read_4k(BUF_4K *buf, u32 start_sec, u8 *clock_head, u8 buf_size)
{
	int i;
	int vic;  // the victim block
	for (i = 0; i <= buf_size - 1; i++)
		if (buf[i].sec == start_sec)  // already in buffer
		{
			#ifdef FS_DEBUG
			printf("inside read_4k(): ");
			printf("the block is already in buf[%d].\n", i);
			#endif
			buf[i].state |= 0x02;  // set ref bit 1
			return i;
		}
	// otherwise, find a victim
	vic = victim_4k(buf, clock_head, buf_size);  // may be dirty
	#ifdef FS_DEBUG
	printf("inside read_4k(): called victim_4k(), ");
	printf("the content is put in buf[%d].\n", vic);
	#endif
	write_4k(&buf[vic]);  // the dirty bit is set 0 here
	read_blocks(buf[vic].buf, FSINFO_sec.base_addr + start_sec, DBR_sec.attrs.secs_per_clus);
	buf[vic].state |= 0x02;  // the ref bit is set 1 here
	buf[vic].sec = start_sec;
	return vic;
}

// ===== tested ===== //
u8 victim_512(BUF_512 *buf, u8 *clock_head, u8 buf_size)
{
	int index;
	for (; *clock_head <= buf_size - 1; (*clock_head)++)
	{  // the 1st sweep
		if ((buf[*clock_head].state & 0x02) == 0)  // ref bit == 0
		{
			if ((buf[*clock_head].state & 0x01) == 0x00)  // dirty bit == 0
			{
				#ifdef FS_DEBUG
				printf("inside victim_512(): found buf[%d] in 1st sweep.\n", *clock_head);
				#endif
				index = *clock_head;
				if (*clock_head + 1 == buf_size)  // reach the last block
					*clock_head = 0;  // return to the head of the queue
				return index;  // found
			}
		}
		else
			buf[*clock_head].state &= 0x01;  // set ref bit 0
	}
	for (*clock_head = 0; *clock_head <= buf_size - 1; (*clock_head)++)
	{  // the 2nd sweep
		if ((buf[*clock_head].state & 0x02) == 0x02)  // ref bit != 0
			continue;
		else if ((buf[*clock_head].state & 0x01) == 0) // ref bit == 0, dirty bit == 0
		{
			#ifdef FS_DEBUG
			printf("inside victim_512(): found buf[%d] in 2st sweep.\n", *clock_head);
			dump_buf_512_states(buf, buf_size); // print all "state" of buf
			#endif
			return index = *clock_head;  // found
		}
	}
	// not found, reaching the end during the 2nd sweep
	#ifdef FS_DEBUG
	printf("inside victim_512(): not found in 2 sweeps.\n");
	dump_buf_512_states(buf, buf_size); // print all "state" of buf
	#endif
	*clock_head = 0;
	return index = buf_size - 1;  // just return the last block (its ref bit must == 0)
}

void write_512(BUF_512 *block)
{
	unsigned int addr;
	if ((block->state & 0x01) == 0)  // dirty bit == 0, do nothing
		return;  // success
	else  // write
	{
		write_blocks(block->buf, FSINFO_sec.base_addr + block->sec, 1);
		#ifdef FS_DEBUG
		addr = FSINFO_sec.base_addr + block->sec;
		printf("inside write_512(): ");
		printf("sector %d (ABSOLUTE, offset is %X) of FS modified.\n", addr, 512 * addr);
		#endif
		block->state &= 0x02;  // set dirty bit 0
	}
}

// sec: related to base_addr
// ===== tested ===== //
int read_512(BUF_512 *buf, u32 sec, u8 *clock_head, u8 buf_size)
{
	int i;
	int vic;  // the victim block
	for (i = 0; i <= buf_size - 1; i++)
		if (buf[i].sec == sec)  // already in buffer
		{
			#ifdef FS_DEBUG
			printf("inside read_512(): ");
			printf("the block is already in buf[%d].\n", i);
			#endif
			buf[i].state |= 0x02;  // set ref bit 1
			return i;
		}
	// otherwise, find a victim
	vic = victim_512(buf, clock_head, buf_size);  // may be dirty
	#ifdef FS_DEBUG
	printf("inside read_512(): called victim_512(), ");
	printf("the content is put in buf[%d].\n", vic);
	#endif
	write_512(&buf[vic]);  // will check the dirty bit, ad set it 0 afterwards
	read_blocks(buf[vic].buf, FSINFO_sec.base_addr + sec, 1);
	buf[vic].state |= 0x02;  // the ref bit is set 1 here
	buf[vic].sec = sec;
	return vic;
}

/* buf: write the contents starting from "buf"
 * dst_sec: write to the dst starting sector of SD (related to 0)
 * num_blocks: write these blocks
 *
 * notice: this function does not modify "state"
 */
int write_blocks(u8 *buf, u32 dst_sec, u8 num_blocks)
{
	int i;
	FILE *f_ptr;
	/*
	if (dst_sec + num_blocks - 1 > DBR_sec.attrs.total_secs)  // overflow
	{
		printf("inside write_blocks(): overflow.\n");
		return 1;  // fail
	}
	*/
	if ((f_ptr = fopen(SD_PATH, "rb+")) == NULL)
	{
		printf("inside write_blocks(): Fail to open file.\n");
		return 1;  // fail
	}
	for (i = 0; i <= num_blocks - 1; i++)
	{  // SEEK_SET: from the beginning of file
		// fseek(): the file pointer points to the sector to be written
		fseek(f_ptr, (dst_sec + i) * 512/*DBR_sec.attrs.bytes_per_sec*/, SEEK_SET);
		fwrite(buf + i * 512/*DBR_sec.attrs.bytes_per_sec*/, sizeof(u8), 
			512/*DBR_sec.attrs.bytes_per_sec*/, f_ptr);
	}
	fclose(f_ptr);
	return 0;
}

/* buf: read to it
 * src_sec: read the contents starting from "src_sec"
 * 
 * notice: this function does not modify "state"
 */
// ===== tested ===== //
int read_blocks(u8 *buf, u32 src_sec, u8 num_blocks)
{
	int i;
	FILE *f_ptr;
	/*
	if (src_sec + num_blocks - 1 > DBR_sec.attrs.total_secs)  // overflow
	{
		printf("inside read_blocks(): overflow.\n");
		return 1;  // fail
	}
	*/
	if ((f_ptr = fopen(SD_PATH, "rb")) == NULL)
	{
		printf("inside write_blocks(): Fail to open file.\n");
		return 1;  // fail
	}
	for (i = 0; i <= num_blocks - 1; i++)
	{
		fseek(f_ptr, (src_sec + i) * 512/* DBR_sec.attrs.bytes_per_sec */, SEEK_SET);
		fread(buf + i * 512/* DBR_sec.attrs.bytes_per_sec*/, sizeof(u8), 
			512/*DBR_sec.attrs.bytes_per_sec*/, f_ptr);
	}
	fclose(f_ptr);
	return 0;
}
