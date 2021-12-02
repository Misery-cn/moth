#include <errno.h>
#include <unistd.h> // pipe
#include <fcntl.h>
#include <sys/stat.h>
// #include <fstream>
// #include <sstream>
#include <iostream>
#include <iomanip>
#include "buffer.h"
#include "atomic.h"
#include "spinlock.h"
#include "env.h"
#include "intarith.h"
#include "builtin.h"
#include "armor.h"
#include "safe_io.h"
#include "crc32.h"
#include "error.h"
#include "string_utils.h"


#define BUFFER_ALLOC_UNIT  (MIN(PAGE_SIZE, 4096))
#define BUFFER_APPEND_SIZE (BUFFER_ALLOC_UNIT - sizeof(raw_combined))

// 原始数据buf
class raw
{
public:
    /**
     * 构造函数
     *
     * @param len: 申请内存大小
     */
    explicit raw(uint32_t len) : _data(NULL), _len(len), _ref(0)
    {
    }

    raw(char* c, uint32_t l) : _data(c), _len(l), _ref(0)
    {
    }

    virtual ~raw()
    {
        // TODO
    }

    /**
     * 是否页对齐
     *
     * @param len: 申请内存大小
     */
    virtual bool is_page_aligned()
    {
        return (0 == ((long)_data & ~PAGE_SIZE));
    }

    /**
     * 获取数据字符串
     *
     * @return: 返回保存的字符串
     */
    virtual char* get_data()
    {
        return _data;
    }

    /**
     * 是否可共享
     *
     * @return: 可共享返回true,否则false
     */
    virtual bool is_shareable()
    {
        return true;
    }

    /**
     * 用于申请一段内存,由派生类实现
     *
     * @return: 返回新的对象
     */
    virtual raw* clone_empty() = 0;

    /**
     * 深拷贝数据
     *
     * @return: 返回拷贝后的对象
     */
    raw* clone()
    {
        raw* c = clone_empty();
        memcpy(c->_data, _data, _len);
        return c;
    }

    /**
     * 获取一段数据的crc值
     *
     * @return: 如果有该段的crc信息返回true，否则false
     */
    bool get_crc(const std::pair<size_t, size_t>& fromto, std::pair<uint32_t, uint32_t>* crc) const
    {
        SpinLock::Locker locker(_crc_spinlock);
        
        std::map<std::pair<size_t, size_t>, std::pair<uint32_t, uint32_t> >::const_iterator iter = _crc_map.find(fromto);
        
        if (iter == _crc_map.end())
        {
            return false;
        }
        
        *crc = iter->second;
        
        return true;
    }

    
    /**
     * 保存一段数据的crc值
     *
     *
     */
    void set_crc(const std::pair<size_t, size_t>& fromto, const std::pair<uint32_t, uint32_t>& crc)
    {
        SpinLock::Locker locker(_crc_spinlock);
        
        _crc_map[fromto] = crc;
    }

    void invalidate_crc()
    {
        SpinLock::Locker locker(_crc_spinlock);
        
        if (0 != _crc_map.size())
        {
            _crc_map.clear();
        }
    }
    
public:
    // 保存的数据
    char* _data;
    // 申请的内存长度
    uint32_t _len;
    // 引用计数
    atomic_t _ref;
    // 保护_crc_map
    SpinLock _crc_spinlock;
    // crc校验信息
    // key pair为数据的起始和结束
    // value pair为base crc值和加上数据段后的crc值
    std::map<std::pair<size_t, size_t>, std::pair<uint32_t, uint32_t> > _crc_map;
};

class raw_combined : public raw
{
public:
    
    raw_combined(char* data, uint32_t len, uint32_t align = 0) : raw(data, len), _align(align)
    {
    }

    virtual ~raw_combined()
    {
    }

