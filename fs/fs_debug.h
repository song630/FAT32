#ifndef _FS_DEBUG_H_
#define _FS_DEBUG_H_

#include <stdio.h>  // printf()
#include "fscache.h"  // BUF_4K, BUF_512

void dump_buf_512_states(BUF_512 *buf, u8 buf_size)
{
	int i;
	printf("buf 512 states:");
	for (i = 0; i <= buf_size - 1; i++)
		printf(" %02X", buf[i].state);
	printf("\n");
}

void dump_buf_4k_states(BUF_4K *buf, u8 buf_size)
{
	int i;
	printf("buf 4k states:");
	for (i = 0; i <= buf_size - 1; i++)
		printf(" %02X", buf[i].state);
	printf("\n");
}

void dump_block_512(u8 *block)
{
	int i;
	printf("Dump a block (512):\n");
	for (i = 0; i <= 511; i++)
	{
		printf("%02X ", block[i]);
		if (i % 16 == 15)
			printf("\n");
	}
}

void dump_block_4k(u8 *block)
{
	int i;
	printf("Dump a block (4k):\n");
	for (i = 0; i <= 512 * 8 - 1; i++)
	{
		printf("%02X ", block[i]);
		if (i % 16 == 15)
			printf("\n");
	}
}

#endif
