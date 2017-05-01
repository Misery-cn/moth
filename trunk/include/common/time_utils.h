#ifndef _TIME_UTILS_H_
#define _TIME_UTILS_H_

#include <time.h>
#include <string>
#include <iomanip> // setw
#include <math.h> // trunc
#include "encoding.h"
#include "strtol.h"

// UTILS_NS_BEGIN


extern int32_t is_leap(int32_t year);
extern int32_t days_from_0(int32_t year);
extern int32_t days_from_1970(int32_t year);
extern int32_t days_from_1jan(int32_t year, int32_t month, int32_t day);
extern time_t internal_timegm(tm const* t);

struct utime_t
{
	struct
	{
		// 秒
		uint32_t tv_sec;
		// 纳秒
		uint32_t tv_nsec;
	} _tv;

	utime_t() { _tv.tv_sec = 0; _tv.tv_nsec = 0; }
	
	utime_t(time_t s, int n) { _tv.tv_sec = s; _tv.tv_nsec = n; normalize(); }
	
	utime_t(const struct timespec v)
	{
		_tv.tv_sec = v.tv_sec;
		_tv.tv_nsec = v.tv_nsec;
	}
	
	utime_t(const struct timeval& v)
	{
		set_from_timeval(&v);
	}
	
	utime_t(const struct timeval* v)
	{
		set_from_timeval(v);
	}

	bool is_zero() const
	{
		return ((0 == _tv.tv_sec) && (0 == _tv.tv_nsec));
	}

	void normalize()
	{
		if (1000000000ul < _tv.tv_nsec)
		{
			_tv.tv_sec += _tv.tv_nsec / (1000000000ul);
			_tv.tv_nsec %= 1000000000ul;
		}
	}

	void to_timespec(struct timespec* ts) const
	{
		ts->tv_sec = _tv.tv_sec;
		ts->tv_nsec = _tv.tv_nsec;
	}
	
	void set_from_double(double d)
	{ 
		_tv.tv_sec = (uint32_t)trunc(d);
		_tv.tv_nsec = (uint32_t)((d - (double)_tv.tv_sec) * (double)1000000000.0);
	}

	// 秒
	time_t sec() const { return _tv.tv_sec; }

	// 微秒
	long usec() const { return _tv.tv_nsec/1000; }

	// 纳秒
	int nsec() const { return _tv.tv_nsec; }

	uint32_t& sec_ref()  { return _tv.tv_sec; }
	uint32_t& nsec_ref() { return _tv.tv_nsec; }

	// 转成纳秒
	uint64_t to_nsec() const
	{
		return (uint64_t)_tv.tv_nsec + (uint64_t)_tv.tv_sec * 1000000000ull;
	}
	
	// 转成毫秒
	uint64_t to_msec() const
	{
		return (uint64_t)_tv.tv_nsec / 1000000ull + (uint64_t)_tv.tv_sec * 1000ull;
	}

	void copy_to_timeval(struct timeval* v) const
	{
		v->tv_sec = _tv.tv_sec;
		v->tv_usec = _tv.tv_nsec / 1000;
	}
	
	void set_from_timeval(const struct timeval* v)
	{
		_tv.tv_sec = v->tv_sec;
		_tv.tv_nsec = v->tv_usec * 1000;
	}

	void encode(buffer& buf) const
	{
#if defined(LITTLE_ENDIAN)
		buf.append((char *)(this), sizeof(uint32_t) + sizeof(uint32_t));
#else
		encode(_tv.tv_sec, buf);
		encode(_tv.tv_nsec, buf);
#endif
	}
	void decode(buffer::iterator& p)
	{
#if defined(LITTLE_ENDIAN)
		p.copy(sizeof(uint32_t) + sizeof(uint32_t), (char *)(this));
#else
		decode(_tv.tv_sec, p);
		decode(_tv.tv_nsec, p);
#endif
	}


	utime_t round_to_minute()
	{
		struct tm m;
		time_t t = sec();
		localtime_r(&t, &m);
		m.tm_sec = 0;
		t = mktime(&m);
		return utime_t(t, 0);
	}