    /**
     * 创建内存对象
     *
     * @param len: 申请内存大小
     * @param align: 对齐方式
     */
    static raw_combined* create(uint32_t len, uint32_t align = 0)
    {
        if (!align)
        {
            // align = PAGE_SIZE;
            align = sizeof(size_t);
        }

        size_t raw_align = alignof(raw_combined);
        // raw_combined对象需要申请内存大小
        size_t rawlen = ROUND_UP_TO(sizeof(raw_combined), raw_align);
        // 数据段需要申请内存大小
        size_t datalen = ROUND_UP_TO(len, raw_align);
#ifdef DARWIN
        char* p = (char*)valloc(rawlen + datalen);
#else
        char* p = NULL;
        int r = posix_memalign((void**)(void*)&p, align, rawlen + datalen);
        if (r)
        {
            THROW_SYSCALL_EXCEPTION(NULL, r, "posix_memalign");
        }
#endif
        if (!p)
        {
            THROW_SYSCALL_EXCEPTION(NULL, r, "posix_memalign");
        }

        return new (p + datalen)raw_combined(p, len, align);
    }

    static void operator delete(void* p)
    {
        raw_combined* raw = (raw_combined*)p;
        free((void*)raw->_data);
    }

    raw* clone_empty()
    {
        return create(_len, _align);
    }
    
private:
    // 按多少字节对齐
    uint32_t _align;
};

#ifndef __CYGWIN__
class raw_posix_aligned : public raw
{
public:
    raw_posix_aligned(uint32_t l, uint32_t align) : raw(l)
    {
        _align = align;

#ifdef DARWIN
        _data = (char *)valloc(_len);
#else
        _data = NULL;
        int r = posix_memalign((void**)(void*)&_data, _align, _len);
        if (r)
        {
            THROW_SYSCALL_EXCEPTION(NULL, r, "posix_memalign");
        }
#endif
        if (!_data)
        {
            THROW_SYSCALL_EXCEPTION(NULL, r, "posix_memalign");
        }
    }
    
    ~raw_posix_aligned()
    {
        free((void*)_data);
    }
    
    raw* clone_empty()
    {
        return new raw_posix_aligned(_len, _align);
    }
    
private:
    uint32_t _align;
};
#endif


#ifdef __CYGWIN__
class raw_hack_aligned : public raw
{
public:
    raw_hack_aligned(uint32_t l, uint32_t align) : raw(l), _align(align)
    {
        // _align = align;
        _realdata = new char[_len + _align - 1];

        // 检查地址是否按照align对齐
        uint32_t off = ((uint32_t)_realdata) & (_align - 1);
        
        if (off)
        {
            _data = _realdata + _align - off;
        }
        else
        {
            _data = _realdata;
        }
    }
    
    ~raw_hack_aligned()
    {
        delete[] _realdata;
    }
    raw* clone_empty()
    {
        return new raw_hack_aligned(_len, _align);
    }

private:
    uint32_t _align;
    char* _realdata;
};
#endif


raw* create(uint32_t len)
{
    return create_aligned(len, sizeof(size_t));
}

raw* create_aligned(uint32_t len, uint32_t align)
{
    int page_size = PAGE_SIZE;

    // page_size - 1 -> 0FFF
    // 对齐系数是page_size的倍数
    if (0 == (align & (page_size - 1)) || len >= page_size * 2)
    {
#ifndef __CYGWIN__
        return new raw_posix_aligned(len, align);
#else
        return new raw_hack_aligned(len, align);
#endif
    }
    
    return raw_combined::create(len, align);
}

raw* create_page_aligned(uint32_t len)
{
    return create_aligned(len, PAGE_SIZE);
}

raw* copy(const char* c, uint32_t len)
{
    raw* r = create_aligned(len, sizeof(size_t));
    memcpy(r->_data, c, len);
    return r;
}



ptr::ptr(raw* r) : _raw(r), _off(0), _len(r->_len)
{
    atomic_inc(&(r->_ref));
}

ptr::ptr(uint32_t len) : _off(0), _len(len)
{
    _raw = create(len);
    atomic_inc(&(_raw->_ref));
}

