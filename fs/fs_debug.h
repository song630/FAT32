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
	for (i = 0; i <= 511; i++) {
		printf("%02X ", block[i]);
		if (i % 16 == 15)
			printf("\n");
	}
}

void dump_block_4k(u8 *block)
{
	int i;
	printf("Dump a block (4k):\n");
	for (i = 0; i <= 512 * 8 - 1; i++) {
		printf("%02X ", block[i]);
		if (i % 16 == 15)
			printf("\n");
	}
}

extern DBR_SEC DBR_sec;

void dump_DBR_info()
{
	printf("\ninside init_MBR_DBR(): values of some attrs in DBR:\n");
	printf("\tedition: %s\n", DBR_sec.attrs.edition);
	printf("\tbytes_per_sec: %d\n", DBR_sec.attrs.bytes_per_sec);
	printf("\tsecs_per_clus: %d\n", DBR_sec.attrs.secs_per_clus);
	printf("\treserved_secs: %d\n", DBR_sec.attrs.reserved_secs);
	printf("\tnum_FAT: %d\n", DBR_sec.attrs.num_FAT);
	printf("\thidden_secs: %ld\n", DBR_sec.attrs.hidden_secs);
	printf("\ttotal_secs: %ld\n", DBR_sec.attrs.total_secs);
	printf("\tsecs_per_FAT: %ld\n", DBR_sec.attrs.secs_per_FAT);
	printf("\troot_clus: %ld\n", DBR_sec.attrs.root_clus);
	printf("\tFSINFO_sec: %d\n", DBR_sec.attrs.FSINFO_sec);
	printf("\tcopy_DBR_SEC: %d\n", DBR_sec.attrs.copy_DBR_SEC);
}

extern FSINFO_SEC FSINFO_sec;

void dump_FSINFO_info()
{
	printf("\ninside init_MBR_DBR(): values of some attrs in FSINFO:\n");
	printf("\tempty_clusters: %ld\n", FSINFO_sec.empty_clusters);
	printf("\tnext_avil_clus: %ld\n", FSINFO_sec.next_avil_clus);
	printf("\tbase_addr: %ld\n", FSINFO_sec.base_addr);
	printf("\ttotal_data_clusters: %ld\n", FSINFO_sec.total_data_clusters);
	printf("\ttotal_data_sectors: %ld\n", FSINFO_sec.total_data_sectors);
	printf("\tfirst_data_sector: %ld\n", FSINFO_sec.first_data_sector);
}

void dump_fs_FILE(fs_FILE *f_ptr)
{
	int i;
	printf("\tdump fs_FILE:\n");
	printf("\tpath: %s\n", f_ptr->path);
	printf("\tloc: %ld\n", f_ptr->loc);
	printf("\tdir_entry_pos = %ld\n", f_ptr->dir_entry_pos);
	printf("\tdir_entry_sector = %ld\n", f_ptr->dir_entry_sector);
	printf("\tentry[32]: \n\t");
	for (i = 0; i < 32; i++) {
		printf("%02X ", f_ptr->entry.entry[i]);
		if (i % 16 == 15)
			printf("\n\t");
	}
}

#endif
