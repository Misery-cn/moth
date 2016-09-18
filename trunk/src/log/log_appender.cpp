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


CLogEvent::CLogEvent()
{
	memset(content_, 0, LOG_LINE_SIZE);
}

CLogEvent::~CLogEvent()
{
	// TODO
}

CRunLogEvent::CRunLogEvent(log_level_t level)
{
	// TODO
	level_ = level;
}

CRunLogEvent::~CRunLogEvent()
{
	// TODO
}

std::string CRunLogEvent::format()
{
	std::ostringstream message;

	message << "[" << get_formatted_current_datetime() << "]";
	message << "[" << CThread::get_current_thread_id() << "]";
	message << "[" << get_log_level_name(level_) << "]:";
	message << content_;
	if('\n' != content_[strlen(content_) - 1])
    {
        message << "\n";
    }

	return message.str();
}

CAppender::CAppender()
{
	// TODO
}

CAppender::~CAppender()
{
	// TODO
}

CFileAppender::CFileAppender(const char* filename)
{
	log_fd_ = -1;
	log_file_seq_ = 0;
	memset(log_file_name_, 0, FILENAME_MAX);
	memset(current_file_name_, 0, FILENAME_MAX);
	memset(log_path_, 0, PATH_MAX);

	snprintf(log_file_name_, sizeof(log_file_name_), "%s", filename);
	
	init();
}

CFileAppender::~CFileAppender()
{
	// TODO
	if (log_fd_)
	{
		close(log_fd_);
		log_fd_ = -1;
	}
}

void CFileAppender::init()
{
	// 获取进程执行路径
	std::string work_path = CUtils::get_program_path();
	
	int pos = work_path.rfind('/');
	if (0 < pos)
	{
		work_path = work_path.substr(0, pos);
	}

	snprintf(log_path_, PATH_MAX, "%s/log/", work_path.c_str());

	// 日志路径不存在则创建
	if (!CDirUtils::exist(log_path_))
	{
		CDirUtils::create_directory(log_path_);
	}

	for (int i = 0; i < MAX_LOG_FILE_SEQ; i++)
	{
		memset(current_file_name_, 0, FILENAME_MAX);
		snprintf(current_file_name_, FILENAME_MAX, "%s/%s.%d.log", log_path_, log_file_name_, i);
		// 文件不存在
		if (!CFileUtils::exist(current_file_name_))
		{
			log_file_seq_ = i;
			break;
		}

		// 最后一个文件还有
		if (MAX_LOG_FILE_SEQ - 1 == i)
		{
			log_file_seq_ = 0;
			snprintf(current_file_name_, FILENAME_MAX, "%s/%s.%d.log", log_path_, log_file_name_, log_file_seq_);
			// 删除第一个文件,重新写
			CFileUtils::remove(current_file_name_);
		}
	}

	create_log_file();
	
    // if (-1 == fstat(log_fd_, &st))
    // {
    //     THROW_SYSCALL_EXCEPTION(NULL, errno, "fstat");
    // }       
           
    // current_bytes_ = st.st_size;
}

void CFileAppender::create_log_file()
{
	// 打开文件
	log_fd_ = open(current_file_name_, O_WRONLY|O_CREAT|O_APPEND, FILE_DEFAULT_PERM);

    if (-1 == log_fd_)
    {
    	// THROW_SYSCALL_EXCEPTION(NULL, errno, "open");
    }
}


void CFileAppender::check_log_file()
{
	off_t logsize = CFileUtils::get_file_size(current_file_name_);

	// 大于日志文件最大值
	if (MAX_LOG_FILE_SIZE <= logsize)
	{
		// 关闭日志文件
		close(log_fd_);
		log_fd_ = -1;

		if (MAX_LOG_FILE_SEQ <= ++log_file_seq_)
        {
            log_file_seq_ = 0;
        }

		snprintf(current_file_name_, FILENAME_MAX, "%s/%s.%d.log", log_path_, log_file_name_, log_file_seq_);
		// 如果文件存在,删除并重新打开
		if (CFileUtils::exist(current_file_name_))
		{
			CFileUtils::remove(current_file_name_);
		}
		
		create_log_file();
	}
}

void CFileAppender::do_appender(CLogEvent* event)
{
	// 日志路径不存在则创建
	if (!CDirUtils::exist(log_path_))
	{
		CDirUtils::create_directory(log_path_);
	}

	check_log_file();

	std::string message(event->format());
	write(log_fd_, message.c_str(), message.length());
}

