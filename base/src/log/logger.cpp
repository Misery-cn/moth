#include <stdarg.h>
#include <stdio.h>
#include <sys/uio.h>
#include "logger.h"
#include "datetime_utils.h"
#include "dir_utils.h"
#include "string_utils.h"

#define LOG_FLAG_BIN  0x01
#define LOG_FLAG_TEXT 0x02

// 在log.h中声明
CLog* g_logger = NULL;

// 日志级别名称数组，最大名称长度为8个字符，如果长度不够，编译器会报错
static char log_level_name_array[][8] = { "DETAIL", "DEBUG", "INFO", "WARN", "ERROR", "FATAL", "STATE", "TRACE" };

log_level_t get_log_level(const char* level_name)
{
    if (NULL == level_name)
	{
		return LOG_LEVEL_DEBUG;
    }
	
    if (0 == strcasecmp(level_name, "DETAIL"))
	{
		return LOG_LEVEL_DETAIL;
    }
	
    if (0 == strcasecmp(level_name, "DEBUG"))
	{
		return LOG_LEVEL_DEBUG;
    }
	
    if (0 == strcasecmp(level_name, "TRACE"))
	{
		return LOG_LEVEL_TRACE;
    }
	
    if (0 == strcasecmp(level_name, "INFO"))
	{
		return LOG_LEVEL_INFO;
    }
	
    if (0 == strcasecmp(level_name, "WARN"))
	{
		return LOG_LEVEL_WARN;
    }
	
    if (0 == strcasecmp(level_name, "ERROR"))
	{
		return LOG_LEVEL_ERROR;
    }
	
    if (0 == strcasecmp(level_name, "FATAL"))
	{
		return LOG_LEVEL_FATAL;
    }
	
    if (0 == strcasecmp(level_name, "STATE"))
	{
		return LOG_LEVEL_STATE;
    }

    return LOG_LEVEL_INFO;
}

const char* get_log_level_name(log_level_t log_level)
{
    if ((LOG_LEVEL_DETAIL > log_level) || (LOG_LEVEL_TRACE < log_level))
	{
		return NULL;
    }
	
    return log_level_name_array[log_level];
}

std::string get_log_filename()
{
    std::string program_short_name = utils::CUtils::get_program_short_name();
	
    return utils::CStringUtils::replace_suffix(program_short_name, ".log");
}

std::string get_log_dirpath(bool enable_program_path)
{
    std::string log_dirpath;
    std::string program_path = utils::CUtils::get_program_path();

    try
    {
        log_dirpath = program_path + std::string("/../log");
		
        if (!utils::CDirUtils::exist(log_dirpath))
        {
            if (enable_program_path)
            {
                log_dirpath = program_path;
            }
        }
    }
    catch (sys::CSysCallException& ex)
    {
        if (enable_program_path)
        {
            log_dirpath = program_path;
        }

        fprintf(stderr, "get_log_filepath failed: %s\n", ex.to_string().c_str());
    }

    return log_dirpath;
}

std::string get_log_filepath(bool enable_program_path)
{
    std::string log_dirpath = get_log_dirpath(enable_program_path);
	
    return log_dirpath + std::string("/") + get_log_filename();
}

void set_log_level(CLog* logger)
{
    const char* c_log_level = getenv("LOG_LEVEL");

    if (NULL != c_log_level)
    {
        log_level_t log_level = get_log_level(c_log_level);
		
        logger->set_log_level(log_level);
    }
}

void enable_screen_log(CLog* logger)
{
    const char* c_log_screen = getenv("LOG_SCREEN");

    if ((c_log_screen != NULL) && (0 == strcmp(c_log_screen, "1")))
    {
        logger->enable_screen(true);
    }
}


CLogProber::CLogProber()
{
    if (-1 == pipe(pipe_fd_))
    {
        THROW_SYSCALL_EXCEPTION(NULL, errno, "pipe");
    }
}

CLogProber::~CLogProber()
{
    if (-1 != pipe_fd_[0])
    {
        close(pipe_fd_[0]);
        close(pipe_fd_[1]);
    }
}

void CLogProber::send_signal()
{
    char c = 'x';

    for (;;)
    {
        if (-1 == write(pipe_fd_[1], &c, 1))
        {
            if (EINTR == sys::Error::code())
			{
				continue;
            }
			
            THROW_SYSCALL_EXCEPTION(NULL, errno, "write");
        }

        break;
    }
}