ptr::ptr(const ptr& p) : _raw(p._raw), _len(p._len), _off(p._off)
{
    if (_raw)
    {
        atomic_inc(&(_raw->_ref));
    }
}

ptr::ptr(ptr&& p) : _raw(p._raw), _off(p._off), _len(p._len)
{
    p._raw = NULL;
    p._off = 0;
    p._len = 0;
}

ptr::ptr(const ptr& p, uint32_t offset, uint32_t len) : _raw(p._raw), _off(p._off + offset), _len(len)
{
    atomic_inc(&(_raw->_ref));
}


ptr& ptr::operator=(const ptr& p)
{
    if (p._raw)
    {
        atomic_inc(&(p._raw->_ref));
    }

    raw* r = p._raw; 
    release();
    
    if (r)
    {
        _raw = r;
        _off = p._off;
        _len = p._len;
    }
    else
    {
        _off = 0;
        _len = 0;
    }
    
    return *this;
}

ptr& ptr::operator=(ptr&& p)
{
    release();
    raw* raw = p._raw;
    if (raw)
    {
        _raw = raw;
        _off = p._off;
        _len = p._len;
        p._raw = NULL;
        p._off = 0;
        p._len = 0;
    }
    else
    {
        _off = 0;
        _len = 0;
    }
    
    return *this;
}


/*
const char& ptr::operator[](uint32_t n)
{
    return _raw->get_data()[_off + n];
}

char& ptr::operator[](uint32_t n)
{
    return _raw->get_data()[_off + n];
}
*/


void ptr::release()
{
    if (_raw)
    {
        atomic_dec(&(_raw->_ref));
        
        if (0 == _raw->_ref)
        {
            delete _raw;
        }
        
        _raw = 0;
    }
}

uint32_t ptr::append(const char* p, uint32_t len)
{
    char* c = _raw->get_data() + _off + _len;

    memcpy(c, p, len);

    _len += len;
    
    return _len + _off;
}

uint32_t ptr::append(char c)
{
    char* p = _raw->get_data() + _off + _len;

    *p = c;

    _len++;
    
    return _len + _off;
}

char* ptr::c_str()
{
    return _raw->get_data() + _off;
}

const char* ptr::c_str() const
{
    return _raw->get_data() + _off;
}

const char* ptr::end_c_str() const
{
    return _raw->get_data() + _len + _off;
}

uint32_t ptr::unused_tail_length() const
{
    if (_raw)
    {
        return _raw->_len - (_off + _len);
    }
    else
    {
        return 0;
    }
}

ptr& ptr::make_shareable()
{
    if (_raw && !_raw->is_shareable())
    {
        raw* t = _raw;
        _raw = t->clone();
        atomic_set(&(_raw->_ref), 1);
        atomic_dec(&(t->_ref));
        if (unlikely(0 == _raw->_ref))
        {
            delete t;
        }
    }
    
    return *this;
}

void ptr::copy_in(uint32_t o, uint32_t l, const char* src)
{
    copy_in(o, l, src, true);
}

void ptr::copy_in(uint32_t o, uint32_t l, const char* src, bool crc_reset)
{
    char* dest = _raw->_data + _off + o;
    
    if (crc_reset)
    {
        _raw->invalidate_crc();
    }
    
    maybe_inline_memcpy(dest, src, l, 64);
}

void ptr::copy_out(uint32_t o, uint32_t l, char* dest) const 
{
    if (o + l > _len)
    {
        THROW_SYSCALL_EXCEPTION(NULL, -1, "copy_out");
    }
    
    char* src =  _raw->_data + _off + o;
    maybe_inline_memcpy(dest, src, l, 8);
}


void ptr::swap(ptr& other)
{
    raw* r = _raw;
    unsigned o = _off;
    unsigned l = _len;
    _raw = other._raw;
    _off = other._off;
    _len = other._len;
    other._raw = r;
    other._off = o;
    other._len = l;
}