	utime_t round_to_hour()
	{
		struct tm m;
		time_t t = sec();
		localtime_r(&t, &m);
		m.tm_sec = 0;
		m.tm_min = 0;
		t = mktime(&m);
		return utime_t(t, 0);
	}

	operator double() const
	{
		return (double)sec() + ((double)nsec() / 1000000000.0L);
	}

	void sleep() const
	{
		struct timespec ts;
		to_timespec(&ts);
		nanosleep(&ts, NULL);
	}

	std::ostream& gmtime(std::ostream& out) const
	{
		out.setf(std::ios::right);
		char oldfill = out.fill();
		out.fill('0');
		if (sec() < ((time_t)(60*60*24*365*10)))
		{
			out << (long)sec() << "." << std::setw(6) << usec();
		} 
		else
		{
			struct tm m;
			time_t t = sec();
			gmtime_r(&t, &m);
			out << std::setw(4) << (m.tm_year+1900)
			<< '-' << std::setw(2) << (m.tm_mon+1)
			<< '-' << std::setw(2) << m.tm_mday
			<< ' '
			<< std::setw(2) << m.tm_hour
			<< ':' << std::setw(2) << m.tm_min
			<< ':' << std::setw(2) << m.tm_sec;
			out << "." << std::setw(6) << usec();
			out << "Z";
		}
		out.fill(oldfill);
		out.unsetf(std::ios::right);
		return out;
	}

	std::ostream& gmtime_nsec(std::ostream& out) const
	{
		out.setf(std::ios::right);
		char oldfill = out.fill();
		out.fill('0');
		if (sec() < ((time_t)(60*60*24*365*10)))
		{
			out << (long)sec() << "." << std::setw(6) << usec();
		}
		else
		{
			struct tm m;
			time_t t = sec();
			gmtime_r(&t, &m);
			out << std::setw(4) << (m.tm_year+1900)  // 2007 -> '07'
			<< '-' << std::setw(2) << (m.tm_mon+1)
			<< '-' << std::setw(2) << m.tm_mday
			<< ' '
			<< std::setw(2) << m.tm_hour
			<< ':' << std::setw(2) << m.tm_min
			<< ':' << std::setw(2) << m.tm_sec;
			out << "." << std::setw(9) << nsec();
			out << "Z";
		}
		out.fill(oldfill);
		out.unsetf(std::ios::right);
		return out;
	}

	std::ostream& asctime(std::ostream& out) const
	{
		out.setf(std::ios::right);
		char oldfill = out.fill();
		out.fill('0');
		if (sec() < ((time_t)(60*60*24*365*10)))
		{
			out << (long)sec() << "." << std::setw(6) << usec();
		}
		else
		{
			struct tm m;
			time_t t = sec();
			gmtime_r(&t, &m);

			char buf[128];
			asctime_r(&m, buf);
			int len = strlen(buf);
			if (buf[len - 1] == '\n')
				buf[len - 1] = '\0';
			out << buf;
		}
		out.fill(oldfill);
		out.unsetf(std::ios::right);
		return out;
	}
  
	std::ostream& localtime(std::ostream& out) const
	{
		out.setf(std::ios::right);
		char oldfill = out.fill();
		out.fill('0');
		if (sec() < ((time_t)(60*60*24*365*10)))
		{
			out << (long)sec() << "." << std::setw(6) << usec();
		}
		else
		{
			struct tm m;
			time_t t = sec();
			localtime_r(&t, &m);
			out << std::setw(4) << (m.tm_year+1900)  // 2007 -> '07'
			<< '-' << std::setw(2) << (m.tm_mon+1)
			<< '-' << std::setw(2) << m.tm_mday
			<< ' '
			<< std::setw(2) << m.tm_hour
			<< ':' << std::setw(2) << m.tm_min
			<< ':' << std::setw(2) << m.tm_sec;
			out << "." << std::setw(6) << usec();
		}
		out.fill(oldfill);
		out.unsetf(std::ios::right);
		return out;
	}

