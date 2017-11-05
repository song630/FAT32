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

void fs_memset(void *buf, u8 c, u32 count)
{
	int i;
	u8 *ptr = (u8 *)buf;
	for (i = 0; i <= count - 1; i++)
		*(ptr + i) = c;
}

void fs_memcpy(void *dst, void *src, u32 count)
{
	int i;
	u8 *dst_ptr = (u8 *)dst;
	u8 *src_ptr = (u8 *)src;
	for (i = 0; i <= count - 1; i++)
		*(dst_ptr + i) = *(src_ptr + i);
}

typedef struct A_struct {
	u8 a1;
	u8 a2;
	u8 a3[3];
	u16 a4;
	u32 a5;
	u8 a6;
} A_attr;

typedef union A_union {
	A_attr attr;
	u8 buf[12];
} A;

int main(void)
{
	// read_a_sector(0);  // MBR
	// printf("\n\n");
	// start by 0xEB58: line 4096 and line 4288
	// read_a_sector(446 + 8);
	// printf("\n\n");
	// read_a_sector(512 * 128);
	A obj;
	obj.buf[0] = 0x05;  // a1
	obj.buf[1] = 0x04;  // a2
	obj.buf[2] = 0x03;  // a3
	obj.buf[3] = 0x02;
	obj.buf[4] = 0x01;
	obj.buf[5] = 0xA0;  // a4
	obj.buf[6] = 0x5B;
	obj.buf[7] = 0x08;  // a5
	obj.buf[8] = 0x15;
	obj.buf[9] = 0xA1;
	obj.buf[10] = 0x04;
	obj.buf[11] = 0x10;  // a6
	u32 a = 77665544;  // 0x04A11508
	u32 b = 23456;  // 0x5BA0
	printf("%d\n", obj.attr.a4 == 23456);

	system("pause");
	return 0;
}