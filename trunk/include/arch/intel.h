#ifndef _ARCH_INTEL_H_
#define _ARCH_INTEL_H_

#ifdef __cplusplus
extern "C" {
#endif

extern int arch_intel_pclmul; /* true if we have PCLMUL features */
extern int arch_intel_sse42;  /* true if we have sse 4.2 features */
extern int arch_intel_sse41;  /* true if we have sse 4.1 features */
extern int arch_intel_ssse3;  /* true if we have ssse 3 features */
extern int arch_intel_sse3;   /* true if we have sse 3 features */
extern int arch_intel_sse2;   /* true if we have sse 2 features */
extern int arch_intel_probe(void);

#ifdef __cplusplus
}
#endif

#endif