template<bool is_const>
buffer::iterator_impl<is_const>::iterator_impl(buf_t* buf, uint32_t offset) : _buffer(buf), _ptrs(&(buf->_ptrs)),
                                _offset(0), _iter(_ptrs->begin()), _p_offset(0)
{
    advance(offset);
}

template<bool is_const>
void buffer::iterator_impl<is_const>::advance(ssize_t offset)
{
    if (0 < offset)
    {
        _p_offset += offset;
        while (0 < _p_offset)
        {
            if (_iter == _ptrs->end())
            {
                THROW_SYSCALL_EXCEPTION(NULL, -1, "advance");
            }
            
            if (_p_offset >= _iter->length())
            {
                _p_offset -= _iter->length();
                _iter++;
            }
            else
            {
                break;
            }
        }
        
        _offset += offset;
        
        return;
    }

    
    while (0 > offset) 
    {
        if (_p_offset)
        {
            uint32_t d = -offset;
            if (d > _p_offset)
            {
                d = _p_offset;
            }
            
            _p_offset -= d;
            _offset -= d;
            offset += d;
        }
        else if (0 < _offset)
        {
            _iter--;
            _p_offset = _iter->length();
        }
        else
        {
            THROW_SYSCALL_EXCEPTION(NULL, -1, "advance");
        }
    }
}

template<bool is_const>
ptr buffer::iterator_impl<is_const>::get_current_ptr() const
{
    if (_iter == _ptrs->end())
    {
        THROW_SYSCALL_EXCEPTION(NULL, -1, "get_current_ptr");
    }
    
    return ptr(*_iter, _p_offset, _iter->length() - _p_offset);
}

template<bool is_const>
void buffer::iterator_impl<is_const>::seek(size_t offset)
{
    _iter = _ptrs->begin();
    _offset = _p_offset = 0;
    advance(offset);
}

template<bool is_const>
void buffer::iterator_impl<is_const>::copy(uint32_t len, char* dest)
{
    if (_iter == _ptrs->end())
    {
        seek(_offset);
    }
    
    while (0 < len)
    {
        if (_iter == _ptrs->end())
        {
            THROW_SYSCALL_EXCEPTION(NULL, -1, "copy");
        }

        uint32_t howmuch = _iter->length() - _p_offset;
        if (len < howmuch)
        {
            howmuch = len;
        }
        
        _iter->copy_out(_p_offset, howmuch, dest);
        dest += howmuch;

        len -= howmuch;
        
        advance(howmuch);
    }
}

template<bool is_const>
void buffer::iterator_impl<is_const>::copy(uint32_t len, ptr& dest)
{
    dest = create(len);
    copy(len, dest.c_str());
}

template<bool is_const>
void buffer::iterator_impl<is_const>::copy(uint32_t len, buffer& dest)
{
    if (_iter == _ptrs->end())
    {
        seek(_offset);
    }
    
    while (0 < len)
    {
        if (_iter == _ptrs->end())
        {
            THROW_SYSCALL_EXCEPTION(NULL, -1, "copy");
        }

        uint32_t howmuch = _iter->length() - _p_offset;
        if (len < howmuch)
        {
            howmuch = len;
        }
        
        dest.append(*_iter, _p_offset, howmuch);

        len -= howmuch;
        advance(howmuch);
    }
}

template<bool is_const>
void buffer::iterator_impl<is_const>::copy(uint32_t len, std::string& dest)
{
    if (_iter == _ptrs->end())
    {
        seek(_offset);
    }
    
    while (0 < len)
    {
        if (_iter == _ptrs->end())
        {
            THROW_SYSCALL_EXCEPTION(NULL, -1, "copy");
        }

        unsigned howmuch = _iter->length() - _p_offset;
        const char *c_str = _iter->c_str();
        if (len < howmuch)
        {
            howmuch = len;
        }
        
        dest.append(c_str + _p_offset, howmuch);

        len -= howmuch;
        advance(howmuch);
    }
}

