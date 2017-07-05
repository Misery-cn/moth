#include "acconfig.h"
#include "int_types.h"
#include "crc32_intel_baseline.h"

extern unsigned int crc32_iscsi_00(unsigned char const* buffer, int len, unsigned int crc);
extern unsigned int crc32_iscsi_zero_00(unsigned char const* buffer, int len, unsigned int crc);

#ifdef HAVE_GOOD_YASM_ELF64
uint32_t crc32c_intel_fast(uint32_t crc, unsigned char const* buffer, unsigned len)
{
	uint32_t v;
	unsigned left;


	if (!buffer)
	{
		return crc32_iscsi_zero_00(buffer, len, crc);
	}

	if (len < 16)
	{
		return crc32c_intel_baseline(crc, buffer, len);
	}
	
	left = ((unsigned long)buffer + len) & 7;
	len -= left;
	v = crc32_iscsi_00(buffer, len, crc);
	
	if (left)
	{
		v = crc32c_intel_baseline(v, buffer + len, left);
	}
	
	return v;
}

// yasm
int crc32c_intel_fast_exists(void)
{
	return 1;
}
#else

int crc32c_intel_fast_exists(void)
{
	return 0;
}

uint32_t crc32c_intel_fast(uint32_t crc, unsigned char const* buffer, unsigned len)
{
	return 0;
}

#endif
