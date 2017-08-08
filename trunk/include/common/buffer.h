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
#include "acconfig.h"
#include "builtin.h"
#include "page.h"
#include "exception.h"


class raw;

raw* copy(const char* c, uint32_t len);
raw* create(uint32_t len);
raw* create_aligned(uint32_t len, uint32_t align);
raw* create_page_aligned(uint32_t len);

// ptr为raw的部分数据段
class ptr
{
public:
	ptr() : _raw(0), _off(0), _len(0)
	{}

	ptr(raw* r);
	ptr(uint32_t l);
	ptr(const char* d, uint32_t l);
	ptr(const ptr& p);
	ptr(ptr&& p);
	ptr(const ptr& p, uint32_t o, uint32_t l);
	ptr& operator=(const ptr& other);
	ptr& operator=(ptr&& p);
	~ptr() 
	{
		release();
    }

	bool have_raw() const
	{
		return _raw ? true : false;
	}

	raw* clone();

	void swap(ptr& other);

	ptr& make_shareable();

	bool at_buffer_head() const { return 0 == _off; }

	bool at_buffer_tail() const;

	bool is_aligned(uint32_t align) const
	{
		return 0 == ((long)c_str() & (align - 1));
	}

	bool is_page_aligned() const
	{
		return is_aligned(PAGE_SIZE);
	}

	bool is_n_align_sized(uint32_t align) const
    {
		return 0 == (length() % align);
    }

	bool is_n_page_sized() const
	{
		return is_n_align_sized(PAGE_SIZE);
	}

	bool is_partial() const 
	{
		return have_raw() && (0 < start() || end() < raw_length());
    }

	raw* get_raw() const { return _raw; }

	const char *c_str() const;
	
	char *c_str();
	
	const char* end_c_str() const;
	
    char* end_c_str();

	uint32_t length() const { return _len; }

	uint32_t offset() const { return _off; }

	uint32_t start() const { return _off; }

	uint32_t end() const { return _off + _len; }

	uint32_t unused_tail_length() const;

	const char& operator[](uint32_t n) const;

	char& operator[](uint32_t n);

	const char* raw_c_str() const;

	uint32_t raw_length() const;

	int raw_nref() const;

	void copy_out(uint32_t o, uint32_t l, char* dest) const;

	void copy_in(uint32_t o, uint32_t l, const char* src);

	void copy_in(uint32_t o, uint32_t l, const char* src, bool crc_reset);

	bool can_zero_copy() const;

	int zero_copy_to_fd(int fd, int64_t* offset) const;

	uint32_t wasted();

	int cmp(const ptr& other) const;

	bool is_zero() const;

	void set_offset(uint32_t o)
	{
		_off = o;
    }

	void set_length(uint32_t l)
	{
		_len = l;
    }

	uint32_t append(char c);

	uint32_t append(const char* p, uint32_t l);

	void zero();

	void zero(bool crc_reset);

	void zero(uint32_t o, uint32_t l);

	void zero(uint32_t o, uint32_t l, bool crc_reset);

public:
	class iterator
	{
		const ptr* _p;
		const char* _begin;
		const char* _pos;
		const char* _end;
		bool _deep;

		iterator(const ptr* p, size_t offset, bool d) : _p(p), _begin(p->c_str() + offset), _pos(_begin),
				_end(p->end_c_str()), _deep(d)
		{}

		friend class ptr;

	public:
		const char* get_pos_add(size_t n)
		{
			const char* r = _pos;
			_pos += n;
			if (_pos > _end)
			{
				THROW_SYSCALL_EXCEPTION(NULL, -1, "get_pos_add");
			}
			
			return r;
		}

		ptr get_ptr(size_t len)
		{
			if (_deep)
			{
				return copy(get_pos_add(len), len);
			}
			else
			{
				size_t off = _pos - _p->c_str();
				_pos += len;
				if (_pos > _end)
				{
					THROW_SYSCALL_EXCEPTION(NULL, -1, "get_ptr");
				}
				
				return ptr(*_p, off, len);
			}
		}
		
		ptr get_preceding_ptr(size_t len)
		{
			if (_deep)
			{
				return copy(get_pos() - len, len);
			}
			else
			{
				size_t off = _pos - _p->c_str();
				return ptr(*_p, off - len, len);
			}
		}

