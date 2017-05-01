#ifndef _STRING_UTILS_H_
#define _STRING_UTILS_H_

#include <math.h>
#include <sstream>
#include "int_types.h"
#include "utils.h"
#include "scoped_ptr.h"


// UTILS_NS_BEGIN

class StringUtils
{
public:
	// 删除字符串尾部从指定字符开始的内容
    static void remove_last(std::string& source, char c);
	// 删除字符串尾部从指定字符串开始的内容
    static void remove_last(std::string& source, const std::string& sep);
	
	// 将字符串中的所有小写字符转换成大写
    static void to_upper(char* source);
    static void to_upper(std::string& source);
	
	// 将字符串中的所有大写字符转换成小写
    static void to_lower(char* source);    
    static void to_lower(std::string& source);

	// 判断指定字符是否为空格或TAB符(\t)或回车符(\r)或换行符(\n)
    static bool is_space(char c);

	// 删除字符串首尾空格或TAB符(\t)或回车符(\r)或换行符(\n)
    static void trim(char* source);
    static void trim(std::string& source);

	// 删除字符串头部空格或TAB符(\t)或回车符(\r)或换行符(\n)
    static void trim_left(char* source);
    static void trim_left(std::string& source);

	// 删除字符串尾部空格或TAB符(\t)或回车符(\r)或换行符(\n)
    static void trim_right(char* source);        
    static void trim_right(std::string& source);

	// 将字符串转换成8位的有符号整数
	// converted_length:需要进行数字转换的字符个数，超过部分不做解析，如果取值为0则处理整个字符串
	// ignored_zero:是否允许允许字符串以0打头，并自动忽略
    static bool string2int(const char* source, int8_t& result, uint8_t converted_length = 0, bool ignored_zero = false);
    static bool string2int8(const char* source, int8_t& result, uint8_t converted_length = 0, bool ignored_zero = false);    

	// 将字符串转换成16位的有符号整数
	// converted_length:需要进行数字转换的字符个数，超过部分不做解析，如果取值为0则处理整个字符串
	// ignored_zero:是否允许允许字符串以0打头，并自动忽略
    static bool string2int(const char* source, int16_t& result, uint8_t converted_length = 0, bool ignored_zero = false);
	static bool string2int16(const char* source, int16_t& result, uint8_t converted_length = 0, bool ignored_zero = false);    

	// 将字符串转换成32位的有符号整数
	// converted_length:需要进行数字转换的字符个数，超过部分不做解析，如果取值为0则处理整个字符串
	// ignored_zero:是否允许允许字符串以0打头，并自动忽略
    static bool string2int(const char* source, int32_t& result, uint8_t converted_length = 0, bool ignored_zero = false);
    static bool string2int32(const char* source, int32_t& result, uint8_t converted_length = 0, bool ignored_zero = false);    

	// 将字符串转换成64位的有符号整数
	// converted_length:需要进行数字转换的字符个数，超过部分不做解析，如果取值为0则处理整个字符串
	// ignored_zero:是否允许允许字符串以0打头，并自动忽略
    static bool string2int(const char* source, int64_t& result, uint8_t converted_length = 0, bool ignored_zero = false);
    static bool string2int64(const char* source, int64_t& result, uint8_t converted_length=0, bool ignored_zero = false);    

	// 将字符串转换成8位的无符号整数
	// converted_length:需要进行数字转换的字符个数，超过部分不做解析，如果取值为0则处理整个字符串
	// ignored_zero:是否允许允许字符串以0打头，并自动忽略
    static bool string2int(const char* source, uint8_t& result, uint8_t converted_length = 0, bool ignored_zero = false);
    static bool string2uint8(const char* source, uint8_t& result, uint8_t converted_length = 0, bool ignored_zero = false);    

	// 将字符串转换成16位的无符号整数
	// converted_length:需要进行数字转换的字符个数，超过部分不做解析，如果取值为0则处理整个字符串
	// ignored_zero:是否允许允许字符串以0打头，并自动忽略
    static bool string2int(const char* source, uint16_t& result, uint8_t converted_length = 0, bool ignored_zero = false);
    static bool string2uint16(const char* source, uint16_t& result, uint8_t converted_length = 0, bool ignored_zero = false);    

	// 将字符串转换成32位的无符号整数
	// converted_length:需要进行数字转换的字符个数，超过部分不做解析，如果取值为0则处理整个字符串
	// ignored_zero:是否允许允许字符串以0打头，并自动忽略
    static bool string2int(const char* source, uint32_t& result, uint8_t converted_length = 0, bool ignored_zero = false);
    static bool string2uint32(const char* source, uint32_t& result, uint8_t converted_length = 0, bool ignored_zero = false);    

