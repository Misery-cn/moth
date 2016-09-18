#ifndef _UTILS_SCOPED_PTR_H_
#define _UTILS_SCOPED_PTR_H_

#include <assert.h>
#include <cstddef>


// UTILS_NS_BEGIN

template <class T> class ScopedPtr;
template <class T> class ScopedArray;

template <class T>
class ScopedPtr
{
public:
    typedef T element_type;

    explicit ScopedPtr(T* p = NULL) : ptr_(p)
    {
    }

    ~ScopedPtr()
    {
    	// 防止T只有声明没有定义
        enum { type_must_be_complete = sizeof(T) };

		if (ptr_)
		{
			delete ptr_;
        	ptr_ = reinterpret_cast<T*>(-1);
		}
    }

    bool operator!() const
    {
        return (NULL == ptr_);
    }

    void reset(T* p = NULL)
    {
        if (p != ptr_)
        {
            enum { type_must_be_complete = sizeof(T) };

			if (ptr_)
			{
            	delete ptr_;
			}
			
            ptr_ = p;
        }
    }

    T& operator*() const
    {
        assert(NULL != ptr_);
		
        return *ptr_;
    }

    T* operator->() const
    {
        assert(NULL != ptr_);
		
        return ptr_;
    }

    T* get() const
    {
        return ptr_;
    }

    bool operator==(T* p) const
    {
        return (ptr_ == p);
    }

    bool operator!=(T* p) const
    {
        return (ptr_ != p);
    }

    void swap(ScopedPtr& p2)
    {
        T* tmp = ptr_;
        ptr_ = p2._ptr;
        p2.ptr_ = tmp;
    }

    T* release()
    {
        T* tmp = ptr_;
		
        ptr_ = NULL;

        return tmp;
    }

private:
    ScopedPtr(const ScopedPtr&);
    ScopedPtr& operator=(const ScopedPtr&);
    template <class T2> bool operator==(ScopedPtr<T2> const& p2) const;
    template <class T2> bool operator!=(ScopedPtr<T2> const& p2) const;

private:
    T* ptr_;
};

template <class T>
class ScopedArray
{
public:
    typedef T element_type;

    explicit ScopedArray(T* p = NULL) : array_(p)
    {
    }

    ~ScopedArray()
    {
        enum { type_must_be_complete = sizeof(T) };

		if (array_)
		{
			delete[] array_;
        	array_ = reinterpret_cast<T*>(-1);
		}
    }

    bool operator!() const
    {
        return (NULL == array_);
    }

    void reset(T* p = NULL)
    {
        if (p != array_)
        {
            enum { type_must_be_complete = sizeof(T) };

			if (array_)
			{
				delete[] array_;
			}
			
            array_ = p;
        }
    }

    T& operator [](std::ptrdiff_t i) const
    {
        assert(0 <= i);
        assert(NULL != array_);

        return array_[i];
    }

    T* get() const
    {
        return array_;
    }

    bool operator==(T* p) const
	{
		return (array_ == p);
	}
	
    bool operator!=(T* p) const
	{
		return (array_ != p);
	}

    void swap(ScopedArray& p2)
    {
        T* tmp = array_;
        array_ = p2._array;
        p2._array = tmp;
    }

    T* release()
    {
        T* tmp = array_;
        array_ = NULL;

        return tmp;
    }

private:
    ScopedArray(const ScopedArray&);
    ScopedArray& operator=(const ScopedArray&);
    template <class T2> bool operator==(ScopedArray<T2> const& p2) const;
    template <class T2> bool operator!=(ScopedArray<T2> const& p2) const;

private:
    T* array_;
};

// UTILS_NS_END

#endif
