#ifndef _ARCH_ARM_H_
#define _ARCH_ARM_H_

#ifdef __cplusplus
extern "C" {
#endif

extern int arch_neon;  /* true if we have ARM NEON or ASIMD abilities */
extern int arch_aarch64_crc32;  /* true if we have AArch64 CRC32/CRC32C abilities */

extern int arch_arm_probe(void);

#ifdef __cplusplus
}
#endif

#endif
