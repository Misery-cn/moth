#ifndef _UTILS_H_
#define _UTILS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "define.h"
#include "const.h"

UTILS_NAMESPACE_BEGIN


template<class Point>
class CPointGuard
{
public:
    CPointGuard(Point* p, bool is_arrary = false);
    virtual ~CPointGuard();
private:
    Point* point_;
	bool is_arrary_;
};

template<class Point>
CPointGuard<Point>::CPointGuard(Point * p, bool is_arrary)
{
	point_ = p;
	is_arrary_ = is_arrary;
}

template<class Point>
CPointGuard<Point>::~CPointGuard()
{
	if (!is_arrary_)
	{
		delete point_;
	}
	else
	{
		delete[] point_;
	}
	
	point_ = NULL;
}

bool get_work_dir(char* buf);

UTILS_NAMESPACE_END

#endif
