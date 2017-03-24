#ifndef _UTILS_H_
#define _UTILS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <vector>
#include <sys/types.h>
#include <unistd.h>
#include "const.h"
#include "exception.h"
#include "error.h"

// UTILS_NS_BEGIN


template<typename T>
class CPointGuard
{
public:
    CPointGuard(T* p, bool is_arrary = false);
    virtual ~CPointGuard();
private:
    T* _point;
	bool _is_arrary;
};

template<typename T>
CPointGuard<T>::CPointGuard(T * p, bool is_arrary)
{
	_point = p;
	_is_arrary = is_arrary;
}

template<typename T>
CPointGuard<T>::~CPointGuard()
{
	if (!_is_arrary)
	{
		delete _point;
	}
	else
	{
		delete[] _point;
	}
	
	_point = NULL;
}

// 类类型close助手函数，要求该类有公有的close方法
template <typename T>
class CloseHelper
{
public:
    CloseHelper(T* obj) : _obj(obj)
    {
    }

    ~CloseHelper()
    {
        if (NULL != _obj)
        {
            _obj->close();
        }
    }

    T* operator->()
    {
        return _obj;
    }

    T* release()
    {
        T* obj = _obj;
        _obj = NULL;
        return obj;
    }

private:
    T* _obj;
};

template <>
class CloseHelper<int>
{
public:
    CloseHelper<int>(int fd) : _fd(fd)
    {
    }
    
    ~CloseHelper<int>()
    {
        if (-1 != _fd)
        {
            ::close(_fd);
        }
    }

    operator int() const
    {
        return _fd;
    }

    int get() const
    {
        return _fd;
    }

    int release()
    {
        int fd = _fd;
        _fd = -1;
        return fd;
    }

private:
    int _fd;
};
 

template <>
class CloseHelper<FILE*>
{
public:
    CloseHelper<FILE*>(FILE* fp) : _fd(fp)
    {
    }
    
    ~CloseHelper<FILE*>()
    {
        if (NULL != _fd)
        {
            fclose(_fd);
        }
    }

    operator FILE*() const
    {
        return _fd;
    }

    FILE* release()
    {
        FILE* fp = _fd;
        _fd = NULL;
        return fp;
    }
 
private:
    FILE* _fd;
};

template <typename T>
class ReleaseHelper
{
public:
    ReleaseHelper(T* obj) : _obj(obj)
    {
    }

    ~ReleaseHelper()
    {
        _obj->release();
    }

    T* operator->()
    {
        return _obj;
    }

private:
    T* _obj;
};


template <typename DataType>
class CArrayList
{       
public:
    CArrayList(uint32_t list_max) : _head(0), _tail(0), _list_size(0)
    {
        _list_max = list_max;
        if (0 == _list_max)
        {
            _items = NULL;
        }
        else
        {
            _items = new DataType[_list_max];        
            memset(_items, 0, _list_max);
        }
    }

    ~CArrayList()
    {
        delete []_items;
    }

	
    bool is_full() const 
    {
        return (_list_max == _list_size);
    }
    
    bool is_empty() const 
    {
        return (0 == _list_size);
    }

    DataType front() const 
    {
        return _items[_head];
    }
    
    DataType pop_front() 
    {
        DataType item = _items[_head];
        _head = (_head + 1) % _list_max;
        --_list_size;
        return item;
    }

    void push_back(DataType item) 
    {
        _items[_tail] = item;
        _tail = (_tail + 1) % _list_max; 
        ++_list_size;
    }

    uint32_t size() const 
    {
        return _list_size;
    }

	DataType& operator[](uint32_t pos)
	{
		if (_list_size < pos)
		{
			return _items[_list_size];
		}

		return _items[pos];
	}
    
    uint32_t capacity() const
    {
        return _list_max;
    }

private:
	volatile uint32_t _head;
    volatile uint32_t _tail;
    volatile uint32_t _list_size;
    uint32_t _list_max;
    DataType* _items;
};


class CUtils
{
public:
	// 毫秒级sleep函数
    static void millisleep(uint32_t millisecond);

    // 获取指定系统调用错误码的字符串错误信息
    static std::string get_error_message(int errcode);

    // 得到最近一次的出错信息
    static std::string get_last_error_message();

    // 得到最近一次的出错代码
    static int get_last_error_code();