void CLogProber::read_signal(int signal_number)
{
    char signals[LOG_NUMBER_WRITED_ONCE];

    for (;;)
    {
        if (-1 == read(pipe_fd_[0], reinterpret_cast<void*>(signals), static_cast<size_t>(signal_number)))
        {
            if (EINTR == sys::Error::code())
			{
				continue;
            }
			
            THROW_SYSCALL_EXCEPTION(NULL, errno, "read");
        }

        break;
    }
}


sys::CMutex CLogger::thread_lock_;
CLogThread* CLogger::log_thread_ = NULL;

CLogger::CLogger(uint16_t log_line_size)
    : log_fd_(-1), auto_adddot_(false), auto_newline_(true), bin_log_enabled_(false), trace_log_enabled_(false), \
	registered_(false), destroying_(false), screen_enabled_(false), current_bytes_(0), log_queue_(NULL), waiter_number_(0)
{    
    atomic_set(&max_bytes_, DEFAULT_LOG_FILE_SIZE);
    atomic_set(&log_level_, LOG_LEVEL_INFO);
    atomic_set(&backup_number_, DEFAULT_LOG_FILE_BACKUP_NUMBER);

    // 保证日志行最大长度不小于指定值
    log_line_size_ = (log_line_size < LOG_LINE_SIZE_MIN) ? LOG_LINE_SIZE_MIN : log_line_size;
}

CLogger::~CLogger()
{    
    // destroy(); 
    // 删除队列
    delete log_queue_;
    log_queue_ = NULL;

    if (-1 != log_fd_)
    {
        close(log_fd_);
        log_fd_ = -1;      
    }
}

void CLogger::destroy()
{       
    {
        sys::CMutexGuard guard(queue_lock_);

        // 停止Logger的日志，长度为0
        log_message_t* log_message = (log_message_t*)malloc(sizeof(log_message_t));
        log_message->length = 0;
        log_message->content[0] = '\0';
        destroying_ = true;
        push_log_message(log_message);
    }

    {
        sys::CMutexGuard guard(CLogger::thread_lock_);

        CLogger::log_thread_->stop();
        CLogger::log_thread_ = NULL; 
    }
}

void CLogger::create(const char* log_path, const char* log_filename, uint32_t log_queue_size)
{
    // 日志文件路径和文件名
    snprintf(log_path_, sizeof(log_path_), "%s", log_path);
    snprintf(log_filename_, sizeof(log_filename_), "%s", log_filename);    

    // 创建日志队列
    if (0 >= log_queue_size)
    {
        log_queue_size = 1;
    }
	
    log_queue_ = new utils::CArrayList<log_message_t*>(log_queue_size);
    
    // 创建和启动日志线程
    create_thread();

    // 创建日志文件
    create_logfile(false);
}

void CLogger::create_thread()
{
    sys::CMutexGuard guard(CLogger::thread_lock_);
	
    if (NULL == CLogger::log_thread_)
    {
        CLogger::log_thread_ = new CLogThread;

        try
        {
            CLogger::log_thread_->start();
        }
        catch (sys::CSysCallException& ex)
        {
            CLogger::log_thread_ = NULL;
            throw; // 重新抛出异常
        }
    }
}

bool CLogger::execute()
{   
    // 增加引用计数，以保证下面的操作是安全的
    CLogger* self = const_cast<CLogger*>(this);

    try
    {           
        // 写入前，预处理
        if (!prewrite())
        {
            return false;
        }
        if (-1 != log_fd_)
        {
#if HAVE_UIO_H == 1
            return batch_write();
#else
            return single_write();
#endif
        }        
    }
    catch (sys::CSysCallException& ex)
    {
        fprintf(stderr, "Writed log %s/%s error: %s.\n", log_path_, log_filename_, ex.to_string().c_str());
        close_logfile();
    }

    return true;
}

bool CLogger::prewrite()
{
    if (need_create_file())
    {
        close_logfile();
        create_logfile(false);
    }
    else if (need_rotate_file())
    {
        close_logfile();
        rotate_file();
    }

    return is_registered();
}

