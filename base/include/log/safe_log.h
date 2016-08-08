#ifndef _SAFE_LOG_H_
#define _SAFE_LOG_H_

#include <stdio.h>
#include "log.h"
#include "exception.h"
#include "mutex.h"
#include "atomic.h"
#include "string_utils.h"
#include "utils.h"
#include "file_utils.h"
#include "datetime_utils.h"
#include "file_locker.h"

// 默认的文件模式
#define FILE_DEFAULT_PERM (S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)


class CSafeLogger;


extern void close_thread_log_fd();

extern CSafeLogger* create_safe_logger(bool enable_program_path = true, uint16_t log_line_size = 8192) throw (sys::CSysCallException);

extern CSafeLogger* create_safe_logger(const std::string& log_dirpath, const std::string& cpp_filename, uint16_t log_line_size = 8192) throw (sys::CSysCallException);


class CSafeLogger : public CLog
{
public:
    CSafeLogger(const char* log_dir, const char* log_filename, uint16_t log_line_size=8192) throw (sys::CSysCallException);
    ~CSafeLogger();

    virtual void enable_screen(bool enabled);

    virtual void enable_bin_log(bool enabled);

    virtual void enable_trace_log(bool enabled);

    virtual void enable_raw_log(bool enabled);

    virtual void enable_auto_adddot(bool enabled);

    virtual void enable_auto_newline(bool enabled);

    virtual void set_log_level(log_level_t log_level);

    virtual void set_single_filesize(uint32_t filesize);

    virtual void set_backup_number(uint16_t backup_number);

    virtual bool enabled_bin();
	
    virtual bool enabled_detail();

    virtual bool enabled_debug();

    virtual bool enabled_info();

    virtual bool enabled_warn();

    virtual bool enabled_error();

    virtual bool enabled_fatal();

    virtual bool enabled_state();

    virtual bool enabled_trace();

    virtual bool enabled_raw();

    virtual void log_detail(const char* filename, int lineno, const char* module_name, const char* format, ...)  __attribute__((format(printf, 5, 6)));
    virtual void log_debug(const char* filename, int lineno, const char* module_name, const char* format, ...) __attribute__((format(printf, 5, 6)));
    virtual void log_info(const char* filename, int lineno, const char* module_name, const char* format, ...) __attribute__((format(printf, 5, 6)));
    virtual void log_warn(const char* filename, int lineno, const char* module_name, const char* format, ...) __attribute__((format(printf, 5, 6)));
    virtual void log_error(const char* filename, int lineno, const char* module_name, const char* format, ...) __attribute__((format(printf, 5, 6)));
    virtual void log_fatal(const char* filename, int lineno, const char* module_name, const char* format, ...) __attribute__((format(printf, 5, 6)));
    virtual void log_state(const char* filename, int lineno, const char* module_name, const char* format, ...) __attribute__((format(printf, 5, 6)));
    virtual void log_trace(const char* filename, int lineno, const char* module_name, const char* format, ...) __attribute__((format(printf, 5, 6)));

    virtual void log_raw(const char* format, ...) __attribute__((format(printf, 2, 3)));

    virtual void log_bin(const char* filename, int lineno, const char* module_name, const char* log, uint16_t size);

private:
    int get_thread_log_fd() const;
    bool need_rotate(int fd) const;
    void do_log(log_level_t log_level, const char* filename, int lineno, const char* module_name, const char* format, va_list& args);
    void rotate_log();

private:
    int log_fd_;
    bool auto_adddot_;
    bool auto_newline_;
    uint16_t log_line_size_;
    atomic_t log_level_;
    bool bin_log_enabled_;
    bool trace_log_enabled_;
    bool raw_log_enabled_;

private:
    bool screen_enabled_;
    atomic_t max_bytes_;
    atomic_t backup_number_;
    std::string log_dir_;
    std::string log_filename_;
    std::string log_filepath_;
    mutable sys::CMutex lock_;
};

#endif