		void advance(size_t len)
		{
			_pos += len;
			if (_pos > _end)
			{
				THROW_SYSCALL_EXCEPTION(NULL, -1, "advance");
			}
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

		bool end() const
		{
			return _pos == _end;
		}
    };
	

private:
	void release();
	
private:
	// 
	raw* _raw;
	// _raw里的偏移量
	uint32_t _off;
	// prt的长度
	uint32_t _len;
};

// 多个ptr的列表
class buffer
{
public:
	class iterator;

private:
	template<bool is_const>
	class iterator_impl : public std::iterator<std::forward_iterator_tag, char>
	{
	protected:
		typedef typename std::conditional<is_const, const buffer, buffer>::type bl_t;
		typedef typename std::conditional<is_const, const std::list<ptr>, std::list<ptr> >::type list_t;
		typedef typename std::conditional<is_const, typename std::list<ptr>::const_iterator, typename std::list<ptr>::iterator>::type list_iter_t;

		bl_t* _bl;
		list_t* _ls;
		uint32_t _off;
		uint32_t _p_off;
		list_iter_t _iter;
		friend class iterator_impl<true>;
	public:
		iterator_impl() : _bl(NULL), _ls(NULL), _off(0), _p_off(0) {}
		iterator_impl(bl_t* l, uint32_t o = 0);
		iterator_impl(bl_t* l, uint32_t o, list_iter_t li, uint32_t po) : _bl(l), _ls(&_bl->_buffers), _off(o), _p_off(po), _iter(li) {}
		iterator_impl(const buffer::iterator& i);

		uint32_t get_off() const { return _off; }
		
		unsigned get_remaining() const { return _bl->length() - _off; }

		bool end() const
		{
			return _iter == _ls->end();
			//return _off == _bl->length();
		}

		void advance(ssize_t o);

		void seek(size_t o);

		char operator *() const;
		
		iterator_impl& operator ++();
		
		ptr get_current_ptr() const;

		bl_t& get_bl() const { return *_bl; }

		void copy(uint32_t len, char* dest);

		void copy(uint32_t len, ptr& dest);

		void copy(uint32_t len, buffer& dest);

		void copy(unsigned len, std::string& dest);

		void copy_all(buffer& dest);

		size_t get_ptr_and_advance(size_t want, const char** p);

		uint32_t crc32(size_t length, uint32_t crc);

		friend bool operator ==(const iterator_impl& lhs, const iterator_impl& rhs)
		{
			return &lhs.get_bl() == &rhs.get_bl() && lhs.get_off() == rhs.get_off();
		}

		friend bool operator !=(const iterator_impl& lhs, const iterator_impl& rhs)
		{
			return &lhs.get_bl() != &rhs.get_bl() || lhs.get_off() != rhs.get_off();
		}
		
	};

public:
	typedef iterator_impl<true> const_iterator;

	class iterator : public iterator_impl<false>
	{
	public:
		iterator() {};
		iterator(bl_t *l, uint32_t o = 0);
		iterator(bl_t *l, uint32_t o, list_iter_t ip, uint32_t po);

		void advance(ssize_t o);
		void seek(size_t o);
		char operator*();
		iterator& operator++();
		ptr get_current_ptr();

		void copy(uint32_t len, char* dest);
		void copy(uint32_t len, ptr& dest);
		void copy(uint32_t len, buffer& dest);
		void copy(uint32_t len, std::string& dest);
		void copy_all(buffer& dest);

		void copy_in(uint32_t len, const char *src);
		void copy_in(uint32_t len, const char *src, bool crc_reset);
		void copy_in(uint32_t len, const buffer& otherl);

		bool operator==(const iterator& rhs) const
		{
			return _bl == rhs._bl && _off == rhs._off;
		}
		
		bool operator!=(const iterator& rhs) const
		{
			return _bl != rhs._bl || _off != rhs._off;
		}
		
	};

	class page_aligned_appender
	{
		buffer* _buf;
		size_t _offset;
		uint32_t _min_alloc;
		ptr _p;
		char* _pos;
		char* _end;

		page_aligned_appender(buffer* b, uint32_t min_pages) : _buf(b), _min_alloc(min_pages * PAGE_SIZE), _pos(NULL), _end(NULL)
		{}

		friend class buffer;

	public:
		~page_aligned_appender()
		{
			flush();
		}

