#include "log_appender.h"

static char log_level_name_array[][8] = { "Off", "Serious", "Error", "Info", "Debug" };

log_level_t get_log_level(const char* level_name)
{
    if (NULL == level_name) return Log_Off;

    if (0 == strcasecmp(level_name, "Serious")) return Log_Serious;
    if (0 == strcasecmp(level_name, "Error")) return Log_Error;
    if (0 == strcasecmp(level_name, "Info")) return Log_Info;
    if (0 == strcasecmp(level_name, "Debug")) return Log_Debug;
    
    return Log_Off;
}

const char* get_log_level_name(log_level_t log_level)
{
    if ((log_level < Log_Off) || (log_level > Log_Debug)) return NULL;
    return log_level_name_array[log_level];
}


LogEvent::LogEvent()
{
    memset(_content, 0, LOG_LINE_SIZE);
}

LogEvent::~LogEvent()
{
    // TODO
}

RunLogEvent::RunLogEvent(log_level_t level)
{
    // TODO
    _level = level;
}

RunLogEvent::~RunLogEvent()
{
    // TODO
}

std::string RunLogEvent::format()
{
    std::ostringstream message;

    message << "[" << get_formatted_current_datetime() << "]";
    message << "[" << Thread::get_current_thread_id() << "]";
    message << "[" << get_log_level_name(_level) << "]:";
    message << _content;
    if('\n' != _content[strlen(_content) - 1])
    {
        message << "\n";
    }

    return message.str();
}

FileAppender::FileAppender(const char* filename)
{
    _log_fd = -1;
    _log_file_seq = 0;
    memset(_log_file_name, 0, FILENAME_MAX);
    memset(_current_file_name, 0, FILENAME_MAX);
    memset(_log_path, 0, PATH_MAX);

    snprintf(_log_file_name, sizeof(_log_file_name), "%s", filename);
    
    init();
}

FileAppender::~FileAppender()
{
    // TODO
    if (_log_fd)
    {
        close(_log_fd);
        _log_fd = -1;
    }
}

void FileAppender::init()
{
    // 获取进程执行路径
    std::string work_path = Utils::get_program_path();
    
    int pos = work_path.rfind('/');
    if (0 < pos)
    {
        work_path = work_path.substr(0, pos);
    }

    snprintf(_log_path, PATH_MAX, "%s/log/", work_path.c_str());

    // 日志路径不存在则创建
    if (!DirUtils::exist(_log_path))
    {
        DirUtils::create_directory(_log_path);
    }

    for (int i = 0; i < MAX_LOG_FILE_SEQ; i++)
    {
        memset(_current_file_name, 0, FILENAME_MAX);
        snprintf(_current_file_name, FILENAME_MAX, "%s/%s.%d.log", _log_path, _log_file_name, i);
        // 文件不存在
        if (!FileUtils::exist(_current_file_name))
        {
            _log_file_seq = i;
            break;
        }

        // 最后一个文件还有
        if (MAX_LOG_FILE_SEQ - 1 == i)
        {
            _log_file_seq = 0;
            snprintf(_current_file_name, FILENAME_MAX, "%s/%s.%d.log", _log_path, _log_file_name, _log_file_seq);
            // 删除第一个文件,重新写
            FileUtils::remove(_current_file_name);
        }
    }

    create_log_file();
    
    // if (-1 == fstat(_log_fd, &st))
    // {
    //     THROW_SYSCALL_EXCEPTION(NULL, errno, "fstat");
    // }       
           
    // current_bytes_ = st.st_size;
}

void FileAppender::create_log_file()
{
    // 打开文件
    _log_fd = open(_current_file_name, O_WRONLY|O_CREAT|O_APPEND, FILE_DEFAULT_PERM);

    if (-1 == _log_fd)
    {
        // THROW_SYSCALL_EXCEPTION(NULL, errno, "open");
    }
}


void FileAppender::check_log_file()
{
    off_t logsize = FileUtils::get_file_size(_current_file_name);

    // 大于日志文件最大值
    if (MAX_LOG_FILE_SIZE <= logsize)
    {
        // 关闭日志文件
        close(_log_fd);
        _log_fd = -1;

        if (MAX_LOG_FILE_SEQ <= ++_log_file_seq)
        {
            _log_file_seq = 0;
        }

        snprintf(_current_file_name, FILENAME_MAX, "%s/%s.%d.log", _log_path, _log_file_name, _log_file_seq);
        // 如果文件存在,删除并重新打开
        if (FileUtils::exist(_current_file_name))
        {
            FileUtils::remove(_current_file_name);
        }
        
        create_log_file();
    }
}

void FileAppender::do_appender(LogEvent* event)
{
    // 日志路径不存在则创建
    if (!DirUtils::exist(_log_path))
    {
        DirUtils::create_directory(_log_path);
    }

    check_log_file();

    std::string message(event->format());
    write(_log_fd, message.c_str(), message.length());
}

