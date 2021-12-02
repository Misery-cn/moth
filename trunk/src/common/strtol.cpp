#include <cerrno>
#include <climits>
#include <cstdlib>
#include <sstream>
#include "strtol.h"


long long strict_strtoll(const char* str, int base, std::string* err)
{
    char* endptr;
    std::string errStr;
    errno = 0;
    long long ret = strtoll(str, &endptr, base);

    if ((errno == ERANGE && (ret == LLONG_MAX || ret == LLONG_MIN)) || (0 != errno && 0 == ret))
    {
        errStr = "The option value '";
        errStr.append(str);
        errStr.append("'");
        errStr.append(" seems to be invalid");
        *err = errStr;
        return 0;
    }
    
    if (endptr == str)
    {
        errStr = "Expected option value to be integer, got '";
        errStr.append(str);
        errStr.append("'");
        *err =  errStr;
        return 0;
    }
    
    if (*endptr != '\0')
    {
        errStr = "The option value '";
        errStr.append(str);
        errStr.append("'");
        errStr.append(" seems to be invalid");
        *err =  errStr;
        return 0;
    }
    
    *err = "";
    return ret;
}

int strict_strtol(const char* str, int base, std::string* err)
{
    std::string errStr;
    long long ret = strict_strtoll(str, base, err);
    if (!err->empty())
    {
        return 0;
    }
    
    if ((ret <= INT_MIN) || (ret >= INT_MAX))
    {
        errStr = "The option value '";
        errStr.append(str);
        errStr.append("'");
        errStr.append(" seems to be invalid");
        *err = errStr;
        return 0;
    }
    
    return static_cast<int>(ret);
}

double strict_strtod(const char* str, std::string* err)
{
    char *endptr;
    errno = 0;
    double ret = strtod(str, &endptr);
    
    if (errno == ERANGE)
    {
        std::ostringstream oss;
        oss << "strict_strtod: floating point overflow or underflow parsing '"
            << str << "'";
        *err = oss.str();
        return 0.0;
    }
    
    if (endptr == str)
    {
        std::ostringstream oss;
        oss << "strict_strtod: expected double, got: '" << str << "'";
        *err = oss.str();
        return 0;
    }
    
    if (*endptr != '\0')
    {
        std::ostringstream oss;
        oss << "strict_strtod: garbage at end of string. got: '" << str << "'";
        *err = oss.str();
        return 0;
    }
    
    *err = "";
    return ret;
}

float strict_strtof(const char* str, std::string* err)
{
    char *endptr;
    errno = 0;
    float ret = strtof(str, &endptr);
    if (errno == ERANGE)
    {
        std::ostringstream oss;
        oss << "strict_strtof: floating point overflow or underflow parsing '"
            << str << "'";
        *err = oss.str();
        return 0.0;
    }
    
    if (endptr == str)
    {
        std::ostringstream oss;
        oss << "strict_strtof: expected float, got: '" << str << "'";
        *err = oss.str();
        return 0;
    }
    
    if (*endptr != '\0')
    {
        std::ostringstream oss;
        oss << "strict_strtof: garbage at end of string. got: '" << str << "'";
        *err = oss.str();
        return 0;
    }
    
    *err = "";
    return ret;
}

template<typename T>
T strict_si_cast(const char* str, std::string* err)
{
    std::string s(str);
    if (s.empty())
    {
        *err = "strict_sistrtoll: value not specified";
        return 0;
    }
    
    const char &u = s.back();
    int m = 0;
    if ('B' == u)
    {
        m = 0;
    }
    else if ('K' == u)
    {
        m = 10;
    }
    else if ('M' == u)
    {
        m = 20;
    }
    else if ('G' == u)
    {
        m = 30;
    }
    else if ('T' == u)
    {
        m = 40;
    }
    else if ('P' == u)
    {
        m = 50;
    }
    else if ('E' == u)
    {
        m = 60;
    }
    else
    {
        m = -1;
    }

    if (0 <= m)
    {
        s.pop_back();
    }
    else
    {
        m = 0;
    }

    long long ll = strict_strtoll(s.c_str(), 10, err);
    if (0 > ll && !std::numeric_limits<T>::is_signed)
    {
        *err = "strict_sistrtoll: value should not be negative";
        return 0;
    }
    
    if (static_cast<unsigned>(m) >= sizeof(T) * CHAR_BIT)
    {
        *err = ("strict_sistrtoll: the SI prefix is too large for the designated type");
        return 0;
    }
    
    using promoted_t = typename std::common_type<decltype(ll), T>::type;
    if (static_cast<promoted_t>(ll) < static_cast<promoted_t>(std::numeric_limits<T>::min()) >> m)
    {
        *err = "strict_sistrtoll: value seems to be too small";
        return 0;
    }
    
    if (static_cast<promoted_t>(ll) > static_cast<promoted_t>(std::numeric_limits<T>::max()) >> m)
    {
        *err = "strict_sistrtoll: value seems to be too large";
        return 0;
    }
    
    return (ll << m);
}

template int strict_si_cast<int>(const char* str, std::string* err);
template long strict_si_cast<long>(const char* str, std::string* err);
template long long strict_si_cast<long long>(const char* str, std::string* err);
template uint64_t strict_si_cast<uint64_t>(const char* str, std::string* err);
template uint32_t strict_si_cast<uint32_t>(const char* str, std::string* err);

uint64_t strict_sistrtoll(const char* str, std::string* err)
{
    return strict_si_cast<uint64_t>(str, err);
}

