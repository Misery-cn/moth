#include "exception.h"

CException::CException(int errcode, const char* filename, int linenumber, const char* tips) : errcode_(errcode), linenumber_(linenumber)
{
	memset(filename_, 0, MAX_FILENAME);
    strncpy(filename_, filename, sizeof(filename_)-1);
    filename_[sizeof(filename_)-1] = '\0';
	
    if (NULL != tips)
	{
		tips_ = tips;
	}
}

std::string CException::get_errmessage() const
{
    return strerror(get_errcode());
}

std::string CException::to_string() const
{
    std::stringstream ss;
    ss << "exception:"
       << get_errcode() << ":"
       << get_errmessage() << ":"
       << get_linenumber() << ":"
       << get_filename();

    return ss.str();
}

CSysCallException::CSysCallException(int errcode, const char* filename, int linenumber, const char* tips)
			: CException(errcode, filename, linenumber, "SysCallException")
{
	
}

CSysCallException::~CSysCallException()
{
	
}

std::string CSysCallException::to_string() const
{
    std::stringstream ss;
    ss << get_tips() << ":"
       << get_errcode() << ":"
       << get_errmessage() << ":"
       << get_linenumber() << ":"
       << get_filename();

    return ss.str();
}