bool CLogger::single_write()
{
    int r = 0;
    log_message_t* log_message = NULL;
    
    {
        sys::CMutexGuard guard(queue_lock_);        
        log_message = log_queue_->pop_front();
        if (0 < waiter_number_)
        {
            queue_cond_.signal();
        }
    }
        
    // 分成两步，是避免write在Lock中
    if (NULL != log_message)
    {     
        // 读信号
        read_signal(1);
        CLogger::log_thread_->dec_log_number(1);

        // 需要销毁Logger了
        if (0 == log_message->length)
        {
            free(log_message);
            return false;
        }

        // 循环处理中断
        for (;;)
        {
            r = write(log_fd_, log_message->content, log_message->length);
            if ((-1 == r) && (EINTR == sys::Error::code()))
            {
                continue;
            }

            if (0 < r)
            {
                current_bytes_ += (uint32_t)r;
            }
            
            break;
        }               

        // 释放消息
        free(log_message);                

        // 错误处理
        if (-1 == r)
        {
            THROW_SYSCALL_EXCEPTION(NULL, errno, "write");
        }      
    }

    return true;
}

bool CLogger::batch_write()
{
    bool to_destroy_logger = false;

#if HAVE_UIO_H == 1    
    int r = 0;
    int number = 0;
    struct iovec iov_array[LOG_NUMBER_WRITED_ONCE];
    
    {
        CMutexGuard guard(queue_lock_);

        // 批量取出消息
        int i = 0;
        for (; i < LOG_NUMBER_WRITED_ONCE && i < IOV_MAX && !log_queue_->is_empty(); ++i)
        {
            log_message_t* log_message = log_queue_->pop_front();                        
            if (0 < log_message->length)
            {                
                iov_array[i].iov_len = log_message->length;
                iov_array[i].iov_base = log_message->content;
                ++number;
            }
            else
            {
                // 需要销毁Logger了
                read_signal(1);
                free(log_message);
                to_destroy_logger = true;
                CLogger::log_thread_->dec_log_number(1);
            }            
        }
		
        if (0 < waiter_number_)
        {
            queue_event_.broadcast();
        }
    }
  
    // 批量写入文件
    if (0 < number)
    {
        // 读走信号
        read_signal(number);
        CLogger::log_thread_->dec_log_number(number);

        // 循环处理中断
        for (;;)
        {
            r = writev(log_fd_, iov_array, number);
			
            if ((-1 == r) && (EINTR == Error::code()))
            {
                continue;
            }    
			
            if (0 < r)
            {
                // 更新当前日志文件大小
                current_bytes_ += static_cast<uint32_t>(r);
            }
            
            break;
        }                

        // 释放消息
        while (0 < number--)
        {
            log_message_t* log_message = NULL;
            void* iov_base = iov_array[number].iov_base;

            log_message = get_struct_head_address(log_message_t, content, iov_base);
			
            free(log_message);
        }

        // 错误处理
        if (-1 == r)
        {
            throw CSysCallException(Error::code(), __FILE__, __LINE__, "logger writev");
        }
    }   
#endif // HAVE_UIO_H

    return !to_destroy_logger;
}

void CLogger::enable_screen(bool enabled)
{ 
	screen_enabled_ = enabled;
}

void CLogger::enable_bin_log(bool enabled)
{    
    bin_log_enabled_ = enabled;
}

void CLogger::enable_trace_log(bool enabled)
{ 
    trace_log_enabled_ = enabled; 
}

void CLogger::enable_auto_adddot(bool enabled)
{
    auto_adddot_ = enabled;
}

void CLogger::enable_auto_newline(bool enabled)
{ 
    auto_newline_ = enabled;
}

void CLogger::set_log_level(log_level_t log_level)
{
    atomic_set(&log_level_, log_level);
}

void CLogger::set_single_filesize(uint32_t filesize)
{ 
    uint32_t max_bytes = (filesize < LOG_LINE_SIZE_MIN * 10) ? LOG_LINE_SIZE_MIN * 10: filesize;
	
    atomic_set(&max_bytes_, max_bytes);
}

void CLogger::set_backup_number(uint16_t backup_number) 
{
    atomic_set(&backup_number_, backup_number);
}

bool CLogger::enabled_bin()
{
    return bin_log_enabled_;
}

