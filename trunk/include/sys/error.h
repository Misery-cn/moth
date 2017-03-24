#ifndef _SYS_ERROR_H_
#define _SYS_ERROR_H_

#include <errno.h>
#include <string.h>
#include "define.h"

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
    return strerror(errcode);
}

inline bool is_not(int errcode)
{
    return errno != errcode;
}
}

// SYS_NS_END

#endif