	int sprintf(char* out, int outlen) const
	{
		struct tm m;
		time_t t = sec();
		localtime_r(&t, &m);

		return ::snprintf(out, outlen, "%04d-%02d-%02d %02d:%02d:%02d.%06ld",
						m.tm_year + 1900, m.tm_mon + 1, m.tm_mday,
						m.tm_hour, m.tm_min, m.tm_sec, usec());
	}

	static int snprintf(char* out, int outlen, time_t t)
	{
		struct tm m;
		localtime_r(&t, &m);

		return ::snprintf(out, outlen, "%04d-%02d-%02d %02d:%02d:%02d",
						m.tm_year + 1900, m.tm_mon + 1, m.tm_mday,
						m.tm_hour, m.tm_min, m.tm_sec);
	}

	static int parse_date(const std::string& date, uint64_t* epoch, uint64_t* nsec,
                        std::string* out_date = NULL, std::string* out_time = NULL)
	{
	    struct tm m;
	    memset(&m, 0, sizeof(m));

		if (nsec)
			*nsec = 0;

		const char* p = strptime(date.c_str(), "%Y-%m-%d", &m);
		if (p)
		{
			if (*p == ' ')
			{
				p++;
				p = strptime(p, " %H:%M:%S", &m);
				if (!p)
					return -EINVAL;
				
				if (nsec && *p == '.')
				{
					++p;
					unsigned i;
					char buf[10];
					for (i = 0; (i < sizeof(buf) - 1) && isdigit(*p); ++i, ++p)
					{
						buf[i] = *p;
					}
					for (; i < sizeof(buf) - 1; ++i)
					{
						buf[i] = '0';
					}
					buf[i] = '\0';
					std::string err;
					*nsec = (uint64_t)strict_strtol(buf, 10, &err);
					if (!err.empty())
					{
						return -EINVAL;
					}
				}
			}
		} 
		else 
		{
			int sec, usec;
			int r = sscanf(date.c_str(), "%d.%d", &sec, &usec);
			if (r != 2)
			{
				return -EINVAL;
			}

			time_t tt = sec;
			gmtime_r(&tt, &m);

			if (nsec)
			{
				*nsec = (uint64_t)usec * 1000;
			}
		}
	
		time_t t = internal_timegm(&m);
	    if (epoch)
			*epoch = (uint64_t)t;

	    if (out_date)
		{
			char buf[32];
			strftime(buf, sizeof(buf), "%F", &m);
			*out_date = buf;
	    }
		
	    if (out_time)
		{
			char buf[32];
			strftime(buf, sizeof(buf), "%T", &m);
			*out_time = buf;
	    }

		return 0;
	}
	
};

WRITE_CLASS_ENCODER(utime_t);


inline utime_t operator+(const utime_t& l, const utime_t& r)
{
	return utime_t(l.sec() + r.sec() + (l.nsec() + r.nsec()) / 1000000000L,
				(l.nsec() + r.nsec()) % 1000000000L );
}

inline utime_t& operator+=(utime_t& l, const utime_t& r)
{
	l.sec_ref() += r.sec() + (l.nsec()+r.nsec()) / 1000000000L;
	l.nsec_ref() += r.nsec();
	l.nsec_ref() %= 1000000000L;
	return l;
}

inline utime_t& operator+=(utime_t& l, double f)
{
	double fs = trunc(f);
	double ns = (f - fs) * (double)1000000000.0;
	l.sec_ref() += (long)fs;
	l.nsec_ref() += (long)ns;
	l.normalize();
	return l;
}

inline utime_t operator-(const utime_t& l, const utime_t& r)
{
	return utime_t(l.sec() - r.sec() - (l.nsec() < r.nsec() ? 1 : 0),
					l.nsec() - r.nsec() + (l.nsec() < r.nsec() ? 1000000000 : 0) );
}

inline utime_t& operator-=(utime_t& l, const utime_t& r)
{
	l.sec_ref() -= r.sec();
	
	if (l.nsec() >= r.nsec())
	{
		l.nsec_ref() -= r.nsec();
	}
	else
	{
		l.nsec_ref() += 1000000000L - r.nsec();
		l.sec_ref()--;
	}
	
	return l;
}

