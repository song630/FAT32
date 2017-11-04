#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

int fs_strlen(u8 *str)
{
	int i = 0;
	while (str[i++] != '\0');
	return --i;
}

int fs_strcmp(u8 *str1, u8 *str2)
{
	int i;
	int l1 = fs_strlen(str1), l2 = fs_strlen(str2);
	if (l1 != l2)
		return 0;  // not equal
	for (i = 0; i <= l1 && str1[i] == str2[i]; i++);  // of the same length
	if (i - 1 == l1)  // reach the end
		return 1;
	else
		return 0;
}

int main(void)
{
	// read_a_sector(0);  // MBR
	// printf("\n\n");
	// start by 0xEB58: line 4096 and line 4288
	// read_a_sector(446 + 8);
	// printf("\n\n");
	// read_a_sector(512 * 128);
	u8 s1[6] = "";
	u8 s2[6] = "";
	printf("%d\n", fs_strcmp(s1, s2));

	system("pause");
	return 0;
}