template<bool is_const>
void buffer::iterator_impl<is_const>::copy_all(buffer& dest)
{
    if (_iter == _ptrs->end())
    {
        seek(_offset);
    }
    
    while (1)
    {
        if (_iter == _ptrs->end())
        {
            return;
        }

        unsigned howmuch = _iter->length() - _p_offset;
        const char *c_str = _iter->c_str();
        dest.append(c_str + _p_offset, howmuch);

        advance(howmuch);
    }
}




buffer::iterator::iterator(buf_t* buf, uint32_t offset) : iterator_impl(buf, offset)
{
}

buffer::iterator::iterator(buf_t* buf, uint32_t offset, bufs_iter_t iter, uint32_t p_offset) : iterator_impl(buf, offset, iter, p_offset)
{
}



ptr buffer::iterator::get_current_ptr()
{
    if (_iter == _ptrs->end())
    {
        THROW_SYSCALL_EXCEPTION(NULL, -1, "get_current_ptr");
    }
    
    return ptr(*_iter, _p_offset, _iter->length() - _p_offset);
}

void buffer::iterator::copy(uint32_t len, char* dest)
{
    return buffer::iterator_impl<false>::copy(len, dest);
}

void buffer::iterator::copy(uint32_t len, ptr& dest)
{
    buffer::iterator_impl<false>::copy(len, dest);
}

void buffer::iterator::copy(uint32_t len, buffer& dest)
{
    buffer::iterator_impl<false>::copy(len, dest);
}

void buffer::iterator::copy(uint32_t len, std::string& dest)
{
    buffer::iterator_impl<false>::copy(len, dest);
}

void buffer::iterator::copy_all(buffer& dest)
{
    buffer::iterator_impl<false>::copy_all(dest);
}

void buffer::iterator::copy_in(uint32_t len, const char* src)
{
    copy_in(len, src, true);
}

void buffer::iterator::copy_in(uint32_t len, const char* src, bool crc_reset)
{
    if (_iter == _ptrs->end())
    {
        seek(_offset);
    }

    while (0 < len)
    {
        if (_iter == _ptrs->end())
        {
            THROW_SYSCALL_EXCEPTION(NULL, -1, "copy_in");
        }

        uint32_t howmuch = _iter->length() - _p_offset;
        if (len < howmuch)
        {
            howmuch = len;
        }
        
        _iter->copy_in(_p_offset, howmuch, src, crc_reset);

        src += howmuch;
        len -= howmuch;
        advance(howmuch);
    }
}

void buffer::iterator::copy_in(uint32_t len, const buffer& other)
{
    if (_iter == _ptrs->end())
    {
        seek(_offset);
    }

    uint32_t left = len;

    for (std::list<ptr>::const_iterator it = other._ptrs.begin(); it != other._ptrs.end(); ++it)
    {
        uint32_t l = (*it).length();
        if (left < l)
        {
            l = left;
        }
        
        copy_in(l, it->c_str());
        left -= l;
        
        if (0 == left)
        {
            break;
        }
    }
}



buffer::buffer(buffer&& other) : _ptrs(std::move(other._ptrs)), _len(other._len),
        _memcopy_count(other._memcopy_count), _last_p(this)
{
    _append_ptr.swap(other._append_ptr);
    other.clear();
}

void buffer::append(char c)
{
    // 没有可用的空间
    if (!_append_ptr.unused_tail_length())
    {
        _append_ptr = raw_combined::create(BUFFER_APPEND_SIZE);
        _append_ptr.set_length(0);
    }

    append(_append_ptr, _append_ptr.append(c) - 1, 1);
}

void buffer::append(const char* data, uint32_t len)
{
    while (0 < len)
    {
        // _append_ptr剩余的长度
        uint32_t unused = _append_ptr.unused_tail_length();

        if (unused)
        {
            if (len < unused)
            {
                unused = len;
            }

            append(_append_ptr, _append_ptr.append(data, unused) - unused, unused);

            len -= unused;

            data += unused;
        }

        if (0 == len)
        {
            break;
        }

        size_t need = ROUND_UP_TO(len, sizeof(size_t)) + sizeof(raw_combined);
        size_t alen = ROUND_UP_TO(need, BUFFER_ALLOC_UNIT) - sizeof(raw_combined);
        _append_ptr = raw_combined::create(alen);
        _append_ptr.set_length(0);
    }
}