bool CLogger::enabled_detail()
{
    return LOG_LEVEL_DETAIL >= atomic_read(&log_level_);
}

bool CLogger::enabled_debug()
{
    return LOG_LEVEL_DEBUG >= atomic_read(&log_level_);
}

bool CLogger::enabled_info()
{
    return LOG_LEVEL_INFO >= atomic_read(&log_level_);
}

bool CLogger::enabled_warn()
{
    return LOG_LEVEL_WARN >= atomic_read(&log_level_);
}

bool CLogger::enabled_error()
{
    return LOG_LEVEL_ERROR >= atomic_read(&log_level_);
}

bool CLogger::enabled_fatal()
{
    return LOG_LEVEL_FATAL >= atomic_read(&log_level_);
}

bool CLogger::enabled_state()
{
    return LOG_LEVEL_STATE >= atomic_read(&log_level_);
}

bool CLogger::enabled_trace()
{
    return trace_log_enabled_;
}


void CLogger::do_log(log_level_t log_level, const char* filename, int lineno, const char* module_name, const char* format, va_list& args)
{    
    va_list args_copy;
    va_copy(args_copy, args);
    VaListGuard guard(args_copy);
    log_message_t* log_message = (log_message_t*)malloc(log_line_size_ + sizeof(log_message_t) + 1);

    char datetime[sizeof("1970-00-00 00:00:00/0123456789")];
    utils::get_formatted_current_datetime(datetime, sizeof(datetime));
    
    // 模块名称
    std::string module_name_field;
    if (NULL != module_name)
    {
        module_name_field = std::string("[") + module_name + std::string("]");
    }

    // 在构造时，已经保证_log_line_size不会小于指定的值，所以下面的操作是安全的
    int head_length = utils::CStringUtils::fix_snprintf(log_message->content, log_line_size_, "[%s][0x%08x][%s]%s[%s:%d]",
						datetime, sys::CThread::get_current_thread_id(), get_log_level_name(log_level), module_name_field.c_str(), filename, lineno);
    int log_line_length = vsnprintf(log_message->content + head_length, log_line_size_ - head_length, format, args);

    if (log_line_length < log_line_size_ - head_length)
    {
        log_message->length = head_length + log_line_length;
    }
    else
    {
        // 预定的缓冲区不够大，需要增大
        int new_line_length = log_line_length + head_length;
        if (LOG_LINE_SIZE_MAX < new_line_length)
        {
            new_line_length = LOG_LINE_SIZE_MAX;
        }
        
        log_message_t* new_log_message = (log_message_t*)malloc(new_line_length + sizeof(log_message_t) + 1);
        if (NULL == new_log_message)
        {
            // 重新分配失败
            log_message->length = head_length + (log_line_length - 1);
        }
        else
        {
        	// 释放老的，指向新的
            free(log_message);
            log_message = new_log_message;
                                    
            // 这里不需要关心返回值了
            head_length = utils::CStringUtils::fix_snprintf(log_message->content, new_line_length, "[%s][0x%08x][%s]", datetime, sys::CThread::get_current_thread_id(), get_log_level_name(log_level));
            log_line_length = utils::CStringUtils::fix_vsnprintf(log_message->content + head_length, new_line_length - head_length, format, args_copy);            
            log_message->length = head_length + log_line_length;
        }
    }
    
    // 自动添加结尾点号
    if (auto_adddot_ && ('.' != log_message->content[log_message->length - 1]) && ('\n' != log_message->content[log_message->length - 1]))
    {
        log_message->content[log_message->length] = '.';
        log_message->content[log_message->length+1] = '\0';
        ++log_message->length;
    }

    // 自动添加换行符
    if (auto_newline_ && ('\n' != log_message->content[log_message->length - 1]))
    {
        log_message->content[log_message->length] = '\n';
        log_message->content[log_message->length+1] = '\0';
        ++log_message->length;
    }

    // 打印在屏幕上
    if (screen_enabled_)
    {
        (void)write(STDOUT_FILENO, log_message->content, log_message->length);
    }
    
    // 日志消息放入队列中
    sys::CMutexGuard lockguard(queue_lock_);
    if (!destroying_)
    {
        push_log_message(log_message);
    }
}

