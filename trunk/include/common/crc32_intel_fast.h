#ifndef _CRC32C_INTEL_FAST_H_
#define _CRC32C_INTEL_FAST_H_

#ifdef __cplusplus
extern "C" {
#endif

extern int crc32c_intel_fast_exists(void);

#ifdef __x86_64__
extern uint32_t crc32c_intel_fast(uint32_t crc, unsigned char const* buffer, unsigned len);
#else
static inline uint32_t crc32c_intel_fast(uint32_t crc, unsigned char const* buffer, unsigned len)
{
    return 0;
}
#endif

#ifdef __cplusplus
}
#endif

#endif
