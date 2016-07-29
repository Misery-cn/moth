
#ifndef _SYS_EXCEPTION_H_
#define _SYS_EXCEPTION_H_

#include <string>
#include <string.h>
#include <sstream>
#include <errno.h>
#include "const.h"

#define THROW_EXCEPTION(errmsg, errcode) \
	throw ::sys::CException(errmsg, errcode, __FILE__, __LINE__)

#define THROW_SYSCALL_EXCEPTION(errmsg, errcode, syscall) \
    throw ::sys::CSysCallException(errmsg, errcode, __FILE__, __LINE__, syscall)

namespace sys
{

// 所有异常继承该类
class CException : public std::exception
{
public:
	CException(const char* errmsg, int errcode, const char* filename, int linenum) throw ();
	virtual ~CException() throw ();

	// 重写
	virtual const char* what() const throw ();

	virtual std::string to_string() const;
	
	// 获取异常所在文件名
    const char* get_filename() const throw () { return filename_.c_str(); }

    // 获取异常所在行号
    int get_linenumber() const throw () { return linenum_; }

    // 获取errcode
    int get_errcode() const throw () { return errcode_; }

	// 获取错误信息
    std::string get_errmessage() const throw () { return errmsg_.c_str(); }
	
protected:
    std::string errmsg_;
    int errcode_;
    std::string filename_;
    int linenum_;
};



// 系统调用异常
class CSysCallException : public CException
{
public:
	CSysCallException(const char* errmsg, int errcode, const char* filename, int linenum, const char* syscall) throw ();
	virtual ~CSysCallException() throw ();
	
	virtual std::string to_string() const throw ();
	
private:
	std::string syscall_;
};

}

#endif