void CLogger::push_log_message(log_message_t* log_message)
{    
    while (log_queue_->is_full())
    {
        ++waiter_number_;
        queue_cond_.wait(queue_lock_);
        --waiter_number_;
    }

    CLogger::log_thread_->inc_log_number();
    log_queue_->push_back(log_message);    
    send_signal();
}


void CLogger::log_detail(const char* filename, int lineno, const char* module_name, const char* format, ...)
{         
    if (enabled_detail())
    {
        va_list args;
        va_start(args, format);
        VaListGuard guard(args);
        
        do_log(LOG_LEVEL_DETAIL, filename, lineno, module_name, format, args);
    }
}

void CLogger::log_debug(const char* filename, int lineno, const char* module_name, const char* format, ...)
{         
    if (enabled_debug())
    {
        va_list args;
        va_start(args, format);
        VaListGuard guard(args);

        do_log(LOG_LEVEL_DEBUG, filename, lineno, module_name, format, args);
    }
}

void CLogger::log_info(const char* filename, int lineno, const char* module_name, const char* format, ...)
{         
    if (enabled_info())
    {
        va_list args;
        va_start(args, format);
        VaListGuard guard(args);

        do_log(LOG_LEVEL_INFO, filename, lineno, module_name, format, args);
    }
}

void CLogger::log_warn(const char* filename, int lineno, const char* module_name, const char* format, ...)
{         
    if (enabled_warn())
    {
        va_list args;
        va_start(args, format);
        VaListGuard guard(args);

        do_log(LOG_LEVEL_WARN, filename, lineno, module_name, format, args);
    }
}

void CLogger::log_error(const char* filename, int lineno, const char* module_name, const char* format, ...)
{         
    if (enabled_error())
    {
        va_list args;
        va_start(args, format);
        VaListGuard guard(args);

        do_log(LOG_LEVEL_ERROR, filename, lineno, module_name, format, args);
    }
}

void CLogger::log_fatal(const char* filename, int lineno, const char* module_name, const char* format, ...)
{         
    if (enabled_fatal())
    {
        va_list args;        
        va_start(args, format);
        VaListGuard guard(args);

        do_log(LOG_LEVEL_FATAL, filename, lineno, module_name, format, args);
    }
}

void CLogger::log_state(const char* filename, int lineno, const char* module_name, const char* format, ...)
{         
    if (enabled_state())
    {
        va_list args;        
        va_start(args, format);
        VaListGuard guard(args);

        do_log(LOG_LEVEL_STATE, filename, lineno, module_name, format, args);
    }
}

void CLogger::log_trace(const char* filename, int lineno, const char* module_name, const char* format, ...)
{         
    if (enabled_trace())
    {
        va_list args;
        va_start(args, format);
        VaListGuard guard(args);

        do_log(LOG_LEVEL_TRACE, filename, lineno, module_name, format, args);
    }
}

void CLogger::log_bin(const char* filename, int lineno, const char* module_name, const char* log, uint16_t size)
{
    if (enabled_bin())
    {        
        //set_log_length(log, size);
        //_log_thread->push_log(log);
    }
}

void CLogger::close_logfile()
{
    // 关闭文件句柄
    if (-1 != log_fd_)
    {        
        CLogger::log_thread_->remove_logger(this);
        close(log_fd_);
        log_fd_ = -1;        
    }
}

void CLogger::create_logfile(bool truncate)
{    
    struct stat st;
    char filename[PATH_MAX + FILENAME_MAX] = {0};
    snprintf(filename, sizeof(filename), "%s/%s", log_path_, log_filename_);

    int flags = truncate ? O_WRONLY|O_CREAT|O_TRUNC : O_WRONLY|O_CREAT|O_APPEND;
    log_fd_ = open(filename, flags, FILE_DEFAULT_PERM);

    if (-1 == log_fd_)
    {
        THROW_SYSCALL_EXCEPTION(NULL, errno, "open");
    }
	
    if (-1 == fstat(log_fd_, &st))
    {
        THROW_SYSCALL_EXCEPTION(NULL, errno, "fstat");
    }       
           
    current_bytes_ = st.st_size;
    CLogger::log_thread_->register_logger(this);    
}

