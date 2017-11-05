#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define FS_DEBUG

void read_a_sector(int base_addr)
{
	char *path = "D://virtual_disk.vhd";
	FILE *fp;
	if((fp = fopen(path, "rb")) == NULL)
	{
		printf("failed.\n");
		exit(1);
	}
	else
	{
		int read_lines = 35;  // MBR sector
		char read_buf[17];  // the last char is reserved for '\0'
		fseek(fp, base_addr, 0);
		while(read_lines > 0 && !feof(fp))
		{
			read_lines--;
			fgets(read_buf, 17, fp);  // ===== a line is much more than 16 bytes
			int i;
			for(i = 0; i <= 15; i++)
			{
				if(read_buf[i] == '\n')
					printf("e");
				printf("%02X ", (unsigned char)read_buf[i]);
			}
			// w/o (unsigned char), it will print out a string starting with ffff
			printf("\n");
		}
	}
	fclose(fp);
}

typedef unsigned char u8;  // 1 byte
typedef unsigned short u16;  // 2 bytes
typedef unsigned long u32;  // 4 bytes

u8 fat_clock_head = 0;

typedef struct buf_4k {  // 8 sectors per cluster
	u32 sec;  // the num of sector stored in this buffer
	u8 state;
	u8 buf[4096];
} BUF_4K;

BUF_4K buf_zone[4];

void dump_buf_4k_states(BUF_4K *buf, u8 buf_size)
{
	int i;
	printf("buf 4k states:");
	for (i = 0; i <= buf_size - 1; i++)
		printf(" %02X", buf[i].state);
	printf("\n");
}

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

int main(void)
{
	// read_a_sector(0);  // MBR
	// printf("\n\n");
	// start by 0xEB58: line 4096 and line 4288
	// read_a_sector(446 + 8);
	// printf("\n\n");
	// read_a_sector(512 * 128);
	buf_zone[0].state = 0x01;
	buf_zone[1].state = 0x02;
	buf_zone[2].state = 0x01;
	buf_zone[3].state = 0x03;
	int index = victim_4k(buf_zone, &fat_clock_head, 4);
	printf("index = %d\n", index);

	system("pause");
	return 0;
}