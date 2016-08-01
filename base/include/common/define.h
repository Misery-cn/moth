#ifndef _DEFINE_H_
#define _DEFINE_H_

#if defined(HAVE_STDINT)
#include <stdint.h>
#else
typedef unsigned int  uint32_t;
typedef unsigned short  uint16_t;
#if __WORDSIZE == 64
typedef unsigned long int	uint64_t;
typedef long int		int64_t;
#else
__extension__
typedef unsigned long long int	uint64_t;
typedef long long int		int64_t;
#endif
// #define INT64_MAX  9223372036854775807
#endif /* HAVE_STDINT */


#define IN
#define OUT

// 待完成RUNLOG
#define RUNLOG printf

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

#define WORK_DIR "WORK_DIR"

#endif
