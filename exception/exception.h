#include <string>
#include <string.h>
#include <sstream>
#include "const.h"

// 所有异常继承该类
class CException
{
public:
	CException(int errcode, const char* filename, int linenumber, const char* tips = NULL);
	virtual ~CException(){}
	
	// 获取异常所在文件名
    const char* get_filename() const { return filename_; }

    // 获取异常所在行号
    int get_linenumber() const { return linenumber_; }

    // 获取errno
    int get_errcode() const { return errcode_; }

	// 获取错误信息
    std::string get_errmessage() const;

	// 获取异常提示
    const char* get_tips() const { return tips_.empty() ? NULL : tips_.c_str(); }

public:
    // 可由派生类重写
    virtual std::string to_string() const;
	
private:
	int errcode_;
	int linenumber_;
	char filename_[MAX_FILENAME];
    std::string tips_;
};

// 系统调用异常
class CSysCallException : public CException
{
public:
	CSysCallException(int errcode, const char* filename, int linenumber, const char* tips = NULL);
	virtual ~CSysCallException();
	
	virtual std::string to_string() const;
	
private:

};