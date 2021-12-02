#include "exception.h"

// SYS_NS_BEGIN


Exception::Exception(const char* errmsg, int errcode, const char* filename, int linenum) throw ()
                : _errmsg(errmsg), _errcode(errcode), _filename(filename), _linenum(linenum)
{

}

Exception::~Exception() throw ()
{

}

const char* Exception::what() const throw ()
{
    return _errmsg.c_str();
}


std::string Exception::to_string() const
{
    std::stringstream ss;
    ss << "exception:"
       << _errcode << "-"
       << _errmsg<< "-"
       << _filename<< "-"
       << _linenum;

    return ss.str();
}

SysCallException::SysCallException(const char* errmsg, int errcode, const char* filename, int linenum, const char* syscall) throw ()
            : Exception(errmsg, errcode, filename, linenum)
{
    _errmsg = strerror(errno);
    _syscall = syscall;
}

SysCallException::~SysCallException() throw ()
{
    
}

std::string SysCallException::to_string() const throw ()
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
