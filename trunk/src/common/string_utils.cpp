#include <ctype.h>
#include <limits>
#include <stdarg.h>
#include <climits>
#include "string_utils.h"
#include "tokener.h"


// UTILS_NS_BEGIN

/**
  * 快速字符串转换成整数模板通用函数
  * @param src: 需要被转换的字符串
  * @param result: 存储转换后的结果
  * @param max_length: 该整数类型对应的字符串的最多字符个数，不包括结尾符
  * @param converted_length: 需要转换的字符串长度，如果为0则表示转换整个字符串
  * @param ignored_zero: 是否忽略开头的0
  * @return: 如果转换成功返回true, 否则返回false
  */
template <typename IntType>
static bool fast_string2int(const char* str, IntType& dst, uint8_t max_length, uint8_t converted_length, bool ignored_zero)
{
    // 是否是负数
    bool negative = false;
    
    const char* tmp = str;
    
    if (NULL == str)
    {
        return false;
    }

    // 负数
    if ('-' == tmp[0])
    {
        negative = true;
        ++tmp;
    }

    if ('\0' == tmp[0])
    {
        return false;
    }

    // 以0开始的数
    if ('0' == tmp[0])
    {
        // 就是0
        if (('\0' == tmp[1]) || (1 == converted_length))
        {
            dst = 0;
            return true;
        }
        else
        {
            // 如果字符串以0开始且不希望忽略开始的0则转换失败
            if (!ignored_zero)
            {
                return false;
            }

            // 找到字符串中的一个不为0的位置
            for (;;)
            {
                ++tmp;
                
                if (tmp - str > max_length - 1)
                {
                    return false;
                }
                
                if (*tmp != '0')
                {
                    break;
                }
            }
            
            if ('\0' == *tmp)
            {
                dst = 0;
                return true;
            }
        }
    }

    // 非法数字转换失败
    if (('0' > *tmp) || ('9' < *tmp))
    {
        return false;
    }
    
    dst = (*tmp - '0');

    while ((0 == converted_length) || (tmp - str < converted_length - 1))
    {
        ++tmp;
        
        if ('\0' == *tmp)
        {
            break;
        }
        
        if (tmp - str > max_length - 1)
        {
            return false;
        }

        if (('0' > *tmp) || ('9' < *tmp))
        {
            return false;
        }

        dst = dst * 10;
        dst += (*tmp - '0');
    }

    // 负数
    if (negative)
    {
        dst = -dst;
    }
    
    return true;
}

void StringUtils::remove_last(std::string& source, char c)
{
    std::string::size_type pos = source.rfind(c);
    
    if (pos != std::string::npos)
    {
        source.erase(pos);
    }
}

void StringUtils::remove_last(std::string& source, const std::string& sep)
{
    std::string::size_type pos = source.rfind(sep);
    
    if (pos != std::string::npos)
    {
        source.erase(pos);
    }
}

void StringUtils::to_upper(char* source)
{
    char* tmp = source;
    
    while ('\0' != *tmp)
    {
        if (('a' <= *tmp) && ('z' >= *tmp))
        {
            *tmp += 'A' - 'a';
        }

        ++tmp;
    }
}

void StringUtils::to_lower(char* source)
{
    char* tmp = source;
    
    while ('\0' != *tmp)
    {
        if (('A' <= *tmp) && ('Z' >= *tmp))
        {
            *tmp += 'a' - 'A';
        }

        ++tmp;
    }
}

void StringUtils::to_upper(std::string& source)
{
    char* tmp = (char *)source.c_str();
    
    to_upper(tmp);
}

void StringUtils::to_lower(std::string& source)
{
    char* tmp = (char *)source.c_str();
    
    to_lower(tmp);
}

// 判断指定字符是否为空格或TAB符(\t)或回车符(\r)或换行符(\n)
bool StringUtils::is_space(char c)
{
    return (' ' == c) || ('\t' == c) || ('\r' == c) || ('\n' == c);
}

void StringUtils::trim(char* source)
{
    char* space = NULL;
    char* tmp = source;
    
    while (' ' == *tmp)
    {
        ++tmp;
    }

    for (;;)
    {
        *source = *tmp;
        if ('\0' == *tmp)
        {
            if (NULL != space)
            {
                *space = '\0';
            }
            
            break;
        }
        else if (is_space(*tmp))
        {
            if (NULL == space)
            {
                space = source;
            }
        }
        else
        {
            space = NULL;
        }

        ++source;
        ++tmp;
    }
}

void StringUtils::trim_left(char* source)
{
    char* tmp = source;
    
    while (is_space(*tmp)) ++tmp;

    for (;;)
    {
        *source = *tmp;
        if ('\0' == *tmp)
        {
            break;
        }

        ++source;
        ++tmp;
    }
}

