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

    explicit ScopedPtr(T* p = NULL) : _ptr(p)
    {
    }

    ~ScopedPtr()
    {
        // 防止T只有声明没有定义
        enum { type_must_be_complete = sizeof(T) };

        if (_ptr)
        {
            delete _ptr;
            _ptr = reinterpret_cast<T*>(-1);
        }
    }

    bool operator!() const
    {
        return (NULL == _ptr);
    }

    void reset(T* p = NULL)
    {
        if (p != _ptr)
        {
            enum { type_must_be_complete = sizeof(T) };

            if (_ptr)
            {
                delete _ptr;
            }
            
            _ptr = p;
        }
    }

    T& operator*() const
    {
        assert(NULL != _ptr);
        
        return *_ptr;
    }

    T* operator->() const
    {
        assert(NULL != _ptr);
        
        return _ptr;
    }

    T* get() const
    {
        return _ptr;
    }

    bool operator==(T* p) const
    {
        return (_ptr == p);
    }

    bool operator!=(T* p) const
    {
        return (_ptr != p);
    }

    void swap(ScopedPtr& p2)
    {
        T* tmp = _ptr;
        _ptr = p2._ptr;
        p2._ptr = tmp;
    }

    T* release()
    {
        T* tmp = _ptr;
        
        _ptr = NULL;

        return tmp;
    }

private:
    ScopedPtr(const ScopedPtr&);
    ScopedPtr& operator=(const ScopedPtr&);
    template <class T2> bool operator==(ScopedPtr<T2> const& p2) const;
    template <class T2> bool operator!=(ScopedPtr<T2> const& p2) const;

private:
    T* _ptr;
};

template <class T>
class ScopedArray
{
public:
    typedef T element_type;

    explicit ScopedArray(T* p = NULL) : _array(p)
    {
    }

    ~ScopedArray()
    {
        enum { type_must_be_complete = sizeof(T) };

        if (_array)
        {
            delete[] _array;
            _array = reinterpret_cast<T*>(-1);
        }
    }

    bool operator!() const
    {
        return (NULL == _array);
    }

    void reset(T* p = NULL)
    {
        if (p != _array)
        {
            enum { type_must_be_complete = sizeof(T) };

            if (_array)
            {
                delete[] _array;
            }
            
            _array = p;
        }
    }

    T& operator [](std::ptrdiff_t i) const
    {
        assert(0 <= i);
        assert(NULL != _array);

        return _array[i];
    }

    T* get() const
    {
        return _array;
    }

    bool operator==(T* p) const
    {
        return (_array == p);
    }
    
    bool operator!=(T* p) const
    {
        return (_array != p);
    }

    void swap(ScopedArray& p2)
    {
        T* tmp = _array;
        _array = p2._array;
        p2._array = tmp;
    }

    T* release()
    {
        T* tmp = _array;
        _array = NULL;

        return tmp;
    }

private:
    ScopedArray(const ScopedArray&);
    ScopedArray& operator=(const ScopedArray&);
    template <class T2> bool operator==(ScopedArray<T2> const& p2) const;
    template <class T2> bool operator!=(ScopedArray<T2> const& p2) const;

private:
    T* _array;
};

// UTILS_NS_END

#endif
