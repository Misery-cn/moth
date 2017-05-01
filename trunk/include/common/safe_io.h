#ifndef _SAFE_IO_
#define _SAFE_IO_

#include <sys/types.h>
#include "config.h"
#include "compiler.h"
#include "define.h"


#ifdef __cplusplus
extern "C" {
#endif

ssize_t safe_read(int fd, void *buf, size_t count) __attr_warn_unused_result__;
ssize_t safe_write(int fd, const void *buf, size_t count) __attr_warn_unused_result__;
ssize_t safe_pread(int fd, void *buf, size_t count, off_t offset) __attr_warn_unused_result__;
ssize_t safe_pwrite(int fd, const void *buf, size_t count, off_t offset) __attr_warn_unused_result__;

#ifdef HAVE_SPLICE
ssize_t safe_splice(int fd_in, off_t *off_in, int fd_out, off_t *off_out, size_t len, unsigned int flags) __attr_warn_unused_result__;
ssize_t safe_splice_exact(int fd_in, off_t *off_in, int fd_out, off_t *off_out, size_t len, unsigned int flags) __attr_warn_unused_result__;
#endif

ssize_t safe_read_exact(int fd, void *buf, size_t count) __attr_warn_unused_result__;
ssize_t safe_pread_exact(int fd, void *buf, size_t count, off_t offset) __attr_warn_unused_result__;


int safe_write_file(const char *base, const char *file, const char *val, size_t vallen);
int safe_read_file(const char *base, const char *file, char *val, size_t vallen);

#ifdef __cplusplus
}
#endif

#endif