void StringUtils::trim_right(char* source)
{
    char* space = NULL;
    char* tmp = source;

    for (;;)
    {
        if ('\0' == *tmp)
        {
            if (space != NULL)
            {
                *space = '\0';
            }
            
            break;
        }
        else if (is_space(*tmp))
        {
            if (NULL == space)
            {
                space = tmp;
            }
        }
        else
        {
            space = NULL;
        }

        ++tmp;
    }
}

void StringUtils::trim(std::string& source)
{
    trim_left(source);
    trim_right(source);
}

void StringUtils::trim_left(std::string& source)
{
    size_t length = source.length();
    char* tmp = new char[length + 1];
    PointGuard<char> guard(tmp, true);

    strncpy(tmp, source.c_str(), length);
    tmp[length] = '\0';

    trim_left(tmp);
    source = tmp;
}

void StringUtils::trim_right(std::string& source)
{
    size_t length = source.length();
    char* tmp = new char[length + 1];
    PointGuard<char> guard(tmp, true);

    strncpy(tmp, source.c_str(), length);
    tmp[length] = '\0';

    trim_right(tmp);
    source = tmp;
}

bool StringUtils::string2int(const char* source, int8_t& dst, uint8_t converted_length, bool ignored_zero)
{
    int16_t value = 0;

    if (!string2int(source, value, converted_length, ignored_zero))
    {
        return false;
    }
    
    if (value < std::numeric_limits<int8_t>::min() 
        || value > std::numeric_limits<int8_t>::max())
    {
        return false;
    }
    
    dst = (int8_t)value;
    
    return true;
}

bool StringUtils::string2int(const char* source, int16_t& dst, uint8_t converted_length, bool ignored_zero)
{
    int32_t value = 0;

    if (!string2int(source, value, converted_length, ignored_zero))
    {
        return false;
    }
    
    if (value < std::numeric_limits<int16_t>::min()
        || value > std::numeric_limits<int16_t>::max())
    {
        return false;
    }

    dst = (int16_t)value;
    
    return true;
}

bool StringUtils::string2int(const char* source, int32_t& dst, uint8_t converted_length, bool ignored_zero)
{
    if (NULL == source)
    {
        return false;
    }

    long value;
    // unsigned int 0~4294967295
    // int -2147483648～2147483647
    // unsigned long 0~4294967295
    // long -2147483648～2147483647
    // long long -9223372036854775808~9223372036854775807
    // unsigned long long 0~1844674407370955165
    if (!fast_string2int<long>(source, value, sizeof("-2147483648") - 1, converted_length, ignored_zero))
    {
        return false;
    }
    
    if ((value < std::numeric_limits<int32_t>::min())
        || (value > std::numeric_limits<int32_t>::max()))
    {
        return false;
    }

    dst = (int32_t)value;
    
    return true;
}

bool StringUtils::string2int(const char* source, int64_t& dst, uint8_t converted_length, bool ignored_zero)
{
    long long value;
    
    if (!fast_string2int<long long>(source, value, sizeof("-9223372036854775808") - 1, converted_length, ignored_zero))
    {
        return false;
    }

    dst = (int64_t)value;
    
    return true;
}

bool StringUtils::string2int(const char* source, uint8_t& dst, uint8_t converted_length, bool ignored_zero)
{
    uint16_t value = 0;
    
    if (!string2int(source, value, converted_length, ignored_zero))
    {
        return false;
    }
    
    if (value > std::numeric_limits<uint8_t>::max())
    {
        return false;
    }

    dst = (uint8_t)value;
    
    return true;
}

bool StringUtils::string2int(const char* source, uint16_t& dst, uint8_t converted_length, bool ignored_zero)
{
    uint32_t value = 0;
    
    if (!string2int(source, value, converted_length, ignored_zero))
    {
        return false;
    }
    
    if (value > std::numeric_limits<uint16_t>::max())
    {
        return false;
    }

    dst = (uint16_t)value;
    
    return true;
}

bool StringUtils::string2int(const char* source, uint32_t& dst, uint8_t converted_length, bool ignored_zero)
{
    unsigned long value;
    
    if (!fast_string2int<unsigned long>(source, value, sizeof("4294967295") - 1, converted_length, ignored_zero))
    {
        return false;
    }

    dst = (uint32_t)value;
    
    return true;
}

bool StringUtils::string2int(const char* source, uint64_t& dst, uint8_t converted_length, bool ignored_zero)
{
    unsigned long long value;
    
    if (!fast_string2int<unsigned long long>(source, value, sizeof("18446744073709551615") - 1, converted_length, ignored_zero))
    {
        return false;
    }

    dst = (uint64_t)value;
    
    return true;
}

