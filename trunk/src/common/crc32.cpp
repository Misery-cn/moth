#include "crc32.h"
#include "intel.h"
#include "probe.h"
#include "crc32_intel_fast.h"
#include "crc32_aarch64.h"
#include "crc32_sctp.h"

crc32c_func_t choose_crc32(void)
{
    arch_probe();

    if (arch_intel_sse42 && crc32c_intel_fast_exists())
    {
        return crc32c_intel_fast;
    }

    if (arch_aarch64_crc32)
    {
        return crc32c_aarch64;
    }

    return crc32c_sctp;
}


crc32c_func_t crc32c_func = choose_crc32();