void buffer::append(const ptr& p, uint32_t off, uint32_t len)
{
    if (!_ptrs.empty())
     {
         ptr& appender = _ptrs.back();
         // _append_ptr
         if (appender.get_raw() == p.get_raw() && appender.end() == p.start() + off)
         {
             appender.set_length(appender.length() + len);
             _len += len;
             return;
         }
     }

     push_back(ptr(p, off, len));
}

void buffer::append(const buffer& buf)
{
    _len += buf._len;
    for (std::list<ptr>::const_iterator it = buf._ptrs.begin(); it != buf._ptrs.end(); ++it)
    {
        _ptrs.push_back(*it);
    }
}


void buffer::push_back(const ptr& p)
{
    if (0 == p.length())
    {
        return;
    }
    
    _ptrs.push_back(p);
    _len + p.length();
}


uint32_t buffer::crc32(uint32_t crc) const
{
    for (std::list<ptr>::const_iterator it = _ptrs.begin(); it != _ptrs.end(); ++it)
    {
        if (it->length())
        {
            raw* r = it->get_raw();
            std::pair<size_t, size_t> fromto(it->offset(), it->offset() + it->length());
            std::pair<uint32_t, uint32_t> ccrc;
            // 是否有对应数据段的crc值
            if (r->get_crc(fromto, &ccrc))
            {
                if (ccrc.first == crc)
                {
                    crc = ccrc.second;
                }
                else
                {
                    crc = ccrc.second ^ crc32c(ccrc.first ^ crc, NULL, it->length());
                }
            }
            else 
            {
                uint32_t base = crc;
                crc = crc32c(crc, (unsigned char*)it->c_str(), it->length());
                r->set_crc(fromto, std::make_pair(base, crc));
            }
        }
    }
}

void buffer::claim(buffer& buf, unsigned int flags)
{
    clear();

    claim_append(buf, flags);
}

void buffer::claim_append(buffer& buf, unsigned int flags)
{
    _len += buf._len;

    if (!(flags & CLAIM_ALLOW_NONSHAREABLE))
    {
        buf.make_shareable();
    }

    _ptrs.splice(_ptrs.end(), buf._ptrs);

    // 清空源buf
    // buf.set_length(0);
    buf._len = 0;
    buf._last_p = buf.begin();
}

char* buffer::c_str()
{
    if (_ptrs.empty())
    {
        return NULL;
    }

    std::list<ptr>::const_iterator iter = _ptrs.begin();
    ++iter;

    if (iter != _ptrs.end())
    {
        rebuild();
    }
    
    return _ptrs.front().c_str();
}

void buffer::rebuild()
{
    if (0 == _len)
    {
        _ptrs.clear();
        return;
    }
    
    ptr nb;
    
    if (0 == (_len & ~PAGE_MASK))
    {
        nb = create_page_aligned(_len);
    }
    else
    {
        nb = create(_len);
    }
    
    rebuild(nb);
}

void buffer::rebuild(ptr& nb)
{
    uint32_t pos = 0;
    for (std::list<ptr>::iterator it = _ptrs.begin(); it != _ptrs.end(); ++it)
    {
        nb.copy_in(pos, it->length(), it->c_str(), false);
        pos += it->length();
    }
        
    _memcopy_count += pos;
    _ptrs.clear();
    if (nb.length())
    {
        _ptrs.push_back(nb);
    }
    
    invalidate_crc();
    _last_p = begin();
}

void buffer::invalidate_crc()
{
    for (std::list<ptr>::const_iterator it = _ptrs.begin(); it != _ptrs.end(); ++it)
    {
        raw* r = it->get_raw();
        if (r)
        {
            r->invalidate_crc();
        }
    }
}