inline utime_t& operator-=(utime_t& l, double f) 
{
	double fs = trunc(f);
	double ns = (f - fs) * (double)1000000000.0;
	l.sec_ref() -= (long)fs;
	long nsl = (long)ns;
	if (nsl)
	{
		l.sec_ref()--;
		l.nsec_ref() = 1000000000L + l.nsec_ref() - nsl;
	}
	l.normalize();
	return l;
}


inline bool operator>(const utime_t& l, const utime_t& r)
{
	return (l.sec() > r.sec()) || (l.sec() == r.sec() && l.nsec() > r.nsec());
}

inline bool operator<=(const utime_t& l, const utime_t& r)
{
	return !(operator>(l, r));
}

inline bool operator<(const utime_t& l, const utime_t& r)
{
	return (l.sec() < r.sec()) || (l.sec() == r.sec() && l.nsec() < r.nsec());
}

inline bool operator>=(const utime_t& l, const utime_t& r)
{
	return !(operator<(l, r));
}

inline bool operator==(const utime_t& l, const utime_t& r)
{
	return l.sec() == r.sec() && l.nsec() == r.nsec();
}

inline bool operator!=(const utime_t& l, const utime_t& r)
{
	return l.sec() != r.sec() || l.nsec() != r.nsec();
}


inline std::ostream& operator<<(std::ostream& out, const utime_t& t)
{
	return t.localtime(out);
}



class TimeUtils
{
public:
    // 判断是否为同一天
    static bool is_same_day(time_t t1, time_t t2);

    // 判断指定年份是否为闰年
    static bool is_leap_year(int year);
    
    // 将time_t值转换成类似于20160114这样的整数
    static uint32_t time2date(time_t t);

    // 得到当前日期和时间，返回格式由参数format决定，默认为: YYYY-MM-DD HH:SS:MM
	// datetime_buffer:用来存储当前日期和时间的缓冲区
	// datetime_buffer_size:datetime_buffer的字节大小，不应当于小“YYYY-MM-DD HH:SS:MM”
    static void get_current_datetime(char* datetime_buffer, size_t datetime_buffer_size, const char* format = "%04d-%02d-%02d %02d:%02d:%02d");
    static std::string get_current_datetime(const char* format = "%04d-%02d-%02d %02d:%02d:%02d");

	// 得到当前日期，返回格式由参数format决定，默认为: YYYY-MM-DD
	// date_buffer: 用来存储当前日期的缓冲区
	// date_buffer_size: date_buffer的字节大小，不应当于小“YYYY-MM-DD”
    static void get_current_date(char* date_buffer, size_t date_buffer_size, const char* format = "%04d-%02d-%02d");
    static std::string get_current_date(const char* format = "%04d-%02d-%02d");

	// 得到当前时间，返回格式由参数format决定，默认为: HH:SS:MM
	// time_buffer: 用来存储当前时间的缓冲区
	// time_buffer_size: time_buffer的字节大小，不应当于小“HH:SS:MM”
    static void get_current_time(char* time_buffer, size_t time_buffer_size, const char* format = "%02d:%02d:%02d");
    static std::string get_current_time(const char* format = "%02d:%02d:%02d");

    // 得到当前日期和时间结构
    static void get_current_datetime_struct(struct tm* current_datetime_struct);

    // 日期和时间
    static void to_current_datetime(struct tm* current_datetime_struct, char* datetime_buffer, size_t datetime_buffer_size, const char* format = "%04d-%02d-%02d %02d:%02d:%02d");
    static std::string to_current_datetime(struct tm* current_datetime_struct, const char* format = "%04d-%02d-%02d %02d:%02d:%02d");

    // 仅日期
    static void to_current_date(struct tm* current_datetime_struct, char* date_buffer, size_t date_buffer_size, const char* format = "%04d-%02d-%02d");
    static std::string to_current_date(struct tm* current_datetime_struct, const char* format = "%04d-%02d-%02d");

