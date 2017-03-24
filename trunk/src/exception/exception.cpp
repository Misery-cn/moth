#include "exception.h"

// SYS_NS_BEGIN


CException::CException(const char* errmsg, int errcode, const char* filename, int linenum) throw ()
				: _errmsg(errmsg), _errcode(errcode), _filename(filename), _linenum(linenum)
{

}

CException::~CException() throw ()
{

}

const char* CException::what() const throw ()
{
	return _errmsg.c_str();
}


std::string CException::to_string() const
{
    std::stringstream ss;
    ss << "exception:"
       << _errcode << "-"
       << _errmsg<< "-"
       << _filename<< "-"
       << _linenum;

    return ss.str();
}

CSysCallException::CSysCallException(const char* errmsg, int errcode, const char* filename, int linenum, const char* syscall) throw ()
			: CException(errmsg, errcode, filename, linenum)
{
	_errmsg = strerror(errno);
	_syscall = syscall;
}

CSysCallException::~CSysCallException() throw ()
{
	
}

std::string CSysCallException::to_string() const throw ()
{
    std::stringstream ss;
    ss << "SysCallException" << "-"
       << _errcode << "-"
       << _errmsg<< "-"
       << _filename << "-"
       << _linenum << "-"
       << _syscall;

    return ss.str();
}

// SYS_NS_END
