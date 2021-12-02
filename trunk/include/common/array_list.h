#ifndef _ARRAY_LIST_H_
#define _ARRAY_LIST_H_

#include <iterator>
#include <cstdlib>

template <typename T>
class ArrayList
{       
public:
    struct Item
    {
        T _data;
        Item *_prev;
        Item *_next;
        ArrayList *_list;
      
        Item(T i) : _data(i), _prev(NULL), _next(NULL), _list(NULL)
        {
        
        }
        
        ~Item()
        {
        
        }
    
        Item(const Item& other);
        
        const Item& operator= (const Item& other);
    
      
        ArrayList* get_list() { return _list; }
        
        bool is_on_list() const { return _list ? true : false; }
        
        bool remove_myself()
        {
            if (_list)
            {
                _list->remove(this);
                
                return true;
            }
            else
            {
                return false;
            }
        }
        
        void move_to_front()
        {
            _list->push_front(this);
        }
        
        void move_to_back()
        {
            _list->push_back(this);
        }
    };

    ArrayList(const ArrayList& other)
    {
        _head = other._head;
        _tail = other._tail;
        _size = other._size;
    }

    ArrayList() : _head(0), _tail(0), _size(0)
    {
    
    }
    
    virtual ~ArrayList()
    { 

    }

    uint64_t size() const
    {
        return _size;
    }
    
    bool empty() const
    {
        return (0 == _head); 
    }

    void clear()
    {
        while (_head)
        {
            remove(_head);
        }
    }

    void push_front(Item* i)
    {
        if (i->_list)
        {
            i->_list->remove(i);
        }

        i->_list = this;
        i->_next = _head;
        i->_prev = NULL;

        // 头不为空,则设置前一个元素为新插入的元素
        if (_head)
        {
            _head->_prev = i;
        }
        else
        {
            _tail = i;
        }
        
        _head = i;
        _size++;
    }

    void push_back(Item* i)
    {
        if (i->_list)
        {
            i->_list->remove(i);
        }

        i->_list = this;
        i->_next = NULL;
        i->_prev = _tail;
        
        if (_tail)
        {
            _tail->_next = i;
        }
        else
        {
            _head = i;
        }
        
        _tail = i;
        _size++;
    }

    void remove(Item* i)
    {
        // 如果不是头元素
        if (i->_prev)
        {
            i->_prev->_next = i->_next;
        }
        else
        {
            _head = i->_next;
        }
        
        if (i->_next)
        {
            i->_next->_prev = i->_prev;
        }
        else
        {
            _tail = i->_prev;
        }
        
        _size--;

        i->_list = NULL;
        i->_next = NULL;
        i->_prev = NULL;
    }

    T front() { return static_cast<T>(_head->_data); }
    const T front() const { return static_cast<const T>(_head->_data); }

    T back() { return static_cast<T>(_tail->_data); }
    const T back() const { return static_cast<const T>(_tail->_data); }

    void pop_front()
    {
        remove(_head);
    }
    
    void pop_back()
    {
        remove(_tail);
    }

    class iterator : std::iterator<std::forward_iterator_tag, T>
    {
    private:
        Item* _cur;
    public:
        iterator(Item* i = NULL) : _cur(i)
        {
        }
        
        T operator*() { return static_cast<T>(_cur->_data); }

        // 前++
        iterator& operator++()
        {
            _cur = _cur->_next;
            return *this;
        }

        // // 后++
        // iterator& operator++(int)
        // {
        //        Item tmp = *this;  
        //        ++*this;  
        //        return tmp;  
        // }
        
        bool end() const { return NULL == _cur; }
        
        bool operator==(const iterator& other) const
        {
            return _cur == other._cur;
        }
        
        bool operator!=(const iterator& other) const
        {
            return _cur != other._cur;
        }
    };

    iterator begin() { return iterator(_head); }
    
    iterator end() { return iterator(_tail); }

    class const_iterator : std::iterator<std::forward_iterator_tag, T>
    {
    private:
        Item* _cur;
    public:
        const_iterator(Item* i = NULL) : _cur(i)
        {
        }
        
        const T operator*() { return static_cast<const T>(_cur->_data); }

        // 前++
        const_iterator& operator++()
        {
            _cur = _cur->_next;
            return *this;
        }

        // // 后++
        // const_iterator& operator++(int)
        // {
        //        Item tmp = *this;
        //        ++*this;
        //        return tmp; 
        // }
        
        bool end() const { return NULL == _cur; }
  };

  const_iterator begin() const { return const_iterator(_head); }
  const_iterator end() const { return const_iterator(_tail); }
  
private:
    // 头指针
    Item* _head;
    // 尾指针
    Item* _tail;
    // 当前大小
    volatile uint64_t _size;
    // // 列表总大小
    // uint64_t _max_size;
};

#endif
