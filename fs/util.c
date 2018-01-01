#include "util.h"

u16 get_u16(u8 *addr)
{
	return (*(addr + 1) << 8) + *addr;
}

u32 get_u32(u8 *addr)
{
	return (*(addr + 3) << 24) + (*(addr + 2) << 16) + (*(addr + 1) << 8) + *addr;
}

// below are functions defined for string operations

// ===== tested ===== //
int fs_strlen(u8 *str)
{
	int i = 0;
	while (str[i++] != '\0');
	return --i;
}

// ===== tested ===== //
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

void fs_strcpy(u8 *dst_str, u8 *src_str)
{
	int i;
	int l1 = fs_strlen(dst_str), l2 = fs_strlen(src_str);
	if (l1 != l2)  // error
	{
		printf("inside fs_strcpy(): Error. The strings should be of the same length.\n");
		return;
	}
	// else
	for (i = 0; i <= l1; i++)
		dst_str[i] = src_str[i];
	dst_str[i] = '\0';
}

// ===== tested ===== //
// can also pass a struct as parameter
/* buf: set which buffer
 * c: set to a char
 * count: set how many bytes
 */
void fs_memset(void *buf, u8 c, u32 count)
{
	int i;
	u8 *ptr = (u8 *)buf;
	for (i = 0; i <= count - 1; i++)
		*(ptr + i) = c;
}

// ===== tested ===== //
/* dst: the buffer to accept the copy
 * src: the buffer to be copied
 * count: how many bytes to be copied
 */
void fs_memcpy(void *dst, void *src, u32 count)
{
	int i;
	u8 *dst_ptr = (u8 *)dst;
	u8 *src_ptr = (u8 *)src;
	for (i = 0; i <= count - 1; i++)
		*(dst_ptr + i) = *(src_ptr + i);
}

// providing a cluster addr (RELATIVE), return the addr of its 1st sector
u32 sec_addr_from_clus(u32 clus_num)
{  // "2": cluster starts from 2 in data section
	return (clus_num - 2) * 8 + FSINFO_sec.first_data_sector;
}

// providing a sector addr, return the num of cluster it belongs to
u32 clus_num_from_sec(u32 sec_num)
{  // "sec_num" should be more than "first_data_sector"
	return (sec_num - FSINFO_sec.first_data_sector) / 8 + 2;
}

/* "buf": buffer block containing the sector in data section
 * "offset": byet offset of the dst directory entry
 * byte offset inside the entry: 20, 26 (start_clus_high_16, start_clus_low_16)
 * return: num of start cluster.
 */
u32 get_file_start_clus(u16 offset, BUF_512 *buf)
{
	return (get_u16(buf->buf + offset + 20) << 16) + get_u16(buf->buf + offset + 26);
}

// given a cluster number, get the related FAT entry value
// to get the next linked cluster.
// 0x0FFFFFFF marking the end
u32 FAT_entry_from_clus(u32 clus)
{
	// offset from the start of FAT table
	u32 offset = clus << 2;  // clus * 4, since each entry has 4 bytes
	u32 sec_num = DBR_sec.attrs.reserved_secs + offset / DBR_sec.attrs.bytes_per_sec;  // RELATIVE
	u32 sec_offset = offset % DBR_sec.attrs.bytes_per_sec;  // offset from the start of sector
	int buf_index = read_FAT_sector(sec_num);  // read in this sector
	return get_u32(fat_buf[buf_index].buf + sec_offset);
}