bool StringUtils::string2ll(const char* str, int base, long long& result)
{
    char* endptr = NULL;
    errno = 0;
    
    long long ret = strtoll(str, &endptr, base);

    if ((errno == ERANGE && (ret == LLONG_MAX || ret == LLONG_MIN)) 
        || (0 != errno && 0 == ret))
    {
        return false;
    }

    // 不能转换
    if (endptr == str)
    {
        return false;
    }

    // 存在非数字字符
    if (*endptr != '\0')
    {
        return false;
    }

    result = ret;
    
    return true;
}

bool StringUtils::string2l(const char* str, int base, long& result)
{
    long long ret = 0;
    if (!string2ll(str, base, ret))
    {
        return false;
    }
    
    if ((INT_MIN >= ret) || (INT_MAX <= ret))
    {
        return false;
    }
    
    result = static_cast<int>(ret);

    return true;
}

bool StringUtils::string2double(const char* str, double& result)
{
    char* endptr = NULL;
    errno = 0;
    double ret = strtod(str, &endptr);
    
    if (ERANGE == errno)
    {
        return false;
    }
    
    if (endptr == str)
    {
        return false;
    }
    
    if ('\0' != *endptr)
    {
        return false;
    }

    result = ret;

    return true;
}

bool StringUtils::string2float(const char* str, float& result)
{
    char* endptr = NULL;
    errno = 0;
    float ret = strtof(str, &endptr);
    
    if (ERANGE == errno)
    {
        return false;
    }
    
    if (endptr == str)
    {
        return false;
    }
    
    if ('\0' != *endptr)
    {
        return false;
    }

    result = ret;
    
    return true;
}

std::string StringUtils::int_tostring(int16_t source)
{
    char str[sizeof("065535")];
    
    snprintf(str, sizeof(str), "%d", source);
    
    return str;
}

std::string StringUtils::int_tostring(int32_t source)
{
    char str[sizeof("04294967295")];
    
    snprintf(str, sizeof(str), "%d", source);
    
    return str;
}

std::string StringUtils::int_tostring(int64_t source)
{
    char str[sizeof("018446744073709551615")];
    // snprintf(str, sizeof(str), "%"PRId64, source);
    #if __WORDSIZE==64
        snprintf(str, sizeof(str), "%ld", source);
    #else
        snprintf(str, sizeof(str), "%lld", source);
    #endif
    
    return str;
}

std::string StringUtils::int_tostring(uint16_t source)
{
    char str[sizeof("065535")]; // 0xFFFF
    
    snprintf(str, sizeof(str), "%u", source);
    
    return str;
}

std::string StringUtils::int_tostring(uint32_t source)
{
    char str[sizeof("04294967295")]; // 0xFFFFFFFF
    
    snprintf(str, sizeof(str), "%u", source);
    
    return str;
}

std::string StringUtils::int_tostring(uint64_t source)
{
    char str[sizeof("018446744073709551615")]; // 0xFFFFFFFFFFFFFFFF
#if __WORDSIZE==64
    snprintf(str, sizeof(str), "%lu", source);
#else
    snprintf(str, sizeof(str), "%llu", source);
#endif
    return str;
}

char* StringUtils::skip_spaces(char* source)
{
    char* tmp = source;
    
    while (' ' == *tmp)
    {
        ++tmp;
    }

    return tmp;
}

const char* StringUtils::skip_spaces(const char* source)
{
    const char* tmp = source;
    
    while (' ' == *tmp)
    {
        ++tmp;
    }

    return tmp;
}

int StringUtils::fix_snprintf(char* str, size_t size, const char* format, ...)
{
    va_list ap;
    va_start(ap, format);
    int expected = fix_vsnprintf(str, size, format, ap);
    va_end(ap);

    return expected;
}

int StringUtils::fix_vsnprintf(char* str, size_t size, const char* format, va_list ap)
{
    int expected = vsnprintf(str, size, format, ap);

    if (expected < static_cast<int>(size))
    {
        return expected;
    }

    return static_cast<int>(size);
}

int StringUtils::chr_index(const char* str, char c) 
{
    const char* c_position = strchr(str, c);
    
    return (NULL == c_position) ? -1 : c_position - str;
}

int StringUtils::chr_rindex(const char* str, char c) 
{
    const char* c_position = strrchr(str, c);
    
    return (NULL == c_position) ? -1 : c_position - str;
}

std::string StringUtils::extract_dirpath(const char* filepath)
{
    std::string dirpath;
    
    int index = chr_rindex(filepath, '/');
    
    if (-1 != index)
    {
        dirpath.assign(filepath, index);
   }

    return dirpath;
}

std::string StringUtils::extract_filename(const std::string& filepath)
{
    std::string filename;
    const char* slash_position = strrchr(filepath.c_str(), '/');
    
    if (NULL == slash_position)
    {
        filename = filepath;
    }
    else
    {
        filename.assign(slash_position + 1);
    }

    return filename;
}

