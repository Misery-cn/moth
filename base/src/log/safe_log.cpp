#include "safe_log.h"

static __thread int g_thread_log_fd = -1;

void close_thread_log_fd()
{
    if (-1 != g_thread_log_fd)
    {
        if (-1 == close(g_thread_log_fd))
		{
            fprintf(stderr, "[%d:%lu] SafeLogger close error: %m\n", getpid(), pthread_self());
        }

        g_thread_log_fd = -1;
    }
}

CSafeLogger* create_safe_logger(bool enable_program_path, uint16_t log_line_size) throw (sys::CSysCallException)
{
    std::string log_dirpath = get_log_dirpath(enable_program_path);
    std::string log_filename = get_log_filename();
    CSafeLogger* logger = new CSafeLogger(log_dirpath.c_str(), log_filename.c_str(), log_line_size);

    set_log_level(logger);
    enable_screen_log(logger);
	
    return logger;
}

CSafeLogger* create_safe_logger(const std::string& log_dirpath, const std::string& cpp_filename, uint16_t log_line_size) throw (sys::CSysCallException)
{
    char* cpp_filepath = strdup(cpp_filename.c_str());
    std::string only_filename = basename(cpp_filepath);
    free(cpp_filepath);

    std::string log_filename = utils::CStringUtils::replace_suffix(only_filename, ".log");
    CSafeLogger* logger = new CSafeLogger(log_dirpath.c_str(), log_filename.c_str(), log_line_size);

    set_log_level(logger);
	
    return logger;
}

CSafeLogger::CSafeLogger(const char* log_dir, const char* log_filename, uint16_t log_line_size) throw (sys::CSysCallException)
    : auto_adddot_(false), auto_newline_(true), bin_log_enabled_(false), trace_log_enabled_(false), raw_log_enabled_(false), screen_enabled_(false), log_dir_(log_dir), log_filename_(log_filename)
{
    atomic_set(&max_bytes_, DEFAULT_LOG_FILE_SIZE);
    atomic_set(&log_level_, LOG_LEVEL_INFO);
    atomic_set(&backup_number_, DEFAULT_LOG_FILE_BACKUP_NUMBER);

    // 保证日志行最大长度不小于指定值
    log_line_size_ = (log_line_size < LOG_LINE_SIZE_MIN) ? LOG_LINE_SIZE_MIN : log_line_size;
    if (LOG_LINE_SIZE_MAX < log_line_size_)
        log_line_size_ = LOG_LINE_SIZE_MAX;

    log_filepath_ = log_dir_ + std::string("/") + log_filename_;
    log_fd_ = open(log_filepath_.c_str(), O_WRONLY|O_CREAT|O_APPEND, FILE_DEFAULT_PERM);
    if (-1 == log_fd_)
    {
        fprintf(stderr, "[%d:%lu] SafeLogger open %s error: %m\n", getpid(), pthread_self(), log_filepath_.c_str());

        THROW_SYSCALL_EXCEPTION(NULL, errno, "open");
    }
}

CSafeLogger::~CSafeLogger()
{
    if (-1 != log_fd_)
    {
        close(log_fd_);
    }
}

void CSafeLogger::enable_screen(bool enabled)
{
    screen_enabled_ = enabled;
}

void CSafeLogger::enable_bin_log(bool enabled)
{
    bin_log_enabled_ = enabled;
}

void CSafeLogger::enable_trace_log(bool enabled)
{
    trace_log_enabled_ = enabled;
}

void CSafeLogger::enable_raw_log(bool enabled)
{
    raw_log_enabled_ = enabled;
}

void CSafeLogger::enable_auto_adddot(bool enabled)
{
    auto_adddot_ = enabled;
}

void CSafeLogger::enable_auto_newline(bool enabled)
{
    auto_newline_ = enabled;
}

void CSafeLogger::set_log_level(log_level_t log_level)
{
    atomic_set(&log_level_, log_level);
}

void CSafeLogger::set_single_filesize(uint32_t filesize)
{
    uint32_t max_bytes = (LOG_LINE_SIZE_MIN * 10 > filesize) ? LOG_LINE_SIZE_MIN * 10 : filesize;
	
    atomic_set(&max_bytes_, max_bytes);
}

