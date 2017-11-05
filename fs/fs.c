#include "fs.h"
#include "fscache.h"
#include "fs_debug.h"
#include "util.h"

// init global buffers: fat_buf[], dir_buf[]
int init_MBR_DBR()
{
	int i;
	for (i = 0; i <= 3; i++)
	{
		fat_buf[i].state &= 0x00;
		fat_buf[i].sec = 0xFFFFFFFF;  // an invalid value
		memset(fat_buf[i].buf, 0x00, 512);

		dir_buf[i].state &= 0x00;
		dir_buf[i].sec = 0xFFFFFFFF;  // an invalid value
		memset(dir_buf[i].buf, 0x00, 512);
	}
}