    // 仅时间
    static void to_current_time(struct tm* current_datetime_struct, char* time_buffer, size_t time_buffer_size, const char* format = "%02d:%02d:%02d");
    static std::string to_current_time(struct tm* current_datetime_struct, const char* format = "%02d:%02d:%02d");

    // 仅年份
    static void to_current_year(struct tm* current_datetime_struct, char* year_buffer, size_t year_buffer_size);
    static std::string to_current_year(struct tm* current_datetime_struct);

    // 仅月份
    static void to_current_month(struct tm* current_datetime_struct, char* month_buffer, size_t month_buffer_size);
    static std::string to_current_month(struct tm* current_datetime_struct);

    // 仅天
    static void to_current_day(struct tm* current_datetime_struct, char* day_buffer, size_t day_buffer_size);
    static std::string to_current_day(struct tm* current_datetime_struct);

    // 仅小时
    static void to_current_hour(struct tm* current_datetime_struct, char* hour_buffer, size_t hour_buffer_size);
    static std::string to_current_hour(struct tm* current_datetime_struct);

    // 仅分钟
    static void to_current_minite(struct tm* current_datetime_struct, char* minite_buffer, size_t minite_buffer_size);
    static std::string to_current_minite(struct tm* current_datetime_struct);

    // 仅秒钟
    static void to_current_second(struct tm* current_datetime_struct, char* second_buffer, size_t second_buffer_size);
    static std::string to_current_second(struct tm* current_datetime_struct);
	
	// 将一个字符串转换成日期时间格式
	// str:符合“YYYY-MM-DD HH:MM:SS”格式的日期时间
	// datetime_struct:存储转换后的日期时间
	// 转换成功返回true，否则返回false
    static bool datetime_struct_from_string(const char* str, struct tm* datetime_struct);
    static bool datetime_struct_from_string(const char* str, time_t* datetime);

    // 返回“YYYY-MM-DD HH:MM:SS”格式的日期时间
    static std::string to_string(time_t datetime, const char* format = "%04d-%02d-%02d %02d:%02d:%02d");

    // 返回格式由参数format决定，默认为“YYYY-MM-DD HH:MM:SS”格式的日期时间
    static std::string to_datetime(time_t datetime, const char* format = "%04d-%02d-%02d %02d:%02d:%02d");

    // 返回格式由参数format决定，默认为“YYYY-MM-DD”格式的日期时间
    static std::string to_date(time_t datetime, const char* format = "%04d-%02d-%02d");

    // 返回格式由参数format决定，默认为“HH:MM:SS”格式的日期时间
    static std::string to_time(time_t datetime, const char* format = "%02d:%02d:%02d");

    // 得到当前的微秒值
    static int64_t get_current_microseconds();
};

// 返回今天
extern std::string today(const char* format = "%04d-%02d-%02d");
// 返回昨天
extern std::string yesterday(const char* format = "%04d-%02d-%02d");
// 返回明天
extern std::string tomorrow(const char* format = "%04d-%02d-%02d");

// 取得格式化的当前日期时间，
// 如果with_milliseconds为true，则返回格式为：YYYY-MM-DD hh:mm:ss/ms，其中ms最长为10位数字；
// 如果with_milliseconds为false，则返回格式为：YYYY-MM-DD hh:mm:ss。
//
// 如果with_milliseconds为true则datetime_buffer_size的大小不能小于sizeof("YYYY-MM-DD hh:mm:ss/0123456789")，
// 如果with_milliseconds为false则datetime_buffer_size的大小不能小于sizeof("YYYY-MM-DD hh:mm:ss")，
extern void get_formatted_current_datetime(char* datetime_buffer, size_t datetime_buffer_size, bool with_milliseconds = true);

// 如果with_milliseconds为false，则返回同CTimeUtils::get_current_datetime()
// 如果with_milliseconds为true，则返回为：YYYY-MM-DD hh:mm:ss/milliseconds
extern std::string get_formatted_current_datetime(bool with_milliseconds = true);

extern utime_t clock_now(uint32_t offset = 0);

// UTILS_NS_END

#endif
