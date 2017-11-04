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

void fs_strcpy(u8 *dst_str, u8 *src_str)
{
	int i;
	int l1 = fs_strlen(str1), l2 = fs_strlen(str2);
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