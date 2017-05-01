#ifndef _CRC32C_H_
#define _CRC32C_H_

#include <inttypes.h>
#include <string.h>

typedef uint32_t (*crc32c_func_t)(uint32_t crc, unsigned char const* data, uint32_t length);

extern crc32c_func_t crc32c_func;

extern crc32c_func_t choose_crc32(void);

static inline uint32_t crc32c(uint32_t crc, unsigned char const *data, unsigned length)
{
	return crc32c_func(crc, data, length);
}

#endif