		void flush()
		{
			if (_pos && _pos != _p.c_str())
			{
				size_t len = _pos - _p.c_str();
				_buf->append(_p, 0, len);
				_p.set_length(_p.length() - len);
				_p.set_offset(_p.offset() + len);
			}
		}

		void append(const char* buf, size_t len)
		{
			while (0 < len)
			{
				if (!_pos)
				{
					size_t alloc = (len + PAGE_SIZE - 1) & PAGE_MASK;
					if (alloc < _min_alloc)
					{
						alloc = _min_alloc;
					}
					
					_p = create_page_aligned(alloc);
					_pos = _p.c_str();
					_end = _p.end_c_str();
				}
				
				size_t l = len;
				if (l > (size_t)(_end - _pos))
				{
					l = _end - _pos;
				}
				
				memcpy(_pos, buf, l);
				_pos += l;
				buf += l;
				len -= l;
				
				if (_pos == _end)
				{
					_buf->append(_p, 0, _p.length());
					_pos = _end = NULL;
				}
			}
		}
	};

	page_aligned_appender get_page_aligned_appender(uint32_t min_pages = 1)
	{
		return page_aligned_appender(this, min_pages);
	}

	
	class contiguous_appender
	{
		buffer* _buf;
		char* _pos;
		ptr _p;
		bool _deep;
	
		size_t out_of_band_offset = 0;
	
		contiguous_appender(buffer* b, size_t len, bool d) : _buf(b), _deep(d)
		{
			size_t unused = _buf->_append_buffer.unused_tail_length();
			if (len > unused)
			{
				_p = create(len);
				_pos = _p.c_str();
			}
			else
			{
				_pos = _buf->_append_buffer.end_c_str();
			}
		}
	
		void flush_and_continue()
		{
			if (_p.have_raw())
			{
				size_t l = _pos - _p.c_str();
				_buf->append(ptr(_p, 0, l));
				_p.set_length(_p.length() - l);
				_p.set_offset(_p.offset() + l);
			}
			else
			{
				size_t l = _pos - _buf->_append_buffer.end_c_str();
				if (l)
				{
					_buf->_append_buffer.set_length(_buf->_append_buffer.length() + l);
					_buf->append(_buf->_append_buffer, _buf->_append_buffer.end() - l, l);
					_pos = _buf->_append_buffer.end_c_str();
				}
			}
		}
	
		friend class buffer;
	
	public:
		~contiguous_appender()
		{
			if (_p.have_raw())
			{
				_p.set_length(_pos - _p.c_str());
				_buf->append(std::move(_p));
			}
			else
			{
				size_t l = _pos - _buf->_append_buffer.end_c_str();
				if (l)
				{
					_buf->_append_buffer.set_length(_buf->_append_buffer.length() + l);
					_buf->append(_buf->_append_buffer, _buf->_append_buffer.end() - l, l);
				}
			}
		}
	
		size_t get_out_of_band_offset() const
		{
			return out_of_band_offset;
		}
		
		void append(const char* p, size_t l)
		{
			maybe_inline_memcpy(_pos, p, l, 16);
			_pos += l;
		}
		
		char* get_pos_add(size_t len)
		{
			char* r = _pos;
			_pos += len;
			return r;
		}
		
		char* get_pos()
		{
			return _pos;
		}
	
		void append(const ptr& p)
		{
			if (!p.length())
			{
				return;
			}
			
			if (_deep)
			{
				append(p.c_str(), p.length());
			}
			else
			{
				flush_and_continue();
				_buf->append(p);
				out_of_band_offset += p.length();
			}
		}
		
		void append(const buffer& l)
		{
			if (!l.length())
			{
				return;
			}
			
			if (_deep)
			{
				for (const auto &p : l._buffers)
				{
					append(p.c_str(), p.length());
				}
			}
			else
			{
				flush_and_continue();
				_buf->append(l);
				out_of_band_offset += l.length();
			}
		}
	
		size_t get_logical_offset()
		{
			if (_p.have_raw())
			{
				return out_of_band_offset + (_pos - _p.c_str());
			}
			else
			{
				return out_of_band_offset + (_pos - _buf->_append_buffer.end_c_str());
			}
		}
	};
	
	contiguous_appender get_contiguous_appender(size_t len, bool deep = false)
	{
		return contiguous_appender(this, len, deep);
	}

public:
	buffer() : _len(0), _memcopy_count(0), _last_p(this) {}