void CLogger::rotate_file()
{    
    int backup_number = atomic_read(&backup_number_);
    for (uint16_t i = backup_number; 0 < i; --i)
    {
        char old_filename[PATH_MAX + FILENAME_MAX] = {0};
        char new_filename[PATH_MAX + FILENAME_MAX] = {0};

        if (1 == i)
        {
            (void)snprintf(old_filename, sizeof(old_filename), "%s/%s", log_path_, log_filename_);
        }
        else
        {
            (void)snprintf(old_filename, sizeof(old_filename), "%s/%s.%d", log_path_, log_filename_, i - 1);
        }
		
        (void)snprintf(new_filename, sizeof(new_filename), "%s/%s.%d", log_path_, log_filename_, i);

        (void)rename(old_filename, new_filename);
    }

    create_logfile(0 == backup_number);
}

bool CLogger::need_create_file() const
{
    struct stat st;
    if (-1 == fstat(log_fd_, &st))
	{
		return false;
    }

    return 0 == st.st_nlink;
}


CLogThread::CLogThread() : epoll_fd_(-1)
{
    atomic_set(&log_number_, 0);
    epoll_events_ = new struct epoll_event[LOGGER_NUMBER_MAX];
}

CLogThread::~CLogThread() throw ()
{    
    if (-1 != epoll_fd_)
    {
        close(epoll_fd_);
    }

    delete[] epoll_events_;
}

void CLogThread::run()
{
    // 提示
    fprintf(stderr, "[%s]Logger thread %u running.\n", utils::CDatetimeUtils::get_current_datetime().c_str(), get_thread_id());

#if ENABLE_SET_LOG_THREAD_NAME==1
    CUtils::set_process_name("log-thread");
#endif

    // 所有日志都写完了，才可以退出日志线程
    while (!is_stop() || (0 < get_log_number()))
    {        
        int ret = epoll_wait(epoll_fd_, epoll_events_, LOGGER_NUMBER_MAX, -1);
        if (-1 == ret)
        {
            if (EINTR == sys::Error::code())
			{
				continue;
            }
			
            THROW_SYSCALL_EXCEPTION(NULL, errno, "epoll_wait");
        }

        for (int i = 0; i < ret; ++i)
        {
            CLogProber* log_prober = static_cast<CLogProber*>(epoll_events_[i].data.ptr);
            if (!log_prober->execute())
            {
                // LogProber要么为Logger，要么为LogThread
                // 只有Logger，才会进入这里，所以强制转换成立
                CLogger* logger = static_cast<CLogger*>(log_prober);
                remove_logger(logger);                
            }
        }
    }

    // 提示
    fprintf(stderr, "[%s]Logger thread %u exited.\n", utils::CDatetimeUtils::get_current_datetime().c_str(), get_thread_id());
    remove_object(this);
}

void CLogThread::before_stop() throw (sys::CException, sys::CSysCallException)
{
    send_signal();
}

void CLogThread::before_start() throw (sys::CException, sys::CSysCallException)
{
    // 创建Epoll
    epoll_fd_ = epoll_create(LOGGER_NUMBER_MAX);
    if (-1 == epoll_fd_)
    {
        fprintf(stderr, "Logger created epoll error: %s.\n", sys::Error::to_string().c_str());
        THROW_SYSCALL_EXCEPTION(NULL, errno, "epoll_create");
    }

    // 将pipe放入epoll中
    register_object(this);
}

bool CLogThread::execute()
{
    read_signal(1);
    return true;
}

void CLogThread::remove_logger(CLogger* logger)
{
    try
    {
        if (logger->is_registered())
        {
            remove_object(logger);
            logger->set_registered(false);
        }
    }
    catch (sys::CSysCallException& ex)
    {
        // TODO
    }
}

void CLogThread::register_logger(CLogger* logger)
{
    try
    {
        register_object(logger);
        logger->set_registered(true);
    }
    catch (sys::CSysCallException& ex)
    {
        throw;
    }
}

void CLogThread::remove_object(CLogProber* log_prober)
{
    if (-1 == epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, log_prober->get_fd(), NULL))
    {
        THROW_SYSCALL_EXCEPTION(NULL, errno, "epoll_ctl");
    }
}

void CLogThread::register_object(CLogProber* log_prober)
{
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.ptr = log_prober;

    if (-1 == epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, log_prober->get_fd(), &event))
    {
        THROW_SYSCALL_EXCEPTION(NULL, errno, "epoll_ctl");
    }
}

