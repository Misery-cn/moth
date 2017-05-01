#include <limits.h>
#include <errno.h>
#include <unistd.h> // pipe
#include <fcntl.h>
#include <sys/stat.h>
// #include <fstream>
// #include <sstream>
// #include <sys/uio.h>
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

	static atomic_t buffer_total_alloc;
	static atomic_t buffer_history_alloc_bytes;
	static atomic_t buffer_history_alloc_num;
	const bool buffer_track_alloc = get_env_bool("BUFFER_TRACK");



	void inc_total_alloc(uint32_t len)
	{
		if (buffer_track_alloc)
			atomic_add(len, &buffer_total_alloc);
	}

	void dec_total_alloc(uint32_t len)
	{
		if (buffer_track_alloc)
			atomic_sub(len, &buffer_total_alloc);
	}

	void inc_history_alloc(uint64_t len)
	{
		if (buffer_track_alloc)
		{
			atomic_add(len, &buffer_history_alloc_bytes);
			atomic_inc(&buffer_history_alloc_num);
		}
	}


	static atomic_t buffer_c_str_accesses;
	static bool buffer_track_c_str = get_env_bool("BUFFER_TRACK");

#define BUFFER_ALLOC_UNIT  (MIN(PAGE_SIZE, 4096))
#define BUFFER_APPEND_SIZE (BUFFER_ALLOC_UNIT - sizeof(raw_combined))

	static atomic_t buffer_max_pipe_size;

	int update_max_pipe_size()
	{
#ifdef CEPH_HAVE_SETPIPE_SZ
		char buf[32];
		int r;
	    std::string err;
		struct stat stat_result;
	    if (::stat("/proc/sys/fs/pipe-max-size", &stat_result) == -1)
			return -errno;
		r = safe_read_file("/proc/sys/fs/", "pipe-max-size",
			       buf, sizeof(buf) - 1);
	    if (r < 0)
			return r;
	    buf[r] = '\0';
	    size_t size = strict_strtol(buf, 10, &err);
	    if (!err.empty())
			return -EIO;
		atomic_set(&buffer_max_pipe_size, size);
#endif
	    return 0;
	}


	size_t get_max_pipe_size()
	{
#ifdef CEPH_HAVE_SETPIPE_SZ
		size_t size = atomic_read(&buffer_max_pipe_size);
		if (size)
			return size;
	    if (update_max_pipe_size() == 0)
			return atomic_read(&buffer_max_pipe_size);
#endif

	    return 65536;
	}


	// 原始数据buffer
	class raw
	{
	public:
		explicit raw(uint32_t l) : _data(NULL), _len(l), _ref(0)
		{}

		raw(char* c, uint32_t l) : _data(c), _len(l), _ref(0)
	    {}

		virtual ~raw() {}

		raw(const raw& other);
	    const raw& operator =(const raw& other);

		virtual char* get_data()
		{
			return _data;
	    }

		virtual raw* clone_empty() = 0;

		raw *clone()
		{
			raw* c = clone_empty();
			memcpy(c->_data, _data, _len);
			return c;
	    }

		virtual bool can_zero_copy() const
		{
			return false;
	    }

		virtual int zero_copy_to_fd(int fd, loff_t* offset)
		{
			return -ENOTSUP;
	    }

		// 是否页对齐,是否是页的整数倍
		virtual bool is_page_aligned()
		{
			return ((long)_data & ~PAGE_SIZE) == 0;
	    }

		bool is_n_page_sized()
		{
			return (_len & ~PAGE_SIZE) == 0;
	    }

		virtual bool is_shareable()
		{
			return true;
	    }

		bool get_crc(const std::pair<size_t, size_t>& fromto, std::pair<uint32_t, uint32_t>* crc) const
		{
			SpinLock::Locker locker(_crc_spinlock);
			
			std::map<std::pair<size_t, size_t>, std::pair<uint32_t, uint32_t> >::const_iterator iter =
			_crc_map.find(fromto);
			if (iter == _crc_map.end())
			{
				return false;
			}
			
			*crc = iter->second;
			return true;
	    }

		void set_crc(const std::pair<size_t, size_t>& fromto, const std::pair<uint32_t, uint32_t>& crc)
		{
			SpinLock::Locker locker(_crc_spinlock);
			_crc_map[fromto] = crc;
	    }

		void invalidate_crc()
		{
			SpinLock::Locker locker(_crc_spinlock);
			if (_crc_map.size() != 0)
			{
				_crc_map.clear();
			}
	    }

	public:
		// 数据指针
		char* _data;
		// 数据的长度
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
		raw_combined(char* data, uint32_t l, uint32_t align = 0) : raw(data, l), _alignment(align)
		{
			inc_total_alloc(_len);
			inc_history_alloc(_len);
	    }

		~raw_combined()
		{
			dec_total_alloc(_len);
	    }

		raw* clone_empty()
		{
			return create(_len, _alignment);
	    }

		static raw_combined* create(uint32_t len, uint32_t align = 0)
		{
			if (!align)
				align = sizeof(size_t);
	      	size_t rawlen = ROUND_UP_TO(sizeof(raw_combined), alignof(raw_combined));
			size_t datalen = ROUND_UP_TO(len, alignof(raw_combined));

#ifdef DARWIN
			char* ptr = (char *)valloc(rawlen + datalen);
#else
			char* ptr = 0;
			int r = posix_memalign((void**)(void*)&ptr, align, rawlen + datalen);
			if (r)
				THROW_SYSCALL_EXCEPTION(NULL, r, "posix_memalign");
#endif /* DARWIN */
			if (!ptr)
				THROW_SYSCALL_EXCEPTION(NULL, r, "posix_memalign");

			return new (ptr + datalen) raw_combined(ptr, len, align);
		}

		static void operator delete(void* ptr)
		{
			raw_combined* raw = (raw_combined*)ptr;
			free((void *)raw->_data);
	    }
		
	private:
		size_t _alignment;
	};


	class raw_malloc : public raw
	{
	public:
		explicit raw_malloc(uint32_t l) : raw(l)
		{
			if (_len)
			{
				_data = (char*)malloc(_len);
	        	if (!_data)
					THROW_SYSCALL_EXCEPTION(NULL, errno, "malloc");
			}
			else
			{
				_data = 0;
			}
			
			inc_total_alloc(_len);
			inc_history_alloc(_len);
	    }
		
	    raw_malloc(uint32_t l, char* b) : raw(b, l)
		{
			inc_total_alloc(_len);
	    }
		
	    ~raw_malloc()
		{
			free(_data);
			dec_total_alloc(_len);
	    }
		
	    raw* clone_empty()
		{
			return new raw_malloc(_len);
	    }
	};