	buffer(uint32_t prealloc) : _len(0), _memcopy_count(0), _last_p(this)
	{
		reserve(prealloc);
    }

	buffer(const buffer& other) : _buffers(other._buffers), _len(other._len), _memcopy_count(other._memcopy_count), _last_p(this)
	{
		make_shareable();
    }

	buffer(buffer&& other);
	
    buffer& operator=(const buffer& other)
	{
		if (this != &other)
		{
        	_buffers = other._buffers;
        	_len = other._len;
			make_shareable();
		}
		
		return *this;
    }

	buffer& operator=(buffer&& other)
	{
		_buffers = std::move(other._buffers);
		_len = other._len;
		_memcopy_count = other._memcopy_count;
		_last_p = begin();
		_append_buffer.swap(other._append_buffer);
		other.clear();
		return *this;
    }

	unsigned get_num_buffers() const { return _buffers.size(); }

	const ptr& front() const { return _buffers.front(); }

	const ptr& back() const { return _buffers.back(); }

	uint32_t get_memcopy_count() const {return _memcopy_count; }

	const std::list<ptr>& buffers() const { return _buffers; }

	void swap(buffer& other);

	uint32_t length() const { return _len; }

	bool contents_equal(buffer& other);

	bool contents_equal(const buffer& other) const;

	bool can_zero_copy() const;

	bool is_provided_buffer(const char* dst) const;

	bool is_aligned(uint32_t align) const;

	bool is_page_aligned() const;

	bool is_n_align_sized(uint32_t align) const;

	bool is_n_page_sized() const;

	bool is_aligned_size_and_memory(uint32_t align_size, uint32_t align_memory) const;

	bool is_zero() const;

	void clear()
	{
		_buffers.clear();
		_len = 0;
		_memcopy_count = 0;
		_last_p = begin();
		_append_buffer = ptr();
    }

	void push_front(ptr& bp)
	{
		if (0 == bp.length())
		{
			return;
		}
		
      	_buffers.push_front(bp);
      	_len += bp.length();
    }

	void push_front(ptr&& bp)
	{
		if (0 == bp.length())
		{
			return;
		}
		
		_len += bp.length();
		_buffers.push_front(std::move(bp));
    }

	void push_front(raw* r)
	{
		push_front(ptr(r));
    }

	void push_back(const ptr& bp)
	{
		if (0 == bp.length())
		{
			return;
		}
		
		_buffers.push_back(bp);
		_len += bp.length();
    }

	void push_back(ptr&& bp)
	{
		if (0 == bp.length())
		{
			return;
		}
		
      	_len += bp.length();
		_buffers.push_back(std::move(bp));
    }

	void push_back(raw* r)
	{
		push_back(ptr(r));
    }

	void zero();

	void zero(uint32_t o, uint32_t l);

	bool is_contiguous() const;

	void rebuild();

	void rebuild(ptr& nb);

	bool rebuild_aligned(uint32_t align);

	bool rebuild_aligned_size_and_memory(uint32_t align_size, uint32_t align_memory);

	bool rebuild_page_aligned();

	void reserve(size_t prealloc)
	{
		if (_append_buffer.unused_tail_length() < prealloc)
		{
			_append_buffer = create(prealloc);
			_append_buffer.set_length(0);
		}
    }

	const static unsigned int CLAIM_DEFAULT = 0;
    const static unsigned int CLAIM_ALLOW_NONSHAREABLE = 1;

	void claim(buffer& bl, unsigned int flags = CLAIM_DEFAULT);

	void claim_append(buffer& bl, unsigned int flags = CLAIM_DEFAULT);

	void claim_prepend(buffer& bl, unsigned int flags = CLAIM_DEFAULT);

	void make_shareable()
	{
		std::list<ptr>::iterator iter;
		for (iter = _buffers.begin(); iter != _buffers.end(); ++iter)
		{
        	(void)iter->make_shareable();
		}
    }

	void share(const buffer& bl)
    {
		if (this != &bl)
		{
        	clear();
			std::list<ptr>::const_iterator iter;
        	for (iter = bl._buffers.begin(); iter != bl._buffers.end(); ++iter)
			{
          		push_back(*iter);
			}
		}
    }

	iterator begin()
	{
		return iterator(this, 0);
    }

	iterator end()
	{
		return iterator(this, _len, _buffers.end(), 0);
    }

	const_iterator begin() const
	{
		return const_iterator(this, 0);
    }

