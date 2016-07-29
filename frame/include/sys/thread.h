
#ifndef _SYS_THREAD_H_
#define _SYS_THREAD_H_

#include <pthread.h>
#include "cond.h"

namespace sys
{

class CThread
{
public:
	// �߳�ִ�к���
	static void* thread_proc(void* thread_param);
    // ��ȡ��ǰ�̺߳�
    static uint32_t get_current_thread_id() throw ();

public:
	CThread() throw (sys::CException, sys::CSysCallException);
	virtual ~CThread() throw ();
	
	// ��������ʵ��
	// ����ֱ�ӵ���run,����ֱ�ӵ���,Ӧ�õ���start�����߳�
	virtual void run() = 0;

    // ��stop_��Ա����Ϊtrue���߳̿��Ը���_stop״̬�������Ƿ��˳��߳�      
    // wait_stop: �Ƿ�ȴ��߳̽�����ֻ�е��߳��ǿ�Joinʱ����Ч
    virtual void stop(bool wait_stop = true) throw (sys::CException, sys::CSysCallException);

	// Ĭ������һ����join���߳�
    // detach: �Ƿ��Կɷ���ģʽ�����߳�
	void start(bool detach = false) throw (sys::CException, sys::CSysCallException);

    // �����߳�ջ��С��Ӧ����start֮ǰ���ã�����������Ч�������before_start����
    void set_stack_size(uint32_t stack_size) { stack_size_ = stack_size; }
    
    // �õ��߳�ջ��С�ֽ���
    size_t get_stack_size() const throw (sys::CSysCallException);

    // �õ����̺߳�
    uint32_t get_thread_id() const { return thread_; }
    
    // �ȴ��̷߳���
    void join() throw (sys::CSysCallException);

    // ���߳�����Ϊ�ɷ����
    void detach() throw (sys::CSysCallException);

    // �����߳��Ƿ��join
    bool can_join() const throw (sys::CSysCallException);

    // ����߳������ڵȴ�״̬������
    void wakeup();

protected:

	// �ж��߳��Ƿ�Ӧ���˳���Ĭ�Ϸ���stop_��ֵ
    virtual bool is_stop() const;

	// �߳̿��Ե���������˯��״̬�����ҿ���ͨ������do_wakeup����
    void do_sleep(int milliseconds);

private:
	// �����߳�
    void do_wakeup(bool stop);
	
	// �߳�����ǰ�Ĳ���
	// ��������ʵ��
    virtual bool before_start() { return true; }

	// �߳̽���ǰ�Ĳ���
	// ��������ʵ��
    virtual void before_stop() {}

protected:
	// ������
    CMutex lock_;

private:
	// ��������
    CCond cond_;
	// �Ƿ�ֹͣ�̱߳�ʶ
    volatile bool stop_;
	// �߳�״̬
    volatile enum 
	{
		state_sleeping, 
		state_wakeuped, 
		state_running
	} state_;
	
	// �߳�id
    pthread_t thread_;
    pthread_attr_t attr_;
	// ջ��С
    uint32_t stack_size_;    
};

}

#endif