const char* StringUtils::extract_filename(const char* filepath)
{
    const char* slash_position = strrchr(filepath, '/');
    
    if (NULL == slash_position)
    {
        return filepath;
    }

    return slash_position + 1;
}

std::string StringUtils::format_string(const char* format, ...)
{
    va_list ap;
    size_t size = 8192;
    ScopedArray<char> buffer(new char[size]);

    for (;;)
    {
        va_start(ap, format);
        // size要求包含结束符
        int expected = vsnprintf(buffer.get(), size, format, ap);
        va_end(ap);
        
        if (expected > -1 && expected < (int)size)
            break;

        /* Else try again with more space. */
        if (expected > -1)    /* glibc 2.1 */
        {
            size = (size_t)expected + 1; /* precisely what is needed */
        }
        else           /* glibc 2.0 */
        {
            size *= 2;  /* twice the old size */
        }

        buffer.reset(new char[size]);
    }

    return buffer.get();
}

bool StringUtils::is_alphabetic_string(const char* str)
{
    const char* p = str;

    while ('\0' != *p)
    {
        if (!((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z')))
        {
            return false;
        }

        ++p;
    }

    return true;
}

bool StringUtils::is_variable_string(const char* str)
{
    const char* p = str;

    while ('\0' != *p)
    {
        if ('0' <= *p && '9' >= *p)
        {
            ++p;
            continue;
        }
        
        if (('a' <= *p && 'z' >= *p) || ('A' <= *p && 'Z' >= *p))
        {
            ++p;
            continue;
        }
        
        if (('_' == *p) || ('-' == *p))
        {
            ++p;
            continue;
        }

        return false;
    }

    return true;
}

bool StringUtils::is_digit(char c)
{
    return ('0' <= c && '9' >= c);
}

bool StringUtils::is_digit_string(const char* str)
{
    if (!str)
    {
        return false;
    }

    const char* tmp = str;

    while ('\0' != *tmp)
    {
        if (!is_digit(*tmp))
        {
            return false;
        }

        ++tmp;
    }

    return true;
}

std::string StringUtils::remove_suffix(const std::string& filename)
{
    std::string::size_type pos = filename.find('.');
    
    if (pos == std::string::npos)
    {
        return filename;
    }
    else
    {
        return filename.substr(0, pos);
    }
}

std::string StringUtils::replace_suffix(const std::string& filepath, const std::string& new_suffix)
{
    std::string::size_type pos = filepath.find('.');
    
    if (pos == std::string::npos)
    {
        if (new_suffix.empty() || new_suffix == ".")
        {
            return filepath;
        }
        else
        {
            if ('.' == new_suffix[0])
            {
                return filepath + new_suffix;
            }
            else
            {
                return filepath + std::string(".") + new_suffix;
            }
        }
    }
    else
    {
        if (new_suffix.empty() || new_suffix == ".")
        {
            return filepath.substr(0, pos);
        }
        else
        {
            if ('.' == new_suffix[0])
            {
                return filepath.substr(0, pos) + new_suffix;
            }
            else
            {
                return filepath.substr(0, pos) + std::string(".") + new_suffix;
            }
        }
    }
}

std::string StringUtils::to_hex(const std::string& source, bool lowercase)
{
    std::string hex;
    hex.resize(source.size() * 2);
    char* hex_p = const_cast<char*>(hex.data());
    
    for (std::string::size_type i = 0; i < source.size(); ++i)
    {
        // 小写
        if (lowercase)
        {
            snprintf(hex_p, 3, "%02x", source[i]);
        }
        // 大写
        else
        {
            snprintf(hex_p, 3, "%02X", source[i]);
        }

        hex_p += 2;
    }

    *hex_p = '\0';
    
    return hex;
}


void StringUtils::trim_CR(char* line)
{
    if (NULL != line)
    {
        size_t len = strlen(line);
        
        if ('\r' == line[len - 1])
        {
            line[len - 1] = '\0';
        }
    }
}

void StringUtils::trim_CR(std::string& line)
{
    std::string::size_type tail = line.size() - 1;
    
    if ('\r' == line[tail])
    {
        line.resize(tail);
    }
}

std::string StringUtils::char2hex(unsigned char c)
{
    static unsigned char hex_table[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
    std::string hex(2, '\0');

    hex[0] = hex_table[(c >> 4) & 0x0F];
    hex[1] = hex_table[c & 0x0F];
    
    return hex;
}

unsigned char StringUtils::hex2char(const std::string& hex)
{
    unsigned char c = 0;

    for (int i = 0; i < std::min<int>(hex.size(), 2); ++i)
    {
        unsigned char c1 = toupper(hex[i]); // ctype.h
        unsigned char c2 = (c1 >= 'A') ? (c1 - ('A' - 10)) : (c1 - '0');
        (c <<= 4) += c2;
    }

    return c;
}

// UTILS_NS_END
