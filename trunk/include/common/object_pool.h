#include "utils.h"

// 要使用对象池,对象必须继承自该类
class PoolObject
{
public:
    
    PoolObject() throw () :_in_pool(false) ,_index(0)
    {
    }

    void set_index(uint32_t index) throw ()
    {
        _index = index;
    }

    uint32_t get_index() const throw ()
    {
        return _index;
    }

    void set_in_pool(bool in_pool) throw ()
    {
        _in_pool = in_pool;
    }

    // 判断对象是否在对象池中
    bool is_in_pool() const throw ()
    {
        return _in_pool;
    }

    // 派生类实现清空对象
    void reset() = 0;

private:
    // 是否在对象池中
    bool _in_pool;
    // 在对象池中的索引
    uint32_t _index;
    ArrayList<PoolObject*>::Item _item;
};


template<class ObjectClass>
class ObjectPool
{
public:
    
    ObjectPool() throw () : _obj_num(0), _idle_num(0), _object_queue(NULL)
    {
    }

    void create(uint32_t object_number) throw ()
    {
        Mutex::Locker locker(_lock);
        
        _obj_num = object_number;
        _idle_num = _obj_num;

        _object_queue = new ArrayList<ObjectClass*>();

        for (uint32_t i = 0; i < _obj_num; ++i)
        {
            ObjectClass* object = new ObjectClass();
            object->set_index(i);
            object->set_in_pool(true);
            _object_queue->push_back(&ObjectClass._item);
        }
    }

    void destory() throw()
    {
        Mutex::Locker locker(_lock);
        
        DELETE_P(_object_queue);
    }

    // 从对象池中获取一个对象
    ObjectClass* borrow() throw ()
    {
        Mutex::Locker locker(_lock);
        
        ObjectClass* object = NULL;
        
        // 对象池不为空
        if (!_object_queue->is_empty())
        {
            // 弹出一个对象,设置对象已不在池中,空闲对象数减一
            object = _object_queue->pop_front();
            object->set_in_pool(false);
            --_idle_num;
        }      

        return object;
    }

    void pay_back(ObjectClass* object) throw ()
    {
        Mutex::Locker locker(_lock);
        
        // 确保对象是不在池中的
        if (!object->is_in_pool())
        {
            object->reset();
            object->set_in_pool(true);

            _object_queue->push_back(object);
            ++_idle_num;
        }
    }

    uint32_t get_pool_size() const throw ()
    {
        return _idle_num;
    }

    volatile uint32_t get_idle_number() const throw ()
    {
        return _idle_num;
    }
        
private:
    // 池中总对象数,包括使用和未使用的
    uint32_t _obj_num;
    // 未使用的对象数
    uint32_t _idle_num;
    
    Mutex _lock;
    // ObjectClass* _object_array;
    ArrayList<ObjectClass*>* _object_queue;
};