void CSafeLogger::set_backup_number(uint16_t backup_number)
{
    atomic_set(&backup_number_, backup_number);
}

bool CSafeLogger::enabled_bin()
{
    return bin_log_enabled_;
}

bool CSafeLogger::enabled_detail()
{
    return LOG_LEVEL_DETAIL >= atomic_read(&log_level_);
}

bool CSafeLogger::enabled_debug()
{
    return LOG_LEVEL_DEBUG >= atomic_read(&log_level_);
}

bool CSafeLogger::enabled_info()
{
    return LOG_LEVEL_INFO >= atomic_read(&log_level_);
}

bool CSafeLogger::enabled_warn()
{
    return LOG_LEVEL_WARN >= atomic_read(&log_level_);
}

bool CSafeLogger::enabled_error()
{
    return LOG_LEVEL_ERROR >= atomic_read(&log_level_);
}

bool CSafeLogger::enabled_fatal()
{
    return LOG_LEVEL_FATAL >= atomic_read(&log_level_);
}

bool CSafeLogger::enabled_state()
{
    return LOG_LEVEL_STATE >= atomic_read(&log_level_);
}

bool CSafeLogger::enabled_trace()
{
    return trace_log_enabled_;
}

bool CSafeLogger::enabled_raw()
{
    return raw_log_enabled_;
}

void CSafeLogger::log_detail(const char* filename, int lineno, const char* module_name, const char* format, ...)
{
    if (enabled_detail())
    {
        va_list args;
        va_start(args, format);
        VaListGuard vg(args);

        do_log(LOG_LEVEL_DETAIL, filename, lineno, module_name, format, args);
    }
}

void CSafeLogger::log_debug(const char* filename, int lineno, const char* module_name, const char* format, ...)
{
    if (enabled_debug())
    {
        va_list args;
        va_start(args, format);
        VaListGuard vg(args);

        do_log(LOG_LEVEL_DEBUG, filename, lineno, module_name, format, args);
    }
}

void CSafeLogger::log_info(const char* filename, int lineno, const char* module_name, const char* format, ...)
{
    if (enabled_info())
    {
        va_list args;
        va_start(args, format);
        VaListGuard vg(args);

        do_log(LOG_LEVEL_INFO, filename, lineno, module_name, format, args);
    }
}

void CSafeLogger::log_warn(const char* filename, int lineno, const char* module_name, const char* format, ...)
{
    if (enabled_warn())
    {
        va_list args;
        va_start(args, format);
        VaListGuard vg(args);

        do_log(LOG_LEVEL_WARN, filename, lineno, module_name, format, args);
    }
}

void CSafeLogger::log_error(const char* filename, int lineno, const char* module_name, const char* format, ...)
{
    if (enabled_error())
    {
        va_list args;
        va_start(args, format);
        VaListGuard vg(args);

        do_log(LOG_LEVEL_ERROR, filename, lineno, module_name, format, args);
    }
}

void CSafeLogger::log_fatal(const char* filename, int lineno, const char* module_name, const char* format, ...)
{
    if (enabled_fatal())
    {
        va_list args;
        va_start(args, format);
        VaListGuard vg(args);

        do_log(LOG_LEVEL_FATAL, filename, lineno, module_name, format, args);
    }
}

void CSafeLogger::log_state(const char* filename, int lineno, const char* module_name, const char* format, ...)
{
    if (enabled_state())
    {
        va_list args;
        va_start(args, format);
        VaListGuard vg(args);

        do_log(LOG_LEVEL_STATE, filename, lineno, module_name, format, args);
    }
}

void CSafeLogger::log_trace(const char* filename, int lineno, const char* module_name, const char* format, ...)
{
    if (enabled_trace())
    {
        va_list args;
        va_start(args, format);
        VaListGuard vg(args);

        do_log(LOG_LEVEL_TRACE, filename, lineno, module_name, format, args);
    }
}

void CSafeLogger::log_raw(const char* format, ...)
{
    if (enabled_raw())
    {
        va_list args;
        va_start(args, format);
        VaListGuard vg(args);
		
        do_log(LOG_LEVEL_RAW, NULL, -1, NULL, format, args);
    }
}