    // 得到当前进程号
    static int get_current_process_id();

    // 得到当前进程所属可执行文件所在的绝对路径，结尾符不含反斜杠
    static std::string get_program_path();
    
    // 得到与指定fd相对应的文件名，包括路径部分
	static std::string get_filename(int fd);

    // 得到一个目录的绝对路径，路径中不会包含../和./等，是一个完整的原始路径
    static std::string get_full_directory(const char* directory);

    // 得到CPU核个数
	static uint16_t get_cpu_number();    

	// 得到当前调用栈;注意事项: 编译源代码时带上-rdynamic和-g选项，否则可能看到的是函数地址，而不是函数符号名称
    static bool get_backtrace(std::string& call_stack);

	// 得到指定目录字节数大小，非线程安全函数，同一时刻只能被一个线程调用
    static off_t du(const char* dirpath);

    // 得到内存页大小
    static int get_page_size();

	// 得到一个进程可持有的最多文件(包括套接字等)句柄数
    static int get_fd_max();

	// 判断指定fd对应的是否为文件
    static bool is_file(int fd);
	// 判断指定Path是否为一个文件
    static bool is_file(const char* path);
	// 判断指定fd对应的是否为软链接
    static bool is_link(int fd);
	// 判断指定Path是否为一个软链接
    static bool is_link(const char* path);
	// 判断指定fd对应的是否为目录
    static bool is_directory(int fd);
	// 判断指定Path是否为一个目录
    static bool is_directory(const char* path);
    
	// 是否允许当前进程生成coredump文件
    static void enable_core_dump(bool enabled=true, int core_file_size=-1);

	// 得到当前进程程序的名称，结果和main函数的argv[0]相同
    static std::string get_program_long_name();

	// 得到当前进程的的名称，不包含目录部分，如“./abc.exe”值为“abc.exe”
	// 如果调用了set_process_title()，
	// 则通过program_invocation_short_name可能取不到预期的值，甚至返回的是空
    static std::string get_program_short_name();


	// 取路径的文件名部分，结果包含后缀部分
    static std::string get_filename(const std::string& filepath);

	// 取路径的目录部分，不包含文件名部分，并保证不以反斜杠结尾
    static std::string get_dirpath(const std::string& filepath);

	// 设置进程名
    static void set_process_name(const std::string& new_name);
    static void set_process_name(const char* format, ...);

	// 设置进程标题，ps命令看到的结果，必须先调用init_program_title()后，才可以调用set_program_title()
    static void init_process_title(int argc, char *argv[]);    
    static void set_process_title(const std::string& new_title);
    static void set_process_title(const char* format, ...);

	// 通用的pipe读取操作
	// 读取方法为：先读一个4字节的长度，然后根据长度读取内容
	// fd pipe的句柄
	// buffer 存储从pipe中读取的数据，注意调用者使用后必须调用delete []buffer以释放内存
	// buffer_size 存储从pipe中读取到的数据字节数
    static void common_pipe_read(int fd, char** buffer, int32_t* buffer_size);

	// 通用的pipe写操作
	// 读取方法为：先写一个4字节的长度buffer_size，然后根据长度buffer_size写入内容
	// fd pipe的句柄
	// buffer 需要写入pipe的内容
	// buffer_size 需要写入的字节数
    static void common_pipe_write(int fd, const char* buffer, int32_t buffer_size);

    // 取随机数
    template <typename T>
    static T get_random_number(unsigned int i, T max_number)
    {
        struct timeval tv;
        struct timezone *tz = NULL;

        gettimeofday(&tv, tz);
        srandom(tv.tv_usec + i); // 加入i，以解决过快时tv_usec值相同

        // RAND_MAX 类似于INT_MAX
        return static_cast<T>(random() % max_number);
    }

    // 将一个vector随机化
    template <typename T>
    static void randomize_vector(std::vector<T>& vec)
    {
        unsigned int j = 0;
        std::vector<T> tmp;

        while (!vec.empty())
        {
            typename std::vector<T>::size_type max_size = vec.size();
            typename std::vector<T>::size_type random_number = CUtils::get_random_number(j, max_size);

            typename std::vector<T>::iterator iter = vec.begin() + random_number;
            tmp.push_back(*iter);
            vec.erase(iter);
            ++j;
        }

        vec.swap(tmp);
    }
};

// UTILS_NS_END

#endif