#ifndef __CYGWIN__
	class raw_mmap_pages : public raw
	{
	public:
		explicit raw_mmap_pages(uint32_t l) : raw(l)
		{
			_data = (char*)mmap(NULL, _len, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0);
			if (!_data)
				THROW_SYSCALL_EXCEPTION(NULL, errno, "mmap");
			inc_total_alloc(_len);
			inc_history_alloc(_len);
	    }
		
	    ~raw_mmap_pages()
		{
			munmap(_data, _len);
			dec_total_alloc(_len);
	    }
		
		raw* clone_empty()
		{
			return new raw_mmap_pages(_len);
	    }
	};

	class raw_posix_aligned : public raw
	{
	private:
		uint32_t _align;
	public:
		raw_posix_aligned(uint32_t l, uint32_t align) : raw(l)
		{
			_align = align;

#ifdef DARWIN
			_data = (char *)valloc(_len);
#else
			_data = 0;
			int r = posix_memalign((void**)(void*)&_data, _align, _len);
			if (r)
				THROW_SYSCALL_EXCEPTION(NULL, r, "posix_memalign");
#endif /* DARWIN */
			if (!_data)
				THROW_SYSCALL_EXCEPTION(NULL, r, "posix_memalign");
			inc_total_alloc(_len);
			inc_history_alloc(_len);
	    }
		
	    ~raw_posix_aligned()
		{
			free((void*)_data);
			dec_total_alloc(_len);
		}
		
	    raw* clone_empty()
		{
			return new raw_posix_aligned(_len, _align);
	    }
	};
#endif


#ifdef __CYGWIN__
	class raw_hack_aligned : public raw
	{
	private:
		uint32_t _align;
		char* _realdata;
		
	public:
		raw_hack_aligned(uint32_t l, uint32_t align) : raw(l)
		{
			_align = align;
			_realdata = new char[_len + _align - 1];
			uint32_t off = ((uint32_t)_realdata) & (_align - 1);
			if (off)
				_data = _realdata + _align - off;
			else
				_data = _realdata;
			inc_total_alloc(_len + _align - 1);
			inc_history_alloc(_len + _align - 1);
	    }
		
	    ~raw_hack_aligned()
		{
			delete[] _realdata;
			dec_total_alloc(_len + _align - 1);
		}
	    raw* clone_empty()
		{
			return new raw_hack_aligned(_len, _align);
	    }
	};
#endif


#ifdef HAVE_SPLICE
	class raw_pipe : public raw
	{
	private:
		bool _source_consumed;
		int _pipefds[2];
		
	public:
		explicit raw_pipe(uint32_t len) : raw(len), _source_consumed(false)
		{
			size_t max = get_max_pipe_size();
			if (len > max)
			{
				THROW_SYSCALL_EXCEPTION("length larger than max pipe size", -1, "raw_pipe");
			}
			_pipefds[0] = -1;
			_pipefds[1] = -1;

			int r;
			if (pipe(_pipefds) == -1)
			{
				THROW_SYSCALL_EXCEPTION(NULL, errno, "pipe");
			}

			r = set_nonblocking(_pipefds);
			if (r < 0)
			{
				THROW_SYSCALL_EXCEPTION(NULL, errno, "set_nonblocking");
			}

			r = set_pipe_size(_pipefds, len);
			if (r < 0)
			{
			}

			inc_total_alloc(len);
			inc_history_alloc(len);
	    }

		~raw_pipe()
		{
			if (_data)
				free(_data);
			close_pipe(_pipefds);
			dec_total_alloc(_len);
	    }

	    bool can_zero_copy() const
		{
			return true;
	    }

	    int set_source(int fd, loff_t* off)
		{
			int flags = SPLICE_F_NONBLOCK;
			ssize_t r = safe_splice(fd, off, _pipefds[1], NULL, _len, flags);
			if (r < 0)
			{
				return r;
			}

			_len = r;
		  
			return 0;
	    }

	    int zero_copy_to_fd(int fd, loff_t* offset)
		{
			int flags = SPLICE_F_NONBLOCK;
			ssize_t r = safe_splice_exact(_pipefds[0], NULL, fd, offset, _len, flags);
			if (r < 0)
			{
				return r;
			}
			
			_source_consumed = true;
			return 0;
	    }

	    raw* clone_empty()
		{
			return NULL;
	    }

	    char* get_data()
		{
			if (_data)
				return _data;
			
			return copy_pipe(_pipefds);
	    }

		
	private:
		int set_pipe_size(int* fds, long length)
		{
#ifdef HAVE_SETPIPE_SZ
			if (fcntl(fds[1], F_SETPIPE_SZ, length) == -1)
			{
				int r = -errno;
				if (r == -EPERM)
				{
		  			update_max_pipe_size();
					THROW_SYSCALL_EXCEPTION("length larger than new max pipe size", r, "fcntl");
				}
				
				return r;
			}
#endif
			return 0;
		}

		int set_nonblocking(int* fds)
		{
			if (fcntl(fds[0], F_SETFL, O_NONBLOCK) == -1)
				return -errno;
			if (fcntl(fds[1], F_SETFL, O_NONBLOCK) == -1)
				return -errno;
			
			return 0;
	    }

		void close_pipe(int* fds)
		{
			if (fds[0] >= 0)
				VOID_TEMP_FAILURE_RETRY(close(fds[0]));
			if (fds[1] >= 0)
				VOID_TEMP_FAILURE_RETRY(close(fds[1]));
		}
	
	    char* copy_pipe(int* fds)
		{
			int tmpfd[2];
			int r;

			if (pipe(tmpfd) == -1)
			{
				THROW_SYSCALL_EXCEPTION(NULL, errno, "pipe");
			}
			
	      	r = set_nonblocking(tmpfd);
			if (r < 0)
			{
				THROW_SYSCALL_EXCEPTION(NULL, r, "set_nonblocking");
			}
			
			r = set_pipe_size(tmpfd, _len);
			if (r < 0)
			{
			}
			
			int flags = SPLICE_F_NONBLOCK;
			if (tee(fds[0], tmpfd[1], _len, flags) == -1)
			{
				r = errno;
				close_pipe(tmpfd);
				THROW_SYSCALL_EXCEPTION(NULL, r, "tee");
			}
			
			_data = (char*)malloc(_len);
			if (!_data)
			{
				close_pipe(tmpfd);
				THROW_SYSCALL_EXCEPTION(NULL, errno, "malloc");
			}
			
			r = safe_read(tmpfd[0], _data, _len);
			if (r < (ssize_t)_len)
			{
				free(_data);
				_data = NULL;
				close_pipe(tmpfd);
				THROW_SYSCALL_EXCEPTION(NULL, r, "safe_read");
			}
			
			close_pipe(tmpfd);
			return _data;
		}
	};
#endif // HAVE_SPLICE

	class raw_char : public raw
	{
	public:
	    explicit raw_char(uint32_t l) : raw(l)
		{
			if (_len)
				_data = new char[_len];
			else
				_data = 0;
			
			inc_total_alloc(_len);
			inc_history_alloc(_len);
	    }
		
	    raw_char(uint32_t l, char* b) : raw(b, l)
		{
			inc_total_alloc(_len);
	    }
		
	    ~raw_char()
		{
			delete[] _data;
			dec_total_alloc(_len);
	    }
		
	    raw* clone_empty()
		{
			return new raw_char(_len);
	    }
	};

	class raw_unshareable : public raw
	{
	public:
		explicit raw_unshareable(uint32_t l) : raw(l)
		{
			if (_len)
				_data = new char[_len];
			else
				_data = 0;
	    }
		
	    raw_unshareable(uint32_t l, char* b) : raw(b, l)
		{
	    }
		
	    raw* clone_empty()
		{
			return new raw_char(_len);
	    }
		
	    bool is_shareable()
		{
			return false;
	    }
		
	    ~raw_unshareable()
		{
			delete[] _data;
	    }
	};


	class raw_static : public raw
	{
	public:
	    raw_static(const char* d, uint32_t l) : raw((char*)d, l) {}
	    ~raw_static() {}
	    raw* clone_empty()
		{
			return new raw_char(_len);
	    }
	};

