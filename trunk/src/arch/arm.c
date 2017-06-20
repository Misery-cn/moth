#include <stdio.h>
#include "arm.h"

int arch_neon = 0;
int arch_aarch64_crc32 = 0;

#if __linux__

#include <elf.h>
#include <link.h> // ElfW macro

#if __arm__ || __aarch64__
#include <asm/hwcap.h>
#endif // __arm__

static unsigned long get_auxval(unsigned long type)
{
	unsigned long result = 0;
	FILE *f = fopen("/proc/self/auxv", "r");
	if (f)
	{
		ElfW(auxv_t) entry;
		while (fread(&entry, sizeof(entry), 1, f) == 1)
		{
			if (entry.a_type == type)
			{
				result = entry.a_un.a_val;
				break;
			}
		}
		
		fclose(f);
	}
	
	return result;
}

static unsigned long get_hwcap(void)
{
	return get_auxval(AT_HWCAP);
}

#endif // __linux__

int arch_arm_probe(void)
{
#if __arm__ && __linux__
	arch_neon = (get_hwcap() & HWCAP_NEON) == HWCAP_NEON;
#elif __aarch64__ && __linux__
	arch_neon = (get_hwcap() & HWCAP_ASIMD) == HWCAP_ASIMD;
# ifdef HWCAP_CRC32
	arch_aarch64_crc32 = (get_hwcap() & HWCAP_CRC32) == HWCAP_CRC32;
# else
	arch_aarch64_crc32 = 0;
# endif
#else
	if (0)
		get_hwcap();  // make compiler shut up
#endif
	return 0;
}

