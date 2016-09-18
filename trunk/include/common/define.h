#ifndef _DEFINE_H_
#define _DEFINE_H_

#include <stdarg.h>
#include "config.h"

#if defined(HAVE_STDINT_H)
#include <stdint.h>
#else
typedef signed char		int8_t;
typedef short int		int16_t;
typedef int			int32_t;
#if __WORDSIZE == 64
typedef long int		int64_t;
#else
__extension__
typedef long long int		int64_t;
#endif

/* Unsigned.  */
typedef unsigned char		uint8_t;
typedef unsigned short int	uint16_t;
#ifndef __uint32_t_defined
typedef unsigned int		uint32_t;
#define __uint32_t_defined
#endif
#if __WORDSIZE == 64
typedef unsigned long int	uint64_t;
#else
__extension__
typedef unsigned long long int	uint64_t;
#endif
// #define INT64_MAX  9223372036854775807
#endif /* HAVE_STDINT */


#define IN
#define OUT

// sys命名空间宏
#define SYS_NS_BEGIN namespace sys {
#define SYS_NS_END }

// util命名空间宏
#define UTILS_NS_BEGIN namespace utils {
#define UTILS_NS_END	}

#define DELETE_P(p) \
	do { \
		if (p) \
		{ \
			delete p; \
			p = NULL; \
		} \
	} while(0)

/*static inline void 
DELETE_P(void* p)
{
	if (p)
	{
	    delete p;
		p = NULL;
	}
}*/


// list_entry
// 计算成员在结构体中的偏移量
#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif


#define get_struct_head_address(struct_type, member_name, member_address) \
                ((struct_type *)((char *)(member_address) - offsetof(struct_type, member_name)))



// 文件全名最大字节数
#ifdef FILENAME_MAX
#undef FILENAME_MAX
#endif
#define FILENAME_MAX 256
		
// 目录最大字节数
#ifdef PATH_MAX
#undef PATH_MAX
#endif		
#define PATH_MAX 256

#ifndef	LINE_MAX
#define	LINE_MAX 1024
#endif


// IO操作通用缓冲区大小
#ifdef IO_BUFFER_MAX
#undef IO_BUFFER_MAX
#endif
#define IO_BUFFER_MAX 4096

#define WORK_DIR "WORK_DIR"

#endif