	// 将字符串转换成64位的无符号整数
	// converted_length:需要进行数字转换的字符个数，超过部分不做解析，如果取值为0则处理整个字符串
	// ignored_zero:是否允许允许字符串以0打头，并自动忽略
    static bool string2int(const char* source, uint64_t& result, uint8_t converted_length = 0, bool ignored_zero = false);
    static bool string2uint64(const char* source, uint64_t& result, uint8_t converted_length = 0, bool ignored_zero = false);    

	// 当str为非有效的IntType类型整数值时，返回error_value指定的值
    template <typename IntType>
    static IntType string2int(const char* str, IntType error_value = 0)
    {
        IntType m = 0;
        if (!string2int(str, m))
        {
            m = error_value;
        }

        return m;
    }

    template <typename IntType>
    static IntType string2int(const std::string& str, IntType error_value = 0)
    {
        IntType m = 0;
        if (!string2int(str.c_str(), m))
        {
            m = error_value;
        }

        return m;
    }

    static std::string int_tostring(int16_t source);
    static std::string int16_tostring(int16_t source);    

    static std::string int32_tostring(int32_t source);
    static std::string int_tostring(int32_t source);

    static std::string int_tostring(int64_t source);
    static std::string int64_tostring(int64_t source);

    static std::string int_tostring(uint16_t source);
    static std::string uint16_tostring(uint16_t source);    

    static std::string int_tostring(uint32_t source);
    static std::string uint32_tostring(uint32_t source);    

    static std::string int_tostring(uint64_t source);
    static std::string uint64_tostring(uint64_t source);    

	// 跳过空格
    static char* skip_spaces(char* buffer);
    static const char* skip_spaces(const char* buffer);

    static uint32_t hash(const char *str, int len);

	// 总是保证返回实际向str写入的字节数,包括结尾符,而不管size是否足够容纳
    static int fix_snprintf(char *str, size_t size, const char *format, ...) __attribute__((format(printf, 3, 4)));
    static int fix_vsnprintf(char *str, size_t size, const char *format, va_list ap);

	// 路径转换成文件名
    static std::string path2filename(const std::string& path, const std::string& join_string);

	// 万用类型转换函数
    template <typename Any>
    static std::string any2string(Any any)
    {
        std::stringstream s;
        s << any;
        return s.str();
    }

	// 将STL容器转换成字符串
    template <class ContainerClass>
    static std::string container2string(const ContainerClass& container, const std::string& join_string)
    {
        std::string str;
        typename ContainerClass::const_iterator iter = container.begin();

        for (; iter != container.end(); ++iter)
        {
            if (str.empty())
            {
                str = any2string(*iter);
            }
            else
            {
                str += join_string + any2string(*iter);
            }
        }

        return str;
    }

	// 将map容器转换成字符串
    template <class MapClass>
    static std::string map2string(const MapClass& map, const std::string& join_string)
    {
        std::string str;
        typename MapClass::const_iterator iter = map.begin();

        for (; iter!=map.end(); ++iter)
        {
            if (str.empty())
            {
                str = any2string(iter->second);
            }
            else
            {
                str += join_string + any2string(iter->second);
            }
        }

        return str;
    }

	// 返回一个字符在字符串中的位置
    static int chr_index(const char* str, char c);
    static int chr_rindex(const char* str, char c);
    
    static std::string extract_dirpath(const char* filepath);
    
    static std::string extract_filename(const std::string& filepath);

    static const char* extract_filename(const char* filepath);

    static std::string format_string(const char* format, ...) __attribute__((format(printf, 1, 2)));

    static bool is_numeric_string(const char* str);

    static bool is_alphabetic_string(const char* str);

    static bool is_variable_string(const char* str);
    
    static bool is_regex_string(const char* str);

    static std::string remove_suffix(const std::string& filename);
    
    static std::string replace_suffix(const std::string& filepath, const std::string& new_suffix);

    static std::string to_hex(const std::string& source, bool lowercase = true);

    static std::string encode_url(const std::string& url, bool space2plus = false);
    static std::string encode_url(const char* url, size_t url_length, bool space2plus = false);

    static std::string decode_url(const std::string& encoded_url);
    static std::string decode_url(const char* encoded_url, size_t encoded_url_length);

    static void trim_CR(char* line);
    static void trim_CR(std::string& line);

    static std::string char2hex(unsigned char c);
    static unsigned char hex2char(const std::string& hex);
};

// UTILS_NS_END

#endif
