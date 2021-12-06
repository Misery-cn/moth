#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <unistd.h>
#include <thread>
#include <atomic>
#include <list>
#include "shm_queue.h"

#define THREAD_NUM 2
#define THREAD_SEND_NUM 100000

std::atomic_int read_count;
std::atomic_int write_count;
std::atomic_bool done_flag;

void mul_write_func(ShmQueue* writeQueue, int threadId, const char* mes)
{
    int i = 0;
    while (1)
    {
        if (i >= THREAD_SEND_NUM)
        {
            break;
        }

        const std::string& data = std::to_string(i);
        int ret = writeQueue->send((char*) data.c_str(), data.length());
        if (0 == ret)
        {
            i++;
            write_count++;
            if (write_count >= THREAD_SEND_NUM * THREAD_NUM)
            {
                done_flag.store(true);
            }
        }
        else
        {
            printf("Write failed, ret = %d\n", ret);
            // writeQueue->dump();
            // exit(-1);
        }
    }
    printf("Write  %s thread %d ,write count %d\n", mes, threadId, i);
}

void mul_read_func(ShmQueue* writeQueue, int threadId, const char* mes)
{
    int i = 1;
    while (1)
    {
        char data[100] = {0};
        int len = writeQueue->recv(data);
        if (len > 0)
        {
            i++;
            read_count++;
        }
        else
        {
            if (done_flag)
            {
                break;
            }
            // printf("Read failed ret = %d\n", len);
            // writeQueue->dump();
            // exit(-1);
        }
    }
    printf("Read %s ,thread %d ,read count %d\n", mes, threadId, i);
}


int main()
{
    ShareMemory shm;
    shm.open("/tmp");
    shm.attach();

    ShmQueue shmqueue(&shm);

    /*
    std::list<std::thread> write;
    for (int i = 0; i < THREAD_NUM; i++)
    {
        write.push_back(move(std::thread(mul_write_func, &shmqueue, i, "MulRWTest")));
    }
    */

    std::list<std::thread> read;
    for (int i = 0; i < THREAD_NUM; i++)
    {
        read.push_back(move(std::thread(mul_read_func, &shmqueue, i, "MulRWTest")));
    }

    /*
    for (std::thread& thread : write)
    {
        thread.join();
    }
    */

    for (std::thread& thread : read)
    {
        thread.join();
    }
    
    return 0;
}

