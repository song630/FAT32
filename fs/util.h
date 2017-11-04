#ifndef _UTIL_H_
#define _UTIL_H_

typedef unsigned char u8;  // 1 byte
typedef unsigned short u16;  // 2 bytes
typedef unsigned long u32;  // 4 bytes

u16 get_u16(u8 *addr);

u32 get_u32(u8 *addr);

// below are some operations defined for strings:

int fs_strlen(u8 *str);

int fs_strcmp(u8 *str1, u8 *str2);  // only has two results: equal & not equal

void fs_strcpy(u8 *dst_str, u8 *src_str);

#endif
