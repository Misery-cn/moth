#ifndef _DEFINE_H_
#define _DEFINE_H_

#include <stdarg.h>
#include "acconfig.h"

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
	} while(0);

/*static inline void 
DELETE_P(void* p)
{
	if (p)
	{
	    delete p;
		p = NULL;
	}
}*/

#define DELETE_ARRAY(array) \
	do { \
		if (array) \
		{ \
			delete[] array; \
			array = NULL; \
		} \
	} while(0);



// 计算成员在结构体中的偏移量
#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif


#define get_struct_head_address(struct_type, member_name, member_address) \
                ((struct_type *)((char *)(member_address) - offsetof(struct_type, member_name)))


#ifndef TEMP_FAILURE_RETRY
#define TEMP_FAILURE_RETRY(expression) ({     \
	__typeof(expression) __result;              \
	do {                                        \
		__result = (expression);                  \
	} while (__result == -1 && errno == EINTR); \
	__result; })
#endif


#ifdef __cplusplus
# define VOID_TEMP_FAILURE_RETRY(expression) \
   static_cast<void>(TEMP_FAILURE_RETRY(expression))
#else
# define VOID_TEMP_FAILURE_RETRY(expression) \
   do { (void)TEMP_FAILURE_RETRY(expression); } while (0)
#endif


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
