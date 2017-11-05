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

void dump_block_512(BUF_512 *block)
{
	int i;
	printf("Dump a block (512): ");
	printf("block->sec = %ld, block->buf[] =\n", block->sec);
	for (i = 0; i <= 511; i++)
	{
		printf("%02X ", block->buf[i]);
		if (i % 16 == 15)
			printf("\n");
	}
}

void dump_block_4k(BUF_4K *block)
{
	int i, j;
	printf("Dump a block (4k): ");
	printf("block->sec = %ld, block->buf[] =\n", block->sec);
	for (i = 0; i <= 7; i++)
	{
		for (j = 0; j <= 511; j++)
		{
			printf("%02X ", block->buf[j]);
			if (i % 16 == 15)
				printf("\n");
		}
		printf("\n");
	}
}

#endif
