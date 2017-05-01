#ifndef _STRTOL_H_
#define _STRTOL_H_

#include <string>
#include <limits>
extern "C" 
{
#include <stdint.h>
}

long long strict_strtoll(const char* str, int base, std::string* err);

int strict_strtol(const char* str, int base, std::string* err);

double strict_strtod(const char* str, std::string* err);

float strict_strtof(const char* str, std::string* err);

uint64_t strict_sistrtoll(const char* str, std::string* err);

template<typename T>
T strict_si_cast(const char* str, std::string* err);


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
    	*--buf = '0';
	
	return buf;
}

#endif