void CSafeLogger::log_bin(const char* filename, int lineno, const char* module_name, const char* log, uint16_t size)
{
    if (enabled_bin())
    {
        // set_log_length(log, size);
        // log_thread_->push_log(log);
    }
}

int CSafeLogger::get_thread_log_fd() const
{
    if (g_thread_log_fd != log_fd_)
    {
        if (-1 != g_thread_log_fd)
        {
            close(g_thread_log_fd);
        }

        sys::CMutexGuard guard(lock_);
		
        if (-1 == log_fd_)
        {
            g_thread_log_fd = -1;
        }
        else
        {
            g_thread_log_fd = dup(log_fd_);
        }
    }

    return g_thread_log_fd;
}

bool CSafeLogger::need_rotate(int fd) const
{
    return utils::CFileUtils::get_file_size(fd) > static_cast<off_t>(max_bytes_);
}

void CSafeLogger::do_log(log_level_t log_level, const char* filename, int lineno, const char* module_name, const char* format, va_list& args)
{
    int log_real_size = 0;
    utils::ScopedArray<char> log_line(new char[log_line_size_]);

    if (LOG_LEVEL_RAW == log_level)
    {
        // fix_vsnprintf()的返回值包含了结尾符在内的长度
        log_real_size = utils::CStringUtils::fix_vsnprintf(log_line.get(), log_line_size_, format, args);
		// 结尾符不需要写入日志文件中
        --log_real_size;
    }
    else
    {
    	// 每条日志的头
        std::stringstream log_header;
        char datetime[sizeof("1970-00-00 00:00:00/0123456789")];
        utils::get_formatted_current_datetime(datetime, sizeof(datetime));

        // 日志头内容：[日期][线程ID/进程ID][日志级别][模块名][代码文件名][代码行号]
        log_header << "[" << datetime << "]"
                   << "[" << pthread_self() << "/" << getpid() << "]"
                   << "[" << get_log_level_name(log_level) << "]";
		
        if (NULL != module_name)
        {
            log_header << "[" << module_name << "]";
        }
		
        log_header << "[" << utils::CStringUtils::extract_filename(filename) << ":" << lineno << "]";

        int m = 0;
		int n = 0;
        // fix_snprintf()的返回值大小包含了结尾符
        m = utils::CStringUtils::fix_snprintf(log_line.get(), log_line_size_, "%s", log_header.str().c_str());
        n = utils::CStringUtils::fix_vsnprintf(log_line.get() + m - 1, log_line_size_ - m, format, args);
        log_real_size = m + n - 2;

        // 是否自动添加结尾用的点号
        if (auto_adddot_)
        {
            // 如果已有结尾的点，则不再添加，以免重复
            if ('.' != log_line.get()[log_real_size - 1])
            {
                log_line.get()[log_real_size] = '.';
                ++log_real_size;
            }
        }

        // 是否自动换行
        if (auto_newline_)
        {
            // 如果已有一个换行符，则不再添加
            if ('\n' != log_line.get()[log_real_size - 1])
            {
                log_line.get()[log_real_size] = '\n';
                ++log_real_size;
            }
        }
    }

    // 打印在屏幕上
    if (screen_enabled_)
    {
        (void)write(STDOUT_FILENO, log_line.get(), log_real_size);
    }

    // 写日志文件
    int thread_log_fd = get_thread_log_fd();
    if (-1 != thread_log_fd)
    {
        int bytes = write(thread_log_fd, log_line.get(), log_real_size);
        if (-1 == bytes)
        {
            fprintf(stderr, "[%d:%lu] SafeLogger[%d] write error: %m\n", getpid(), pthread_self(), thread_log_fd);
        }
        else if (0 < bytes)
        {
            try
            {
                // 判断是否需要滚动
                if (need_rotate(thread_log_fd))
                {
                    std::string lock_path = log_dir_ + std::string("/.") + log_filename_ + std::string(".lock");
                    utils::FileLocker file_locker(lock_path.c_str(), true);

                    // fd可能已被其它进程或线程滚动了，所以这里需要重新open一下
                    int log_fd = open(log_filepath_.c_str(), O_WRONLY|O_CREAT|O_APPEND, FILE_DEFAULT_PERM);
                    if (-1 == log_fd)
                    {
                        fprintf(stderr, "[%d:%lu] SafeLogger open %s error: %m\n", getpid(), pthread_self(), log_filepath_.c_str());
                    }
                    else
                    {
                        try
                        {
                            if (need_rotate(log_fd))
                            {
                                close(log_fd);
                                rotate_log();
                            }
                            else
                            {
                                // 如果是线程执行了滚动，则log_fd_值为非-1，可直接使用
                                // 如果是其它进程执行了滚动了，则应当使用log_fd替代log_fd_
                                // 可以放在锁FileLocker之外，用来保护log_fd_
                                // 由于每个线程并不直接使用_log_fd，而是对log_fd_做了dup，所以只要保护log_fd_即可
                                sys::CMutexGuard guard(lock_);

                                close(log_fd_);
                                log_fd_ = log_fd;
                            }
                        }
                        catch (sys::CSysCallException& ex)
                        {
                            fprintf(stderr, "[%d:%lu] SafeLogger[%d] rotate error: %s.\n", getpid(), pthread_self(), log_fd, ex.to_string().c_str());
                        }
                    }
                }
            }
            catch (sys::CSysCallException& ex)
            {
                fprintf(stderr, "[%d:%lu] SafeLogger[%d] rotate error: %s\n", getpid(), pthread_self(), thread_log_fd, ex.to_string().c_str());
            }
        }
    }
}