#if defined(HAVE_XIO)
	class xio_msg_buffer : public raw
	{
	private:
	    XioDispatchHook* _hook;
		
	public:
		xio_msg_buffer(XioDispatchHook* hook, const char* d, uint32_t l) : raw((char*)d, l), _hook(hook->get()){}

	    bool is_shareable() { return false; }
		
	    static void operator delete(void* p)
	    {
			xio_msg_buffer *buf = static_cast<xio_msg_buffer*>(p);
			buf->m_hook->put();
	    }
		
	    raw* clone_empty()
		{
			return new raw_char(_len);
	    }
	 };

	class xio_mempool : public raw
	{
	public:
		struct xio_reg_mem* _mp;
	    xio_mempool(struct xio_reg_mem* mp, uint32_t l) : raw((char*)mp->addr, l), _mp(mp)
	    {}
		
	    ~xio_mempool() {}
		
	    raw* clone_empty()
		{
			return new raw_char(_len);
	    }
	};

	struct xio_reg_mem* get_xio_mp(const ptr& bp)
	{
		xio_mempool *mb = dynamic_cast<xio_mempool*>(bp.get_raw());
		if (mb)
		{
			return mb->mp;
	    }
		
	    return NULL;
	}

	raw* create_msg(uint32_t len, char* buf, XioDispatchHook* hook)
	{
		XioPool& pool = hook->get_pool();
	    raw* bp = static_cast<raw*>(pool.alloc(sizeof(xio_msg_buffer)));
	    new (bp)xio_msg_buffer(hook, buf, len);
	    return bp;
	}
