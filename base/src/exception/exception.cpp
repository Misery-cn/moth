#include "exception.h"

SYS_NS_BEGIN


CException::CException(const char* errmsg, int errcode, const char* filename, int linenum) throw ()
				: errmsg_(errmsg), errcode_(errcode), filename_(filename), linenum_(linenum)
{

}

CException::~CException() throw ()
{

}

const char* CException::what() const throw ()
{
	return errmsg_.c_str();
}


std::string CException::to_string() const
{
    std::stringstream ss;
    ss << "exception:"
       << errcode_ << "-"
       << errmsg_<< "-"
       << filename_<< "-"
       << linenum_;

    return ss.str();
}

CSysCallException::CSysCallException(const char* errmsg, int errcode, const char* filename, int linenum, const char* syscall) throw ()
			: CException(errmsg, errcode, filename, linenum)
{
	errmsg_ = strerror(errno);
	syscall_ = syscall;
}

CSysCallException::~CSysCallException() throw ()
{
	
}

std::string CSysCallException::to_string() const throw ()
{
    std::stringstream ss;
    ss << "SysCallException" << "-"
       << errcode_ << "-"
       << errmsg_<< "-"
       << filename_ << "-"
       << linenum_ << "-"
       << syscall_;

    return ss.str();
}

SYS_NS_END