void CSafeLogger::rotate_log()
{
	// 滚动后的文件路径，包含目录和文件名
    std::string new_path;
	// 滚动前的文件路径，包含目录和文件名
    std::string old_path;

    // 历史滚动
    for (uint8_t i = backup_number_ - 1; 1 < i; --i)
    {
        new_path = log_dir_ + std::string("/") + log_filename_ + std::string(".") + utils::CStringUtils::any2string(static_cast<int>(i));
        old_path = log_dir_ + std::string("/") + log_filename_ + std::string(".") + utils::CStringUtils::any2string(static_cast<int>(i - 1));

        if (0 == access(old_path.c_str(), F_OK))
        {
            if (-1 == rename(old_path.c_str(), new_path.c_str()))
            {
                fprintf(stderr, "[%d:%lu] SafeLogger rename %s to %s error: %m.\n", getpid(), pthread_self(), old_path.c_str(), new_path.c_str());
            }
        }
        else
        {
            if (errno != ENOENT)
            {
                fprintf(stderr, "[%d:%lu] SafeLogger access %s error: %m.\n", getpid(), pthread_self(), old_path.c_str());
            }
        }
    }

    if (0 < backup_number_)
    {
        // 当前滚动
        new_path = log_dir_ + std::string("/") + log_filename_ + std::string(".1");
        if (0 == access(log_filepath_.c_str(), F_OK))
        {
            if (-1 == rename(log_filepath_.c_str(), new_path.c_str()))
            {
                fprintf(stderr, "[%d:%lu] SafeLogger rename %s to %s error: %m\n", getpid(), pthread_self(), log_filepath_.c_str(), new_path.c_str());
            }
        }
        else
        {
            if (errno != ENOENT)
            {
                fprintf(stderr, "[%d:%lu] SafeLogger access %s error: %m\n", getpid(), pthread_self(), log_filepath_.c_str());
            }
        }
    }

    // 重新创建
    printf("[%d:%lu] SafeLogger create %s\n", getpid(), pthread_self(), log_filepath_.c_str());
    int log_fd = open(log_filepath_.c_str(), O_WRONLY|O_CREAT|O_EXCL, FILE_DEFAULT_PERM);
    if (-1 == log_fd)
    {
        fprintf(stderr, "[%d:%lu] SafeLogger create %s error: %m\n", getpid(), pthread_self(), log_filepath_.c_str());
    }

    sys::CMutexGuard guard(lock_);
    if (-1 == close(log_fd_))
    {
        fprintf(stderr, "[%d:%lu] SafeLogger close %s error: %m.\n", getpid(), pthread_self(), log_filepath_.c_str());
    }
    log_fd_ = log_fd;
}

