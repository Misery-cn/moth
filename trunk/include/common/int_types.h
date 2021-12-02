#ifndef _INT_TYPES_H_
#define _INT_TYPES_H_

#include "acconfig.h"

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif


#ifdef HAVE_STDINT_H
#include <stdint.h>
#else

typedef signed char                int8_t;
typedef short int                int16_t;
typedef int                        int32_t;

#if __WORDSIZE == 64
typedef long int                    int64_t;
#else
__extension__
typedef long long int            int64_t;
#endif

typedef unsigned char            uint8_t;
typedef unsigned short int        uint16_t;

#ifndef __uint32_t_defined
typedef unsigned int                uint32_t;
#define __uint32_t_defined
#endif

#if __WORDSIZE == 64
typedef unsigned long int        uint64_t;
#else
__extension__
typedef unsigned long long int    uint64_t;
#endif
// #define INT64_MAX  9223372036854775807

#endif


#ifdef HAVE_LINUX_TYPES_H
#include <linux/types.h>
#endif


#if !defined(PRIu64)
#  if defined(HAVE_INTTYPES_H) || defined(HAVE_GLIB)
#    define PRIu64 "llu"
#    define PRIi64 "lli"
#    define PRIx64 "llx"
#    define PRIX64 "llX"
#    define PRIo64 "llo"
#    define PRId64 "lld"
#  else
#    define PRIu64 "lu"
#    define PRIi64 "li"
#    define PRIx64 "lx"
#    define PRIX64 "lX"
#    define PRIo64 "lo"
#    define PRId64 "ld"
#  endif
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifndef HAVE_LINUX_TYPES_H
#ifndef HAVE___U8
typedef uint8_t __u8;
#endif

#ifndef HAVE___S8
typedef int8_t __s8;
#endif

#ifndef HAVE___U16
typedef uint16_t __u16;
#endif

#ifndef HAVE___S16
typedef int16_t __s16;
#endif

#ifndef HAVE___U32
typedef uint32_t __u32;
#endif

#ifndef HAVE___S32
typedef int32_t __s32;
#endif

#ifndef HAVE___U64
typedef uint64_t __u64;
#endif

#ifndef HAVE___S64
typedef int64_t __s64;
#endif
#endif /* LINUX_TYPES_H */

#define __bitwise__

typedef __u16 __bitwise__ __le16;
typedef __u16 __bitwise__ __be16;
typedef __u32 __bitwise__ __le32;
typedef __u32 __bitwise__ __be32;
typedef __u64 __bitwise__ __le64;
typedef __u64 __bitwise__ __be64;

#endif
