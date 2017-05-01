#ifndef _FILE_UTILS_H_
#define _FILE_UTILS_H_
#include "utils.h"

// UTILS_NS_BEGIN

class FileUtils
{
public:
	// 文件复制函数,返回文件大小
	// src_fd: 打开的源文件句柄
	// dst_fd: 打开的目的文件句柄
    static size_t file_copy(int src_fd, int dst_fd) throw (SysCallException);
    static size_t file_copy(int src_fd, const char* dst_filename) throw (SysCallException);
    static size_t file_copy(const char* src_filename, int dst_fd) throw (SysCallException);
    static size_t file_copy(const char* src_filename, const char* dst_filename) throw (SysCallException);

	// 获取文件字节数
    static off_t get_file_size(int fd) throw (SysCallException);
    static off_t get_file_size(const char* filepath) throw (SysCallException);
    
	// 获取文件32位crc值
	// 注意：crc32_file会修改读写文件的偏移值
    static uint32_t crc32_file(int fd) throw (SysCallException);
    static uint32_t crc32_file(const char* filepath) throw (SysCallException);
    
	// 获取文件权限模式
    static uint32_t get_file_mode(int fd) throw (SysCallException);

	// 删除文件
    static void remove(const char* filepath) throw (SysCallException);

	// 重命名文件
    static void rename(const char* from_filepath, const char* to_filepath) throw (SysCallException);

	static bool exist(const char* filepath);
};

// UTILS_NS_END

#endif
