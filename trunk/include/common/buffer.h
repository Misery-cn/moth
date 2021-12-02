#ifndef _BUFFER_H_
#define _BUFFER_H_


#include <list>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <string.h>
#include <sys/uio.h> // iovec
#include <limits.h> // IOV_MAX
#include <sys/mman.h>
//#include <type_traits>
//#include <inttypes.h>
#include <stdlib.h> // size_t ssize_t

#include "page.h"

class raw;

/**
 * 申请内存
 */
raw* create(uint32_t len);

raw* create_aligned(uint32_t len, uint32_t align);

raw* create_page_aligned(uint32_t len);

raw* copy(const char* c, uint32_t len);


// raw部分数据段
class ptr
{
public:
    ptr() : _raw(NULL), _off(0), _len(0)
    {
        // TODO
    }

    ptr(raw* r);
    
    ptr(uint32_t len);

    /**
     * 拷贝构造
     *
     */
    ptr(const ptr& p);

    ptr(ptr&& p);

    ptr(const ptr& p, uint32_t offset, uint32_t len);

    /**
     * 重载赋值运算符
     */
    ptr& operator=(const ptr& other);

    ptr& operator=(ptr&& other);

    virtual ~ptr()
    {
        release();
    }

    /**
     * 添加字符串到ptr
     *
     * @param p: 源字符串
     * @param len: 字符串长度
     * @return: 当前ptr长度
     */
    uint32_t append(const char* p, uint32_t len);

    /**
     * 添加一个字符到ptr
     *
     * @param c: 源字符
     * @return: 当前ptr长度
     */
    uint32_t append(char c);

    /**
     * 转换成char*
     *
     * @return: 返回保存的字符串
     */
    char* c_str();

    /**
     * 转换成const char*
     *
     * @return: 返回保存的字符串
     */
    const char* c_str() const;

    const char* end_c_str() const;

    /**
     * 重载运算符
     *
     * 根据下标返回字符
     */
    const char& operator[](uint32_t n) const;

    char& operator[](uint32_t n);

    /**
     * 获取长度
     */
    uint32_t length() const { return _len; }


    /**
     * 获取ptr内部偏移
     */
    uint32_t offset() const { return _off; }

    /**
     * 获取ptr开始地址
     */
    uint32_t start() const { return _off; }

    /**
     * 获取ptr结束地址
     */
    uint32_t end() const { return _off + _len; }

    /**
     * 获取未使用的内存段长度
     */
    uint32_t unused_tail_length() const;

    void set_length(uint32_t l)
    {
        _len = l;
    }

    ptr& make_shareable();

    raw* get_raw() const
    {
        return _raw;
    }

    void copy_in(uint32_t o, uint32_t l, const char* src);

    void copy_in(uint32_t o, uint32_t l, const char* src, bool crc_reset);

    void copy_out(uint32_t o, uint32_t l, char* dest) const;

    void swap(ptr& other);

private:

    void release();

public:
    class iterator
    {
    public:
        iterator(const ptr* p, size_t offset, bool d) : _ptr(p), _begin(p->c_str() + offset), _end(p->end_c_str()),
            _pos(_begin), _deep(d)
        {
        }

        const char* get_pos()
        {
            return _pos;
        }

        const char* get_end()
        {
            return _end;
        }

        size_t get_offset()
        {
            return _pos - _begin;
        }
        
    private:
        const ptr* _ptr;
        const char* _begin;
        const char* _end;
        const char* _pos;
        bool _deep;
    };


private:
    // 已申请的内存对象
    raw* _raw;
    // 偏移
    uint32_t _off;
    // 当前长度
    uint32_t _len;
};

class buffer
{
public:
    
    buffer() : _len(0), _memcopy_count(0), _last_p(this) {}

    /**
     * 构造函数
     *
     * @param prealloc: 预分配长度
     *
     */
    buffer(uint32_t prealloc) : _len(0), _memcopy_count(0), _last_p(this)
    {
        reserve(prealloc);
    }

    buffer(const buffer& other) : _ptrs(other._ptrs), _len(other._len), _memcopy_count(other._memcopy_count), _last_p(this)
    {
        make_shareable();
    }

    /**
     * 移动构造函数
     *
     */
    buffer(buffer&& other);
    
    buffer& operator=(const buffer& other)
    {
        if (this != &other)
        {
            _ptrs = other._ptrs;
            _len = other._len;
            make_shareable();
        }
        
        return *this;
    }

    buffer& operator=(buffer&& other)
    {
        _ptrs = std::move(other._ptrs);
        _len = other._len;
        _memcopy_count = other._memcopy_count;
        _last_p = begin();
        _append_ptr.swap(other._append_ptr);
        other.clear();
        return *this;
    }

    // 申明iterator
    class iterator;

private:

    template<bool is_const>
    class iterator_impl : public std::iterator<std::forward_iterator_tag, char>
    {
    protected:
        typedef typename std::conditional<is_const, const buffer, buffer>::type buf_t;

        typedef typename std::conditional<is_const, const std::list<ptr>, std::list<ptr>>::type bufs_t;

        typedef typename std::conditional<is_const, typename std::list<ptr>::const_iterator, typename std::list<ptr>::iterator>::type bufs_iter_t;

        // buffer对象
        buf_t* _buffer;
        // buffer中的prt列表
        bufs_t* _ptrs;
        bufs_iter_t _iter;
        uint32_t _offset;
        uint32_t _p_offset;

    public:
        
