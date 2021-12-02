#ifndef _SYS_ERROR_H_
#define _SYS_ERROR_H_

#include <sstream>
#include <errno.h>
#include <string.h>

// SYS_NS_BEGIN

class ErrorKeeper
{
public:
    ErrorKeeper() : _errcode(errno)
    {
    }

    ErrorKeeper(int errcode) : _errcode(errcode)
    {
    }

    ~ErrorKeeper()
    {
        errno = _errcode;
    }

    operator int() const
    {
        return _errcode;
    }

private:
    int _errcode;
};

namespace Error
{
    inline int code()
    {
        return errno;
    }

    inline void set(int errcode)
    {
        errno = errcode;
    }

    inline std::string to_string()
    {
        return strerror(errno);
    }

    inline std::string to_string(int errcode)
    {
        char buf[256] = {0};
        char* errmsg = NULL;

        if (0 > errcode)
        {
            errcode = -errcode;
        }

        std::ostringstream oss;

    #ifdef STRERROR_R_CHAR_P
        errmsg = strerror_r(errcode, buf, sizeof(buf));
    #else
        strerror_r(errcode, buf, sizeof(buf));
        errmsg = buf;
    #endif

        oss << "(" << errcode << ") " << errmsg;
    
        return oss.str();
    }

    inline bool is_not(int errcode)
    {
        return errno != errcode;
    }
}

// SYS_NS_END

#endif
