#include <iostream>
#include <vector>
#include <map>

template <class K, class T>
struct Node
{
	K key_;
	T data_;
	Node* prev_;
	Node* next;
};

template <class K, class T>
class LRUCache
{
public:
	LRUCache(size_t size)
	{
		entries_ = new Node<K, T>[size];
		for (size_t i = 0; i < size; i++)
		{
			free_entries_.push_back(entries_ + i);
		}
		head_ = new Node<K, T>;
		tail_ = new Node<K, T>;
		head_->prev_ = NULL;
		head_->next_ = tail_;
		tail_->prev_ = head_;
		tail_->next_ = NULL;
	}
	
	virtual ~LRUCache()
	{
		if (head_)
		{
			delete head_;
			head_ = NULL;
		}
		if (tail_)
		{
			delete tail_;
			tail_ = NULL;
		}
		if (entries_)
		{
			delete[] entries_;
		}
	}
	
	void put(K key, T data)
	{
		Node<K, T>* node = map_[key];
		// 该节点已经存在
		if (node)
		{
			// 取出该节点
			detach(node);
			// 更新数据
			node->data_ = data;
			// 将该节点插入头部
			attach(node);
		}
		else
		{
			// cache已满
			if (free_entries_.empty())
			{
				// 将最后一个node移除
				node = tail_->prev_;
				detach(node);
				map_.erase(node->key_);
			}
			else
			{
				// 分配一个空节点
				node = free_entries_.back();
				free_entries_.pop_back();
			}
			
			node->key_ = key;
            node->data_ = data;
            map_[key] = node;
			// 该节点添加到头部
            attach(node);
		}
	}
	
	T get(K key)
	{
		Node<K, T>* node = map_[key];
		// 存在
		if (node)
		{
			// 取出该节点，并放到链表头部
			detach(node);
			attach(node);
			return node->data_;
		}
		else
		{
			// 如果cache中没有，返回T的默认值。与map行为一致
			return T();
		}
	}
	
private:
	// 分离出node
	void detach(Node<K, T>* node)
	{
		node->prev_->next_ = node->next_;
		node->next_->prev_ = node->prev_;
	}
	
	// 添加到链表头部
	void attach(Node<K, T>* node)
	{
		node->prev_ = head_;
		node->next_ = head_->next_;
		head_->next_ = node;
		node->next_->prev_ = node;
	}

private:
	std::map<K, Node<K, T>*> map_;
	// 存储可用节点地址
	std::vector<Node<K, T>*> free_entries_;
	Node<K, T>* head_;
	Node<K, T>* tail_;
	// 双向链表中的节点
	Node<K, T>* entries_;
};