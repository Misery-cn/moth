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
    /**
     * 删除源字符串尾部从指定字符开始后的内容
     *
     * @param source: 源字符串
     * @param c: 分隔符
     */
    static void remove_last(std::string& source, char c);

    /**
     * 删除源字符串尾部从指定字符串开始后的内容
     *
     * @param source: 源字符串
     * @param sep: 分隔字符串
     */
    static void remove_last(std::string& source, const std::string& sep);
    
    /**
     * 将源字符串中小写字母转换成大写
     *
     * @param source: 源字符串
     */
    static void to_upper(char* source);
    static void to_upper(std::string& source);
    
    /**
     * 将源字符串中大写字母转换成小写
     *
     * @param source: 源字符串
     */
    static void to_lower(char* source);    
    static void to_lower(std::string& source);

    /**
     * 判断指定字符是否为空格或TAB符(\t)或回车符(\r)或换行符(\n)
     *
     * @param source: 源字符串
     */
    static bool is_space(char c);

    /**
     * 删除字符串首尾(不包括中间)空格或TAB符(\t)或回车符(\r)或换行符(\n)
     *
     * @param source: 源字符串
     */
    static void trim(char* source);
    static void trim(std::string& source);

    static void trim_all(char* source);
    static void trim_all(std::string& source);

    /**
     * 删除字符串头部空格或TAB符(\t)或回车符(\r)或换行符(\n)
     *
     * @param source: 源字符串
     */
    static void trim_left(char* source);
    static void trim_left(std::string& source);

    /**
     * 删除字符串尾部空格或TAB符(\t)或回车符(\r)或换行符(\n)
     *
     * @param source: 源字符串
     */
    static void trim_right(char* source);        
    static void trim_right(std::string& source);

    /**
     * 将字符串转换成8位有符号整数
     *
     * @param source: 源字符串
     * @param result: 转换后的结果
     * @param converted_length: 需要进行数字转换的字符个数，超过部分不做解析，如果取值为0则处理整个字符串
     * @param ignored_zero: 是否允许允许字符串以0打头，并自动忽略
     * @return: 如果转换成功返回true，否则返回false
     */
    static bool string2int(const char* source, int8_t& result, uint8_t converted_length = 0, bool ignored_zero = false);

    /**
     * 将字符串转换成16位有符号整数
     *
     * @param source: 源字符串
     * @param result: 转换后的结果
     * @param converted_length: 需要进行数字转换的字符个数，超过部分不做解析，如果取值为0则处理整个字符串
     * @param ignored_zero: 是否允许允许字符串以0打头，并自动忽略
     * @return: 如果转换成功返回true，否则返回false
     */
    static bool string2int(const char* source, int16_t& result, uint8_t converted_length = 0, bool ignored_zero = false);

    /**
     * 将字符串转换成32位有符号整数
     *
     * @param source: 源字符串
     * @param result: 转换后的结果
     * @param converted_length: 需要进行数字转换的字符个数，超过部分不做解析，如果取值为0则处理整个字符串
     * @param ignored_zero: 是否允许允许字符串以0打头，并自动忽略
     * @return: 如果转换成功返回true，否则返回false
     */
    static bool string2int(const char* source, int32_t& result, uint8_t converted_length = 0, bool ignored_zero = false);

    /**
     * 将字符串转换成64位有符号整数
     *
     * @param source: 源字符串
     * @param result: 转换后的结果
     * @param converted_length: 需要进行数字转换的字符个数，超过部分不做解析，如果取值为0则处理整个字符串
     * @param ignored_zero: 是否允许允许字符串以0打头，并自动忽略
     * @return: 如果转换成功返回true，否则返回false
     */
    static bool string2int(const char* source, int64_t& result, uint8_t converted_length = 0, bool ignored_zero = false);

    /**
     * 将字符串转换成8位无符号整数
     *
     * @param source: 源字符串
     * @param result: 转换后的结果
     * @param converted_length: 需要进行数字转换的字符个数，超过部分不做解析，如果取值为0则处理整个字符串
     * @param ignored_zero: 是否允许允许字符串以0打头，并自动忽略
     * @return: 如果转换成功返回true，否则返回false
     */
    static bool string2int(const char* source, uint8_t& result, uint8_t converted_length = 0, bool ignored_zero = false);

    /**
     * 将字符串转换成16位无符号整数
     *
     * @param source: 源字符串
     * @param result: 转换后的结果
     * @param converted_length: 需要进行数字转换的字符个数，超过部分不做解析，如果取值为0则处理整个字符串
     * @param ignored_zero: 是否允许允许字符串以0打头，并自动忽略
     * @return: 如果转换成功返回true，否则返回false
     */
    static bool string2int(const char* source, uint16_t& result, uint8_t converted_length = 0, bool ignored_zero = false);

    /**
     * 将字符串转换成32位无符号整数
     *
     * @param source: 源字符串
     * @param result: 转换后的结果
     * @param converted_length: 需要进行数字转换的字符个数，超过部分不做解析，如果取值为0则处理整个字符串
     * @param ignored_zero: 是否允许允许字符串以0打头，并自动忽略
     * @return: 如果转换成功返回true，否则返回false
     */
    static bool string2int(const char* source, uint32_t& result, uint8_t converted_length = 0, bool ignored_zero = false);

    /**
     * 将字符串转换成64位无符号整数
     *
     * @param source: 源字符串
     * @param result: 转换后的结果
     * @param converted_length: 需要进行数字转换的字符个数，超过部分不做解析，如果取值为0则处理整个字符串
     * @param ignored_zero: 是否允许允许字符串以0打头，并自动忽略
     * @return: 如果转换成功返回true，否则返回false
     */
    static bool string2int(const char* source, uint64_t& result, uint8_t converted_length = 0, bool ignored_zero = false);

    /**
     * 将字符串转换成整数通用模板
     *
     * @param str: 源字符串
     * @param error_value: 如果转换错误返回默认值
     * @return: 返回转换后的结果
     */
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

    /**
     * 将字符串转换成long long类型
     *
     * @param str: 源字符串
     * @param base: 采用的进制方式，如base值为10则采用10进制，若base值为16则采用16进制
     * @param result: 转换结果
     * @return: 如果转换成功返回true，否则返回false
     */
    static bool string2ll(const char* str, int base, long long& result);

    /**
     * 将字符串转换成long类型
     *
     * @param str: 源字符串
     * @param base: 采用的进制方式，如base值为10则采用10进制，若base值为16则采用16进制
     * @param result: 转换结果
     * @return: 如果转换成功返回true，否则返回false
     */
    static bool string2l(const char* str, int base, long& result);

    /**
     * 将字符串转换成double类型
     *
     * @param str: 源字符串
     * @param result: 转换结果
     * @return: 如果转换成功返回true，否则返回false
     */
    static bool string2double(const char* str, double& result);

    /**
     * 将字符串转换成float类型
     *
     * @param str: 源字符串
     * @param result: 转换结果
     * @return: 如果转换成功返回true，否则返回false
     */
    static bool string2float(const char* str, float& result);


    /**
     * 将整形转换成字符串
     *
     * @param T: 类型
     * @param base: 进制方式，默认按10进制转换
     * @param width: 转换后字符串最小长度不足补0
     * @param u: 需要转换的数字
     * @param buf: 转换结果
     * @return: 返回转换后的字符串
     */
    template<typename T, const unsigned base = 10, const unsigned width = 1>
    static inline char* ritoa(T u, char* buf)
    {
        unsigned digits = 0;
        while (u)
        {
            *--buf = "0123456789abcdef"[u % base];
            u /= base;
            digits++;
        }
        
        while (digits++ < width)
        {
            *--buf = '0';
        }
        
        return buf;
    }

    /**
     * 将16位有符号整数转换成字符串
     *
     * @param source: 16位有符号整数
     * @return: 返回转换后的字符串
     */
    static std::string int_tostring(int16_t source);

    /**
     * 将32位有符号整数转换成字符串
     *
     * @param source: 32位有符号整数
     * @return: 返回转换后的字符串
     */
    static std::string int_tostring(int32_t source);

    /**
     * 将64位有符号整数转换成字符串
     *
     * @param source: 64位有符号整数
     * @return: 返回转换后的字符串
     */
    static std::string int_tostring(int64_t source);

    /**
     * 将16位无符号整数转换成字符串
     *
     * @param source: 16位无符号整数
     * @return: 返回转换后的字符串
     */
    static std::string int_tostring(uint16_t source);

    /**
     * 将32位无符号整数转换成字符串
     *
     * @param source: 32位无符号整数
     * @return: 返回转换后的字符串
     */
    static std::string int_tostring(uint32_t source);

    /**
     * 将64位无符号整数转换成字符串
     *
     * @param source: 64位无符号整数
     * @return: 返回转换后的字符串
     */
    static std::string int_tostring(uint64_t source);

    /**
     * 跳过头部空格的字符串
     *
     * @param source: 源字符串
     * @return: 跳过头部空格的字符串
     */
    static char* skip_spaces(char* source);
    static const char* skip_spaces(const char* source);


    /**
     * 向字符串中打印数据，数据格式用户自定义
     *
     * @param str: 目的字符串
     * @param size:打印长度
     * @param format:格式化字符串
     * @return: 实际打印的长度
     */
    static int fix_snprintf(char* str, size_t size, const char* format, ...) __attribute__((format(printf, 3, 4)));
    static int fix_vsnprintf(char* str, size_t size, const char* format, va_list ap);

    /**
     * 通用类型转换成字符串
     *
     * @param any: 源字符串
     * @return: 转换后的字符串
     */
    template <typename Any>
    static std::string any2string(Any any)
    {
        std::stringstream s;
        
        s << any;
        
        return s.str();
    }

    /**
     * 获取字符在字符串中的位置
     *
     * @param str: 源字符串
     * @param c: 字符
     * @return: 转换后的字符串
     */
    static int chr_index(const char* str, char c);
    static int chr_rindex(const char* str, char c);

    /***
     * 从文件路径中提取目录路径
     *
     * @param filepath: 文件全路径
     * @return 返回提取到的目录路径
     */
    static std::string extract_dirpath(const char* filepath);

    /***
     * 从文件路径中提取文件名
     *
     * @param filepath: 文件全路径
     * @return 返回提取到文件名
     */
    static std::string extract_filename(const std::string& filepath);
    static const char* extract_filename(const char* filepath);

    /**
     * 向字符串中打印数据，格式由用户定义
     * 
     * @param format: 格式化字符串
     * @return 返回格式化后的字符串
     */
    static std::string format_string(const char* format, ...) __attribute__((format(printf, 1, 2)));

    /**
     * 判断字符是否全是纯字符a-z
     * 
     * @param str: 源字符串
     * @return 是纯字符返回true否则false
     */
    static bool is_alphabetic_string(const char* str);

    /**
     * 判断字符是否全是有效字符
     * 
     * @param str: 源字符串
     * @return 是有效字符返回true否则false
     */
    static bool is_variable_string(const char* str);

    /**
     * 判断字符是否是有效数字0-9
     * 
     * @param c: 单个字符
     * @return 是数子返回true否则false
     */
    static bool is_digit(char c);

    /**
     * 判断字符串是否是纯数字
     * 
     * @param str: 源字符串
     * @return 是数子返回true否则false
     */
    static bool is_digit_string(const char* str);

    /**
     * 删除文件后缀名
     * 
     * @param filename: 文件名字符串
     * @return 返回格式化后的字符串
     */
    static std::string remove_suffix(const std::string& filename);

    /**
     * 替换或者删除文件后缀名
     * 
     * @param filename: 文件名字符串
     * @return 返回格式化后的字符串
     */
    static std::string replace_suffix(const std::string& filepath, const std::string& new_suffix);

    /**
     * 将字符串转换为16进制字符串
     * 
     * @param source: 源字符串
     * @param lowercase: 默认小写
     * @return 返回格式化后的字符串
     */
    static std::string to_hex(const std::string& source, bool lowercase = true);

    /**
     * 删除回车符
     * 
     * @param line: 源字符串
     */
    static void trim_CR(char* line);
    static void trim_CR(std::string& line);

    /**
     * 单个字符转换成16进制
     * 
     * @param line: 源字符
     * @return: 转换后的16进制字符串
     */
    static std::string char2hex(unsigned char c);

    /**
     * 16进制字符串转换成字符
     * 
     * @param hex: 源字符串
     * @return: 转换后的字符
     */
    static unsigned char hex2char(const std::string& hex);
};

// UTILS_NS_END

#endif