        iterator_impl() : _buffer(NULL), _ptrs(NULL), _offset(0), _p_offset(0)
        {
        }

        iterator_impl(buf_t* buf, uint32_t offset = 0);

        iterator_impl(buf_t* buf, uint32_t offset, bufs_iter_t iter, uint32_t p_offset) : _buffer(buf), 
            _ptrs(&(_buffer->_ptrs)), _offset(offset), _p_offset(p_offset), _iter(iter)
        {
        }

        iterator_impl(const buffer::iterator& iter);

        /**
         * 增加指定偏移量
         *
         */
        void advance(ssize_t offset);

        ptr get_current_ptr() const;

        void seek(size_t offset);

        buffer& get_buffer() const { return *_buffer; }

        uint32_t get_off() const { return _offset; }
        
        unsigned get_remaining() const { return _ptrs->length() - _offset; }

        bool end() const
        {
            return _iter == _ptrs->end();
            //return _off == _ptrs->length();
        }

        void copy(uint32_t len, char* dest);

        void copy(uint32_t len, ptr& dest);

        void copy(uint32_t len, buffer& dest);

        void copy(uint32_t len, std::string& dest);

        void copy_all(buffer& dest);

        friend bool operator ==(const iterator_impl& lhs, const iterator_impl& rhs)
        {
            return &lhs.get_buffer() == &rhs.get_buffer() && lhs.get_off() == rhs.get_off();
        }

        friend bool operator !=(const iterator_impl& lhs, const iterator_impl& rhs)
        {
            return &lhs.get_buffer() != &rhs.get_buffer() || lhs.get_off() != rhs.get_off();
        }
        
    };

public:
    class iterator : public iterator_impl<false>
    {
    public:
        iterator()
        {
        }

        iterator(buf_t* buf, uint32_t offset = 0);

        iterator(buf_t* buf, uint32_t offset, bufs_iter_t iter, uint32_t p_offset);

        ptr get_current_ptr();

        void copy(uint32_t len, char* dest);

        void copy(uint32_t len, ptr& dest);
        void copy(uint32_t len, buffer& dest);
        void copy(uint32_t len, std::string& dest);
        void copy_all(buffer& dest);

        void copy_in(uint32_t len, const char* src);
        void copy_in(uint32_t len, const char* src, bool crc_reset);
        void copy_in(uint32_t len, const buffer& otherl);

        bool operator==(const iterator& rhs) const
        {
            return _buffer == rhs._buffer && _offset == rhs._offset;
        }
        
        bool operator!=(const iterator& rhs) const
        {
            return _buffer != rhs._buffer || _offset != rhs._offset;
        }
    };

public:
    
    iterator begin()
    {
        return iterator(this, 0);
    }

    iterator end()
    {
        return iterator(this, _len, _ptrs.end(), 0);
    }


public:
    
    /**
     * 预留内存大小
     *
     * @param prealloc: 预分配长度
     *
     */
    void reserve(uint32_t prealloc)
    {
        if (_append_ptr.unused_tail_length() < prealloc)
        {
            _append_ptr = create(prealloc);
            _append_ptr.set_length(0);
        }
    }

    void make_shareable()
    {
        std::list<ptr>::iterator iter;
        for (iter = _ptrs.begin(); iter != _ptrs.end(); ++iter)
        {
            (void)iter->make_shareable();
        }
    }

    const static unsigned int CLAIM_DEFAULT = 0;
    const static unsigned int CLAIM_ALLOW_NONSHAREABLE = 1;

    /**
     * 根据buf初始化本对象
     *
     */
    void claim(buffer& buf, unsigned int flags = CLAIM_DEFAULT);

    /**
     * 追加buf至本对象
     *
     */
    void claim_append(buffer& buf, unsigned int flags = CLAIM_DEFAULT);

    uint32_t length() const { return _len; }

    // void set_length(const uint32_t len) { _len = len; }

    void clear()
    {
        _ptrs.clear();
        _len = 0;
        _memcopy_count = 0;
        _last_p = begin();
        _append_ptr = ptr();
    }

    /**
     * 向buffer中追加字符或字符串
     *
     */
    void append(char c);
    void append(const char* data, uint32_t len);
    void append(const std::string& s)
    {
        append(s.data(), s.length());
    }
    void append(const ptr& p);
    void append(ptr&& p);
    void append(const ptr& p, uint32_t off, uint32_t len);
    void append(const buffer& buf);
    void append_zero(uint32_t len);

    /**
     * 共享buf的数据
     *
     */
    void share(const buffer& buf)
    {
        if (this != &buf)
        {
            clear();
            
            std::list<ptr>::const_iterator it;
            for (it = buf._ptrs.begin(); it != buf._ptrs.end(); ++it)
            {
                  push_back(*it);
            }
        }
    }

    void push_back(const ptr& p);

    const std::list<ptr>& ptrs() const { return _ptrs; }

    uint32_t crc32(uint32_t crc) const;

    char* c_str();

    void rebuild();

    void rebuild(ptr& nb);

    void invalidate_crc();

    uint32_t get_num_buffers() const { return _ptrs.size(); }

    const ptr& front() const { return _ptrs.front(); }

    const ptr& back() const { return _ptrs.back(); }

private:
    // ptr列表
    std::list<ptr> _ptrs;
    // buffer长度
    uint32_t _len;
    uint32_t _memcopy_count;
    // 用于追加内容的内存段
    ptr _append_ptr;
    mutable iterator _last_p;
};

#endif