	const_iterator end() const
	{
		return const_iterator(this, _len, _buffers.end(), 0);
    }

	void copy(uint32_t off, uint32_t len, char* dest) const;
    void copy(uint32_t off, uint32_t len, buffer& dest) const;
    void copy(uint32_t off, uint32_t len, std::string& dest) const;
    void copy_in(uint32_t off, uint32_t len, const char* src);
    void copy_in(uint32_t off, uint32_t len, const char* src, bool crc_reset);
    void copy_in(uint32_t off, uint32_t len, const buffer& src);

    void append(char c);
    void append(const char* data, uint32_t len);
    void append(const std::string& s)
	{
		append(s.data(), s.length());
    }
    void append(const ptr& bp);
    void append(ptr&& bp);
    void append(const ptr& bp, uint32_t off, uint32_t len);
    void append(const buffer& bl);
    void append(std::istream& in);
    void append_zero(uint32_t len);

	const char& operator[](uint32_t n) const;

	char* c_str();

	std::string to_str() const;

	void substr_of(const buffer& other, uint32_t off, uint32_t len);

	char* get_contiguous(uint32_t off, uint32_t len);

	void splice(uint32_t off, uint32_t len, buffer* claim_by = 0);

	void write(uint32_t off, uint32_t len, std::ostream& out) const;

	void encode_base64(buffer& o);

	void decode_base64(buffer& o);

	void write_stream(std::ostream& out) const;

	void hexdump(std::ostream& out, bool trailing_newline = true) const;

	int read_file(const char* fn, std::string* error);

	ssize_t read_fd(int fd, size_t len);

	int read_fd_zero_copy(int fd, size_t len);

	int write_file(const char* fn, int mode = 0644);

	int write_fd(int fd) const;

	int write_fd(int fd, uint64_t offset) const;

	int write_fd_zero_copy(int fd) const;

	void prepare_iov(std::vector<iovec>* piov) const;

	uint32_t crc32(uint32_t crc) const;

	void invalidate_crc();

private:
    int zero_copy_to_fd(int fd) const;
	
private:
	std::list<ptr> _buffers;
	uint32_t _len;
	uint32_t _memcopy_count;
	ptr _append_buffer;
	mutable iterator _last_p;
};

class hash
{
private:
	uint32_t _crc;

public:
    hash() : _crc(0) {}
    hash(uint32_t init) : _crc(init) {}

    void update(buffer& bl)
	{
		_crc = bl.crc32(_crc);
    }

    uint32_t digest()
	{
		return _crc;
    }
};

inline bool operator>(buffer& l, buffer& r)
{
	for (uint32_t p = 0; ; p++)
	{
    	if (l.length() > p && r.length() == p)
    	{
			return true;
    	}
		
    	if (l.length() == p)
    	{
			return false;
    	}
		
    	if (l[p] > r[p])
    	{
			return true;
    	}
		
    	if (l[p] < r[p])
    	{
			return false;
    	}
	}
}

inline bool operator>=(buffer& l, buffer& r)
{
	for (uint32_t p = 0; ; p++)
	{
    	if (l.length() > p && r.length() == p)
    	{
			return true;
    	}
		
    	if (r.length() == p && l.length() == p)
    	{
			return true;
    	}
		
    	if (l.length() == p && r.length() > p)
    	{
			return false;
    	}
		
    	if (l[p] > r[p])
    	{
			return true;
    	}
		
    	if (l[p] < r[p])
    	{
			return false;
    	}
	}
}

inline bool operator==(const buffer &l, const buffer &r)
{
	if (l.length() != r.length())
	{
		return false;
	}
	
	for (uint32_t p = 0; p < l.length(); p++)
	{
    	if (l[p] != r[p])
    	{
			return false;
    	}
	}
	
	return true;
}

inline bool operator<(buffer& l, buffer& r)
{
	return r > l;
}

inline bool operator<=(buffer& l, buffer& r)
{
	return r >= l;
}


std::ostream& operator<<(std::ostream& out, const ptr& bp);

std::ostream& operator<<(std::ostream& out, const raw& r);

std::ostream& operator<<(std::ostream& out, const buffer& bl);

std::ostream& operator<<(std::ostream& out, const SysCallException& e);

inline hash& operator<<(hash& l, buffer& r)
{
	l.update(r);
	return l;
}

#endif