#endif /* HAVE_XIO */


	raw* copy(const char* c, uint32_t len)
	{
		raw* r = create_aligned(len, sizeof(size_t));
	    memcpy(r->_data, c, len);
	    return r;
	}

	raw* create(uint32_t len)
	{
		return create_aligned(len, sizeof(size_t));
	}

	raw* claim_char(uint32_t len, char* buf)
	{
	    return new raw_char(len, buf);
	}

	raw* create_malloc(uint32_t len)
	{
		return new raw_malloc(len);
	}

	raw* claim_malloc(uint32_t len, char* buf)
	{
		return new raw_malloc(len, buf);
	}

	raw* create_static(uint32_t len, char* buf)
	{
	    return new raw_static(buf, len);
	}

	raw* create_aligned(uint32_t len, uint32_t align)
	{
		int page_size = PAGE_SIZE;

		// page_size - 1 -> 0FFF
		// 对齐系数是page_size的倍数
		if ((align & (page_size - 1)) == 0 || len >= page_size * 2)
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

	raw* create_zero_copy(uint32_t len, int fd, int64_t* offset)
	{
#ifdef HAVE_SPLICE
		raw_pipe* buf = new raw_pipe(len);
	    int r = buf->set_source(fd, (loff_t*)offset);
	    if (r < 0)
		{
			delete buf;
			THROW_SYSCALL_EXCEPTION(NULL, r, "safe_splice");
	    }
	    return buf;
#else
		THROW_SYSCALL_EXCEPTION(NULL, -ENOTSUP, "safe_splice");
#endif
	}

	raw* create_unshareable(uint32_t len)
	{
		return new raw_unshareable(len);
	}

	class dummy_raw : public raw
	{
	public:
		dummy_raw() : raw(UINT_MAX)
	    {}
		
	    virtual raw* clone_empty()
		{
			return new dummy_raw();
	    }
	};

	raw* create_dummy()
	{
		return new dummy_raw();
	}


	ptr::ptr(raw* r) : _raw(r), _off(0), _len(r->_len)
	{
		atomic_inc(&(r->_ref));
	}

	ptr::ptr(uint32_t l) : _off(0), _len(l)
	{
		_raw = create(l);
		atomic_inc(&(_raw->_ref));
	}

	ptr::ptr(const char* d, uint32_t l) : _off(0), _len(l)
	{
		_raw = copy(d, l);
		atomic_inc(&(_raw->_ref));
	}

	ptr::ptr(const ptr& p) : _raw(p._raw), _off(p._off), _len(p._len)
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

	ptr::ptr(const ptr& p, uint32_t o, uint32_t l) : _raw(p._raw), _off(p._off + o), _len(l)
	{
		atomic_inc(&(_raw->_ref));
	}

	ptr& ptr::operator =(const ptr& p)
	{
		if (p._raw)
		{
			atomic_inc(&(p._raw->_ref));
		}
		
		raw* raw = p._raw; 
		release();
		
		if (raw)
		{
			_raw = raw;
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

	ptr& ptr::operator =(ptr&& p)
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

	raw* ptr::clone()
	{
		return _raw->clone();
	}

	ptr& ptr::make_shareable()
	{
		if (_raw && !_raw->is_shareable())
		{
			raw* tr = _raw;
			_raw = tr->clone();
			atomic_set(&(_raw->_ref), 1);

			atomic_dec(&(tr->_ref));
			// 不太可能为0
			if (unlikely(0 == tr->_ref))
			{
				delete tr;
			}
			else
			{
			}
		}
		
		return *this;
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

	void ptr::release()
	{
		if (_raw)
		{
			atomic_dec(&(_raw->_ref));
				
			if (0 == _raw->_ref)
			{
				delete _raw;
			}
			else
			{
			}
			
			_raw = 0;
		}
	}

	bool ptr::at_buffer_tail() const
	{
		return _off + _len == _raw->_len;
	}

	const char* ptr::c_str() const
	{
		if (buffer_track_c_str)
			atomic_inc(&buffer_c_str_accesses);
		
		return _raw->get_data() + _off;
	}

	char* ptr::c_str()
	{
		if (buffer_track_c_str)
			atomic_inc(&buffer_c_str_accesses);
		
		return _raw->get_data() + _off;
	}

	uint32_t ptr::unused_tail_length() const
	{
		if (_raw)
			return _raw->_len - (_off + _len);
		else
			return 0;
	}

	const char& ptr::operator [](uint32_t n) const
	{
		return _raw->get_data()[_off + n];
	}

	char& ptr::operator [](uint32_t n)
	{
		return _raw->get_data()[_off + n];
	}

	const char* ptr::raw_c_str() const 
	{
		return _raw->_data;
	}

	unsigned ptr::raw_length() const
	{
		return _raw->_len;
	}

	int ptr::raw_nref() const 
	{
		return atomic_read(&(_raw->_ref));
	}

	void ptr::copy_out(uint32_t o, uint32_t l, char* dest) const
	{
		if (o + l > _len)
			THROW_SYSCALL_EXCEPTION(NULL, -1, "copy_out");
		
		char* src =  _raw->_data + _off + o;
		maybe_inline_memcpy(dest, src, l, 8);
	}

	uint32_t ptr::wasted()
	{
		return _raw->_len - _len;
	}

	int ptr::cmp(const ptr& o) const
	{
		int l = _len < o._len ? _len : o._len;
		if (l)
		{
			int r = memcmp(c_str(), o.c_str(), l);
			if (r)
				return r;
		}
		
		if (_len < o._len)
			return -1;
		
		if (_len > o._len)
			return 1;
		
		return 0;
	}

	bool ptr::is_zero() const
	{
		return mem_is_zero(c_str(), _len);
	}

	uint32_t ptr::append(char c)
	{
		char* ptr = _raw->_data + _off + _len;
		*ptr = c;
		_len++;
		return _len + _off;
	}

	uint32_t ptr::append(const char* p, uint32_t l)
	{
		char* c = _raw->_data + _off + _len;
		maybe_inline_memcpy(c, p, l, 32);
		_len += l;
		return _len + _off;
	}

	void ptr::copy_in(uint32_t o, uint32_t l, const char* src)
	{
		copy_in(o, l, src, true);
	}

	void ptr::copy_in(uint32_t o, uint32_t l, const char *src, bool crc_reset)
	{
		char* dest = _raw->_data + _off + o;
		if (crc_reset)
			_raw->invalidate_crc();
		
		maybe_inline_memcpy(dest, src, l, 64);
	}

	void ptr::zero()
	{
		zero(true);
	}

	void ptr::zero(bool crc_reset)
	{
		if (crc_reset)
			_raw->invalidate_crc();
		
		memset(c_str(), 0, _len);
	}

	void ptr::zero(uint32_t o, uint32_t l)
	{
		zero(o, l, true);
	}

	void ptr::zero(uint32_t o, uint32_t l, bool crc_reset)
	{
		if (crc_reset)
			_raw->invalidate_crc();
		
		memset(c_str() + o, 0, l);
	}

	bool ptr::can_zero_copy() const
	{
		return _raw->can_zero_copy();
	}

	int ptr::zero_copy_to_fd(int fd, int64_t* offset) const
	{
		return _raw->zero_copy_to_fd(fd, (loff_t*)offset);
	}

	template <bool is_const>
	buffer::iterator_impl<is_const>::iterator_impl(bl_t* l, uint32_t o) : _bl(l), _ls(&_bl->_buffers), _off(0), _iter(_ls->begin()), _p_off(0)
	{
		advance(o);
	}

	template <bool is_const>
	buffer::iterator_impl<is_const>::iterator_impl(const buffer::iterator& i) : iterator_impl<is_const>(i._bl, i._off, i._iter, i._p_off) {}

	template<bool is_const>
	void buffer::iterator_impl<is_const>::advance(ssize_t o)
	{
		if (0 < o)
		{
			_p_off += o;
			while (0 < _p_off)
			{
				if (_iter == _ls->end())
					THROW_SYSCALL_EXCEPTION(NULL, -1, "advance");
				
				if (_p_off >= _iter->length())
				{
					_p_off -= _iter->length();
					_iter++;
				}
				else
				{
					break;
				}
			}
			
			_off += o;
			
			return;
		}

		
		while (0 > o) 
		{
			if (_p_off)
			{
				uint32_t d = -o;
				if (d > _p_off)
					d = _p_off;
				
				_p_off -= d;
				_off -= d;
				o += d;
			}
			else if (0 < _off)
			{
				_iter--;
				_p_off = _iter->length();
			}
			else
			{
				THROW_SYSCALL_EXCEPTION(NULL, -1, "advance");
			}
		}
	}

	template<bool is_const>
	void buffer::iterator_impl<is_const>::seek(size_t o)
	{
		_iter = _ls->begin();
		_off = _p_off = 0;
		advance(o);
	}

	template<bool is_const>
	char buffer::iterator_impl<is_const>::operator *() const
	{
		if (_iter == _ls->end())
			THROW_SYSCALL_EXCEPTION(NULL, -1, "operator*");
		
		return (*_iter)[_p_off];
	}

	template<bool is_const>
	buffer::iterator_impl<is_const>& buffer::iterator_impl<is_const>::operator ++()
	{
		if (_iter == _ls->end())
			THROW_SYSCALL_EXCEPTION(NULL, -1, "operator++");
		
		advance(1);
		
		return *this;
	}

	template<bool is_const>
	ptr buffer::iterator_impl<is_const>::get_current_ptr() const
	{
		if (_iter == _ls->end())
			THROW_SYSCALL_EXCEPTION(NULL, -1, "get_current_ptr");
		
		return ptr(*_iter, _p_off, _iter->length() - _p_off);
	}

	template<bool is_const>
	void buffer::iterator_impl<is_const>::copy(uint32_t len, char* dest)
	{
		if (_iter == _ls->end())
			seek(_off);
		
		while (0 < len)
		{
			if (_iter == _ls->end())
				THROW_SYSCALL_EXCEPTION(NULL, -1, "copy");

			uint32_t howmuch = _iter->length() - _p_off;
			if (len < howmuch)
				howmuch = len;
			
			_iter->copy_out(_p_off, howmuch, dest);
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
		if (_iter == _ls->end())
			seek(_off);
		
		while (0 < len)
		{
			if (_iter == _ls->end())
				THROW_SYSCALL_EXCEPTION(NULL, -1, "copy");

			uint32_t howmuch = _iter->length() - _p_off;
			if (len < howmuch)
				howmuch = len;
			
			dest.append(*_iter, _p_off, howmuch);

			len -= howmuch;
			advance(howmuch);
		}
	}

	template<bool is_const>
	void buffer::iterator_impl<is_const>::copy(uint32_t len, std::string& dest)
	{
		if (_iter == _ls->end())
			seek(_off);
		
		while (0 < len)
		{
			if (_iter == _ls->end())
				THROW_SYSCALL_EXCEPTION(NULL, -1, "copy");

			unsigned howmuch = _iter->length() - _p_off;
			const char *c_str = _iter->c_str();
			if (len < howmuch)
				howmuch = len;
			
			dest.append(c_str + _p_off, howmuch);

			len -= howmuch;
			advance(howmuch);
		}
	}

	template<bool is_const>
	void buffer::iterator_impl<is_const>::copy_all(buffer& dest)
	{
		if (_iter == _ls->end())
			seek(_off);
		
		while (1)
		{
			if (_iter == _ls->end())
				return;

			unsigned howmuch = _iter->length() - _p_off;
			const char *c_str = _iter->c_str();
			dest.append(c_str + _p_off, howmuch);

			advance(howmuch);
		}
	}

	template<bool is_const>
	size_t buffer::iterator_impl<is_const>::get_ptr_and_advance(size_t want, const char** data)
	{
		if (_iter == _ls->end())
		{
			seek(_off);
			
			if (_iter == _ls->end())
			{
				return 0;
			}
		}
		
		*data = _iter->c_str() + _p_off;
		size_t l = MIN(_iter->length() - _p_off, want);
		_p_off += l;
		if (_p_off == _iter->length())
		{
			++_iter;
			_p_off = 0;
		}
		
		_off += l;
		
		return l;
	}

	template<bool is_const>
	uint32_t buffer::iterator_impl<is_const>::crc32(size_t length, uint32_t crc)
	{
		length = MIN(length, get_remaining());
		while (0 < length)
		{
			const char* p;
			size_t l = get_ptr_and_advance(length, &p);
			crc = crc32c(crc, (unsigned char*)p, l);
			length -= l;
		}
		
		return crc;
	}

	template class buffer::iterator_impl<true>;
	template class buffer::iterator_impl<false>;

	buffer::iterator::iterator(bl_t* l, uint32_t o) : iterator_impl(l, o)
	{}

	buffer::iterator::iterator(bl_t *l, uint32_t o, list_iter_t ip, unsigned po) : iterator_impl(l, o, ip, po)
	{}

	void buffer::iterator::advance(ssize_t o)
	{
		buffer::iterator_impl<false>::advance(o);
	}

	void buffer::iterator::seek(size_t o)
	{
		buffer::iterator_impl<false>::seek(o);
	}

	char buffer::iterator::operator *()
	{
		if (_iter == _ls->end())
		{
			THROW_SYSCALL_EXCEPTION(NULL, -1, "operator*");
		}
		
		return (*_iter)[_p_off];
	}

	buffer::iterator& buffer::iterator::operator ++()
	{
		buffer::iterator_impl<false>::operator ++();
		
		return *this;
	}

	ptr buffer::iterator::get_current_ptr()
	{
		if (_iter == _ls->end())
		{
			THROW_SYSCALL_EXCEPTION(NULL, -1, "get_current_ptr");
		}
		
		return ptr(*_iter, _p_off, _iter->length() - _p_off);
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
		if (_iter == _ls->end())
			seek(_off);
		
		while (0 < len)
		{
			if (_iter == _ls->end())
				THROW_SYSCALL_EXCEPTION(NULL, -1, "copy_in");

			uint32_t howmuch = _iter->length() - _p_off;
			if (len < howmuch)
				howmuch = len;
			
			_iter->copy_in(_p_off, howmuch, src, crc_reset);

			src += howmuch;
			len -= howmuch;
			advance(howmuch);
		}
	}

	void buffer::iterator::copy_in(uint32_t len, const buffer& other)
	{
		if (_iter == _ls->end())
			seek(_off);
		
		uint32_t left = len;
		
		for (std::list<ptr>::const_iterator i = other._buffers.begin();
			i != other._buffers.end(); ++i)
		{
			uint32_t l = (*i).length();
			if (left < l)
				l = left;
			
			copy_in(l, i->c_str());
			left -= l;
			
			if (left == 0)
				break;
		}
	}


	buffer::buffer(buffer&& other) : _buffers(std::move(other._buffers)), _len(other._len),
				_memcopy_count(other._memcopy_count), _last_p(this)
	{
		_append_buffer.swap(other._append_buffer);
		other.clear();
	}

	void buffer::swap(buffer& other)
	{
		std::swap(_len, other._len);
		std::swap(_memcopy_count, other._memcopy_count);
		_buffers.swap(other._buffers);
		_append_buffer.swap(other._append_buffer);
		_last_p = begin();
		other._last_p = other.begin();
	}

	bool buffer::contents_equal(buffer& other)
	{
		return static_cast<const buffer*>(this)->contents_equal(other);
	}

	bool buffer::contents_equal(const buffer& other) const
	{
		if (length() != other.length())
			return false;

		if (true)
		{
			std::list<ptr>::const_iterator a = _buffers.begin();
			std::list<ptr>::const_iterator b = other._buffers.begin();
			uint32_t aoff = 0, boff = 0;
			while (a != _buffers.end())
			{
				uint32_t len = a->length() - aoff;
				if (len > b->length() - boff)
	  				len = b->length() - boff;
				
				if (0 != memcmp(a->c_str() + aoff, b->c_str() + boff, len))
					return false;
				
				aoff += len;
				if (aoff == a->length())
				{
					aoff = 0;
					++a;
				}
				
				boff += len;
				if (boff == b->length())
				{
					boff = 0;
					++b;
				}
			}
			
			return true;
		}

		if (false)
		{
			buffer::const_iterator me = begin();
			buffer::const_iterator him = other.begin();
			
			while (!me.end())
			{
				if (*me != *him)
					return false;
				
				++me;
				++him;
			}
			
			return true;
		}
	}

	bool buffer::can_zero_copy() const
	{
		for (std::list<ptr>::const_iterator it = _buffers.begin();
			it != _buffers.end(); ++it)
		{
			if (!it->can_zero_copy())
				return false;
		}
			
		return true;
	}

	bool buffer::is_provided_buffer(const char* dst) const
	{
		if (_buffers.empty())
			return false;
		
		return (is_contiguous() && (_buffers.front().c_str() == dst));
	}

	bool buffer::is_aligned(uint32_t align) const
	{
		for (std::list<ptr>::const_iterator it = _buffers.begin();
			it != _buffers.end(); ++it)
		{
			if (!it->is_aligned(align))
				return false;
		}
			
		return true;
	}

	bool buffer::is_n_align_sized(uint32_t align) const
	{
		for (std::list<ptr>::const_iterator it = _buffers.begin();
			it != _buffers.end(); ++it)
		{
			if (!it->is_n_align_sized(align))
				return false;
		}
			
		return true;
	}

	bool buffer::is_aligned_size_and_memory(uint32_t align_size, uint32_t align_memory) const
	{
		for (std::list<ptr>::const_iterator it = _buffers.begin();
			it != _buffers.end(); ++it)
		{
			if (!it->is_aligned(align_memory) || !it->is_n_align_sized(align_size))
				return false;
		}
			
		return true;
	}

	bool buffer::is_zero() const
	{
		for (std::list<ptr>::const_iterator it = _buffers.begin();
			it != _buffers.end(); ++it)
		{
			if (!it->is_zero())
			{
				return false;
			}
		}
			
		return true;
	}

	void buffer::zero()
	{
		for (std::list<ptr>::iterator it = _buffers.begin();
			it != _buffers.end(); ++it)
		{
			it->zero();
		}
	}

	void buffer::zero(uint32_t o, uint32_t l)
	{
	    uint32_t p = 0;
	    for (std::list<ptr>::iterator it = _buffers.begin();
			it != _buffers.end(); ++it)
		{
			if (p + it->length() > o)
			{
				if (p >= o && p+it->length() <= o+l)
				{
					it->zero();
				}
				else if (p >= o)
				{
					it->zero(0, o+l-p);
				}
				else if (p + it->length() <= o+l)
				{
					it->zero(o-p, it->length()-(o-p));
				}
				else
				{
					it->zero(o-p, l);
				}
			}
			
			p += it->length();
			if (o + l <= p)
				break;
		}
	}

	bool buffer::is_contiguous() const
	{
		return &(*_buffers.begin()) == &(*_buffers.rbegin());
	}

	bool buffer::is_n_page_sized() const
	{
		return is_n_align_sized(PAGE_SIZE);
	}

	bool buffer::is_page_aligned() const
	{
		return is_aligned(PAGE_SIZE);
	}

	void buffer::rebuild()
	{
		if (0 == _len)
		{
			_buffers.clear();
			return;
		}
		
		ptr nb;
		
		if ((_len & ~PAGE_MASK) == 0)
			nb = create_page_aligned(_len);
		else
			nb = create(_len);
		
		rebuild(nb);
	}

	void buffer::rebuild(ptr& nb)
	{
		uint32_t pos = 0;
		for (std::list<ptr>::iterator it = _buffers.begin();
			it != _buffers.end(); ++it)
		{
			nb.copy_in(pos, it->length(), it->c_str(), false);
			pos += it->length();
		}
			
		_memcopy_count += pos;
		_buffers.clear();
		if (nb.length())
			_buffers.push_back(nb);
		
		invalidate_crc();
		_last_p = begin();
	}

	bool buffer::rebuild_aligned(uint32_t align)
	{
		return rebuild_aligned_size_and_memory(align, align);
	}

	bool buffer::rebuild_aligned_size_and_memory(uint32_t align_size, uint32_t align_memory)
	{
		uint32_t old_memcopy_count = _memcopy_count;
		std::list<ptr>::iterator p = _buffers.begin();
		while (p != _buffers.end())
		{
			// 如果已经对齐则继续
			if (p->is_aligned(align_memory) && p->is_n_align_sized(align_size))
			{
				++p;
				continue;
			}
			
			buffer unaligned;
			uint32_t offset = 0;
			
			do
			{
				offset += p->length();
				unaligned.push_back(*p);
				_buffers.erase(p++);
			} while (p != _buffers.end() && (!p->is_aligned(align_memory) ||
				!p->is_n_align_sized(align_size) || (offset % align_size)));
			
			if (!(unaligned.is_contiguous() && unaligned._buffers.front().is_aligned(align_memory)))
			{
				ptr nb(create_aligned(unaligned._len, align_memory));
				unaligned.rebuild(nb);
				_memcopy_count += unaligned._len;
			}
			
			_buffers.insert(p, unaligned._buffers.front());
		}
		
		_last_p = begin();

		return  (old_memcopy_count != _memcopy_count);
	}

	bool buffer::rebuild_page_aligned()
	{
		return  rebuild_aligned(PAGE_SIZE);
	}

	void buffer::claim(buffer& bl, uint32_t flags)
	{
		clear();
		claim_append(bl, flags);
	}

	void buffer::claim_append(buffer& bl, uint32_t flags)
	{
		_len += bl._len;
		
		if (!(flags & CLAIM_ALLOW_NONSHAREABLE))
			bl.make_shareable();
		
		_buffers.splice(_buffers.end(), bl._buffers);
		bl._len = 0;
		bl._last_p = bl.begin();
	}

	void buffer::claim_prepend(buffer& bl, uint32_t flags)
	{
		_len += bl._len;
		
		if (!(flags & CLAIM_ALLOW_NONSHAREABLE))
			bl.make_shareable();
		
		_buffers.splice(_buffers.begin(), bl._buffers);
		bl._len = 0;
		bl._last_p = bl.begin();
	}

	void buffer::copy(uint32_t off, uint32_t len, char* dest) const
	{
		if (off + len > length())
			THROW_SYSCALL_EXCEPTION(NULL, -1, "copy");
		
		if (_last_p.get_off() != off) 
			_last_p.seek(off);
		
		_last_p.copy(len, dest);
	}

	void buffer::copy(uint32_t off, uint32_t len, buffer& dest) const
	{
		if (off + len > length())
			THROW_SYSCALL_EXCEPTION(NULL, -1, "copy");
		
		if (_last_p.get_off() != off) 
			_last_p.seek(off);
		
		_last_p.copy(len, dest);
	}

	void buffer::copy(uint32_t off, uint32_t len, std::string& dest) const
	{
		if (_last_p.get_off() != off) 
			_last_p.seek(off);
		
		return _last_p.copy(len, dest);
	}

	void buffer::copy_in(uint32_t off, uint32_t len, const char* src)
	{
		copy_in(off, len, src, true);
	}

	void buffer::copy_in(uint32_t off, uint32_t len, const char* src, bool crc_reset)
	{
		if (off + len > length())
			THROW_SYSCALL_EXCEPTION(NULL, -1, "copy_in");
    
		if (_last_p.get_off() != off) 
			_last_p.seek(off);
		
		_last_p.copy_in(len, src, crc_reset);
	}

	void buffer::copy_in(uint32_t off, uint32_t len, const buffer& src)
	{
		if (_last_p.get_off() != off) 
			_last_p.seek(off);
		
		_last_p.copy_in(len, src);
	}

	void buffer::append(char c)
	{
	    uint32_t gap = _append_buffer.unused_tail_length();
    	if (!gap)
		{
			_append_buffer = raw_combined::create(BUFFER_APPEND_SIZE);
			_append_buffer.set_length(0);
		}
		
		append(_append_buffer, _append_buffer.append(c) - 1, 1);
	}

	void buffer::append(const char* data, uint32_t len)
	{
		while (0 < len)
		{
			uint32_t gap = _append_buffer.unused_tail_length();
			if (gap)
			{
				if (gap > len) gap = len;
				
				_append_buffer.append(data, gap);
				append(_append_buffer, _append_buffer.end() - gap, gap);
				len -= gap;
				data += gap;
			}
			
			if (len == 0)
				break;
      
			size_t need = ROUND_UP_TO(len, sizeof(size_t)) + sizeof(raw_combined);
			size_t alen = ROUND_UP_TO(need, BUFFER_ALLOC_UNIT) - sizeof(raw_combined);
			_append_buffer = raw_combined::create(alen);
			_append_buffer.set_length(0);
		}
	}

	void buffer::append(const ptr& bp)
	{
		if (bp.length())
			push_back(bp);
	}

	void buffer::append(ptr&& bp)
	{
		if (bp.length())
			push_back(std::move(bp));
	}

	void buffer::append(const ptr& bp, uint32_t off, uint32_t len)
	{
	    if (!_buffers.empty())
		{
			ptr& l = _buffers.back();
			if (l.get_raw() == bp.get_raw() && l.end() == bp.start() + off)
			{
				l.set_length(l.length() + len);
				_len += len;
				return;
			}
		}
		
		push_back(ptr(bp, off, len));
	}

	void buffer::append(const buffer& bl)
	{
		_len += bl._len;
		for (std::list<ptr>::const_iterator p = bl._buffers.begin();
			p != bl._buffers.end(); ++p)
		{
			_buffers.push_back(*p);
		}
	}

	void buffer::append(std::istream& in)
	{
		while (!in.eof())
		{
			std::string s;
			getline(in, s);
			append(s.c_str(), s.length());
			
			if (s.length())
				append("\n", 1);
		}
	}

	void buffer::append_zero(uint32_t len)
	{
		ptr bp(len);
		bp.zero(false);
		append(std::move(bp));
	}

	const char& buffer::operator [](uint32_t n) const
	{
		if (n >= _len)
			THROW_SYSCALL_EXCEPTION(NULL, -1, "operator[]");

		for (std::list<ptr>::const_iterator p = _buffers.begin();
			p != _buffers.end(); ++p)
		{
			if (n >= p->length())
			{
				n -= p->length();
				continue;
			}
			
			return (*p)[n];
		}
	}

	char* buffer::c_str()
	{
		if (_buffers.empty())
			return 0;

		std::list<ptr>::const_iterator iter = _buffers.begin();
		++iter;

		if (iter != _buffers.end())
			rebuild();
		
		return _buffers.front().c_str();
	}

	std::string buffer::to_str() const
	{
		std::string s;
		s.reserve(length());
		for (std::list<ptr>::const_iterator p = _buffers.begin();
			p != _buffers.end(); ++p)
		{
			if (p->length())
			{
				s.append(p->c_str(), p->length());
			}
		}
			
		return s;
	}

	char* buffer::get_contiguous(uint32_t orig_off, uint32_t len)
	{
		if (orig_off + len > length())
			THROW_SYSCALL_EXCEPTION(NULL, -1, "get_contiguous");

		if (0 == len)
		{
			return 0;
		}

		uint32_t off = orig_off;
		std::list<ptr>::iterator curbuf = _buffers.begin();
		while (0 < off && off >= curbuf->length())
		{
			off -= curbuf->length();
			++curbuf;
		}

		if (off + len > curbuf->length()) {
			buffer tmp;
			unsigned l = off + len;

			do
			{
				if (l >= curbuf->length())
					l -= curbuf->length();
				else
					l = 0;
				
				tmp.append(*curbuf);
				curbuf = _buffers.erase(curbuf);

			} while (curbuf != _buffers.end() && 0 < l);

			tmp.rebuild();
			_buffers.insert(curbuf, tmp._buffers.front());
			
			return tmp.c_str() + off;
		}

		_last_p = begin();

		return curbuf->c_str() + off;
	}

	void buffer::substr_of(const buffer& other, uint32_t off, uint32_t len)
	{
		if (off + len > other.length())
			THROW_SYSCALL_EXCEPTION(NULL, -1, "substr_of");

		clear();

		std::list<ptr>::const_iterator curbuf = other._buffers.begin();
		while (0 < off && off >= curbuf->length())
		{
			off -= (*curbuf).length();
			++curbuf;
		}
    
		while (0 < len)
		{
			if (off + len < curbuf->length())
			{
				_buffers.push_back( ptr( *curbuf, off, len ) );
				_len += len;
				break;
			}

			unsigned howmuch = curbuf->length() - off;
			_buffers.push_back(ptr( *curbuf, off, howmuch));
			_len += howmuch;
			len -= howmuch;
			off = 0;
			++curbuf;
		}
	}

	void buffer::splice(uint32_t off, uint32_t len, buffer* claim_by)
	{
		if (0 == len)
			return;

		if (off >= length())
			THROW_SYSCALL_EXCEPTION(NULL, -1, "splice");

		std::list<ptr>::iterator curbuf = _buffers.begin();
		while (0 < off)
		{
			if (off >= (*curbuf).length())
			{
				off -= (*curbuf).length();
				++curbuf;
			}
			else
			{
				break;
			}
		}
    
		if (off)
		{
			_buffers.insert(curbuf, ptr(*curbuf, 0, off));
			_len += off;
		}
    
		while (0 < len)
		{
			if (off + len < (*curbuf).length()) {
				if (claim_by) 
					claim_by->append( *curbuf, off, len );
				
				(*curbuf).set_offset(off+len + (*curbuf).offset());
				(*curbuf).set_length((*curbuf).length() - (len+off));
				_len -= off+len;
				break;
			}
      
			unsigned howmuch = (*curbuf).length() - off;
			if (claim_by) 
				claim_by->append( *curbuf, off, howmuch );
			
			_len -= (*curbuf).length();
			_buffers.erase( curbuf++ );
			len -= howmuch;
			off = 0;
		}
      
		_last_p = begin();
	}

	void buffer::write(uint32_t off, uint32_t len, std::ostream& out) const
	{
		buffer s;
		s.substr_of(*this, off, len);
		for (std::list<ptr>::const_iterator it = s._buffers.begin(); 
			it != s._buffers.end(); ++it)
		{
			if (it->length())
				out.write(it->c_str(), it->length());
		}
	}

	void buffer::encode_base64(buffer& o)
	{
		ptr bp(length() * 4 / 3 + 3);
		int l = armor(bp.c_str(), bp.c_str() + bp.length(), c_str(), c_str() + length());
		bp.set_length(l);
		o.push_back(std::move(bp));
	}

	void buffer::decode_base64(buffer& e)
	{
		ptr bp(4 + ((e.length() * 3) / 4));
		int l = unarmor(bp.c_str(), bp.c_str() + bp.length(), e.c_str(), e.c_str() + e.length());
		if (0 > l)
		{
			std::ostringstream oss;
			oss << "decode_base64: decoding failed:\n";
			hexdump(oss);
			THROW_SYSCALL_EXCEPTION(NULL, -1, "decode_base64");
		}
		
		bp.set_length(l);
		push_back(std::move(bp));
	}

	int buffer::read_file(const char* fn, std::string* error)
	{
		int fd = TEMP_FAILURE_RETRY(open(fn, O_RDONLY));
		if (0 > fd)
		{
			int err = errno;
			std::ostringstream oss;
			oss << "can't open " << fn << ": " << Error::to_string(err);
			*error = oss.str();
			return -err;
		}

		struct stat st;
		memset(&st, 0, sizeof(st));
		if (::fstat(fd, &st) < 0)
		{
			int err = errno;
			std::ostringstream oss;
			oss << "buffer::read_file(" << fn << "): stat error: "
				<< Error::to_string(err);
			*error = oss.str();
			VOID_TEMP_FAILURE_RETRY(close(fd));
			return -err;
		}

		ssize_t ret = read_fd(fd, st.st_size);
		if (0 > ret)
		{
			std::ostringstream oss;
			oss << "buffer::read_file(" << fn << "): read error:"
				<< Error::to_string(ret);
			*error = oss.str();
			VOID_TEMP_FAILURE_RETRY(close(fd));
			return ret;
		}
		else if (ret != st.st_size)
		{
			std::ostringstream oss;
			oss << "buffer::read_file(" << fn << "): warning: got premature EOF.";
			*error = oss.str();
		}
		
		VOID_TEMP_FAILURE_RETRY(close(fd));
		return 0;
	}

	ssize_t buffer::read_fd(int fd, size_t len)
	{
		if (false && 0 == read_fd_zero_copy(fd, len))
		{
			return 0;
		}
		
		ptr bp = create(len);
		ssize_t ret = safe_read(fd, (void*)bp.c_str(), len);
		
		if (0 <= ret)
		{
			bp.set_length(ret);
			append(std::move(bp));
		}
		return ret;
	}

	int buffer::read_fd_zero_copy(int fd, size_t len)
	{
#ifdef HAVE_SPLICE
		try
		{
			append(create_zero_copy(len, fd, NULL));
		}
		catch (SysCallException &e)
		{
			return -EIO;
		}
		
		return 0;
#else
		return -ENOTSUP;
#endif
	}

	int buffer::write_file(const char* fn, int mode)
	{
		int fd = TEMP_FAILURE_RETRY(open(fn, O_WRONLY|O_CREAT|O_TRUNC, mode));
		if (0 > fd)
		{
			int err = errno;
			std::cerr << "buffer::write_file(" << fn << "): failed to open file: "
					<< Error::to_string(err) << std::endl;
			return -err;
		}
		
		int ret = write_fd(fd);
		if (ret)
		{
			std::cerr << "buffer::write_fd(" << fn << "): write_fd error: "
				<< Error::to_string(ret) << std::endl;
			VOID_TEMP_FAILURE_RETRY(close(fd));
			return ret;
		}
		if (TEMP_FAILURE_RETRY(close(fd)))
		{
			int err = errno;
			std::cerr << "buffer::write_file(" << fn << "): close error: "
				<< Error::to_string(err) << std::endl;
			return -err;
		}
		
		return 0;
	}

	static int do_writev(int fd, struct iovec *vec, uint64_t offset, unsigned veclen, unsigned bytes)
	{
		ssize_t r = 0;
		while (0 < bytes)
		{
#ifdef HAVE_PWRITEV
			r = pwritev(fd, vec, veclen, offset);
#else
			r = lseek64(fd, offset, SEEK_SET);
			if (r != offset)
			{
				r = -errno;
				return r;
			}
			
			r = writev(fd, vec, veclen);
#endif
			if (0 > r)
			{
				if (errno == EINTR)
					continue;
				return -errno;
			}

			bytes -= r;
			offset += r;
			if (0 == bytes) break;

			while (0 < r)
			{
				if (vec[0].iov_len <= (size_t)r)
				{
					r -= vec[0].iov_len;
					++vec;
					--veclen;
				}
				else
				{
					vec[0].iov_base = (char *)vec[0].iov_base + r;
					vec[0].iov_len -= r;
					break;
				}
			}
		}
		
		return 0;
	}

	int buffer::write_fd(int fd) const
	{
		if (can_zero_copy())
			return write_fd_zero_copy(fd);

		iovec iov[IOV_MAX];
		int iovlen = 0;
		ssize_t bytes = 0;

		std::list<ptr>::const_iterator p = _buffers.begin();
		while (p != _buffers.end())
		{
			if (0 < p->length())
			{
				iov[iovlen].iov_base = (void *)p->c_str();
				iov[iovlen].iov_len = p->length();
				bytes += p->length();
				iovlen++;
			}
			
			++p;

			if (iovlen == IOV_MAX - 1 || p == _buffers.end())
			{
				iovec *start = iov;
				int num = iovlen;
				ssize_t wrote;
				retry:
					wrote = writev(fd, start, num);
					if (wrote < 0)
					{
						int err = errno;
							if (err == EINTR)
								goto retry;
						return -err;
					}
					
				if (wrote < bytes)
				{
					while ((size_t)wrote >= start[0].iov_len)
					{
						wrote -= start[0].iov_len;
						bytes -= start[0].iov_len;
						start++;
						num--;
					}
					
					if (0 < wrote)
					{
						start[0].iov_len -= wrote;
						start[0].iov_base = (char *)start[0].iov_base + wrote;
						bytes -= wrote;
					}
					
					goto retry;
				}
				
				iovlen = 0;
				bytes = 0;
			}
		}
		return 0;
	}


	int buffer::write_fd(int fd, uint64_t offset) const
	{
		iovec iov[IOV_MAX];

		std::list<ptr>::const_iterator p = _buffers.begin();
		uint64_t left_pbrs = _buffers.size();
		while (left_pbrs)
		{
			ssize_t bytes = 0;
			unsigned iovlen = 0;
			uint64_t size = MIN(left_pbrs, IOV_MAX);
			left_pbrs -= size;
			while (0 < size)
			{
				iov[iovlen].iov_base = (void *)p->c_str();
				iov[iovlen].iov_len = p->length();
				iovlen++;
				bytes += p->length();
				++p;
				size--;
			}

			int r = do_writev(fd, iov, offset, iovlen, bytes);
			if (0 > r)
				return r;
			
			offset += bytes;
		}
		
		return 0;
	}

	void buffer::prepare_iov(std::vector<iovec> *piov) const
	{
		piov->resize(_buffers.size());
		uint32_t n = 0;
		for (std::list<ptr>::const_iterator p = _buffers.begin();
			p != _buffers.end(); ++p, ++n)
		{
			(*piov)[n].iov_base = (void *)p->c_str();
			(*piov)[n].iov_len = p->length();
		}
	}

	int buffer::write_fd_zero_copy(int fd) const
	{
		if (!can_zero_copy())
			return -ENOTSUP;

		int64_t offset = lseek(fd, 0, SEEK_CUR);
		int64_t *off_p = &offset;
		if (offset < 0 && errno != ESPIPE)
			return -errno;
		
		if (errno == ESPIPE)
			off_p = NULL;
		
		for (std::list<ptr>::const_iterator it = _buffers.begin();
			it != _buffers.end(); ++it)
		{
			int r = it->zero_copy_to_fd(fd, off_p);
			if (r < 0)
				return r;
			
			if (off_p)
				offset += it->length();
		}
			
		return 0;
	}

	// 处理成32位
	uint32_t buffer::crc32(uint32_t crc) const
	{
		for (std::list<ptr>::const_iterator it = _buffers.begin();
			it != _buffers.end(); ++it) {
			if (it->length())
			{
				raw* r = it->get_raw();
				std::pair<size_t, size_t> ofs(it->offset(), it->offset() + it->length());
				std::pair<uint32_t, uint32_t> ccrc;
				if (r->get_crc(ofs, &ccrc))
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
					r->set_crc(ofs, std::make_pair(base, crc));
				}
			}
		}
			
		return crc;
	}

	void buffer::invalidate_crc()
	{
		for (std::list<ptr>::const_iterator p = _buffers.begin(); p != _buffers.end(); ++p)
		{
			raw *r = p->get_raw();
			if (r)
			{
				r->invalidate_crc();
			}
		}
	}

	void buffer::write_stream(std::ostream& out) const
	{
		for (std::list<ptr>::const_iterator p = _buffers.begin(); p != _buffers.end(); ++p)
		{
			if (0 < p->length())
			{
				out.write(p->c_str(), p->length());
			}
		}
	}

	void buffer::hexdump(std::ostream& out, bool trailing_newline) const
	{
		if (!length())
			return;

		std::ios_base::fmtflags original_flags = out.flags();

		out.setf(std::ios::right);
		out.fill('0');

		unsigned per = 16;
		bool was_zeros = false;
		bool did_star = false;
		for (uint32_t o = 0; o < length(); o += per)
		{
			bool row_is_zeros = false;
			if (o + per < length())
			{
				row_is_zeros = true;
				for (uint32_t i = 0; i < per && o + i < length(); i++)
				{
					if ((*this)[o+i])
					{
						row_is_zeros = false;
					}
				}
				
				if (row_is_zeros)
				{
					if (was_zeros)
					{
						if (!did_star)
						{
							out << "*\n";
							did_star = true;
						}
						
						continue;
					}
					was_zeros = true;
				}
				else
				{
					was_zeros = false;
					did_star = false;
				}
			}
			
			if (o)
				out << "\n";
			
    		out << std::hex << std::setw(8) << o << " ";

			uint32_t i;
			for (i = 0; i < per && o + i< length(); i++)
			{
				if (i == 8)
					out << ' ';
				out << " " << std::setw(2) << ((unsigned)(*this)[o + i] & 0xff);
			}
			
			for (; i < per; i++)
			{
				if (i == 8)
					out << ' ';
				
				out << "   ";
			}
    
			out << "  |";
			for (i = 0; i < per && o + i < length(); i++)
			{
				char c = (*this)[o+i];
				if (isupper(c) || islower(c) || isdigit(c) || c == ' ' || ispunct(c))
					out << c;
				else
					out << '.';
			}
			
    		out << '|' << std::dec;
		}
		
		if (trailing_newline)
		{
			out << "\n" << std::hex << std::setw(8) << length();
			out << "\n";
		}

		out.flags(original_flags);
	}

	std::ostream& operator <<(std::ostream& out, const raw& r)
	{
		return out << "buffer::raw(" << (void*)r._data << " len " << r._len << " nref " << atomic_read(&r._ref) << ")";
	}

	std::ostream& operator <<(std::ostream& out, const ptr& bp)
	{
		if (bp.have_raw())
			out << "buffer::ptr(" << bp.offset() << "~" << bp.length()
				<< " " << (void*)bp.c_str()
				<< " in raw " << (void*)bp.raw_c_str()
				<< " len " << bp.raw_length()
				<< " nref " << bp.raw_nref() << ")";
		else
			out << "buffer:ptr(" << bp.offset() << "~" << bp.length() << " no raw)";
		
		return out;
	}

	std::ostream& operator <<(std::ostream& out, const buffer& bl)
	{
		out << "buffer::list(len=" << bl.length() << "," << std::endl;

		std::list<ptr>::const_iterator it = bl.buffers().begin();
		while (it != bl.buffers().end())
		{
			out << "\t" << *it;
			if (++it == bl.buffers().end())
				break;
			out << "," << std::endl;
		}
		
		out << std::endl << ")";
		return out;
	}

	std::ostream& operator <<(std::ostream& out, const SysCallException& e)
	{
		return out << e.what();
	}
