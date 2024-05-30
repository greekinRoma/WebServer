#ifndef THREADPOOL_H
#define THREADPOOL_H
#include <pthread.h>
#include <exception>
#include <list>
#include <iostream>
#include "./sem.h"
#include "./locker.h"
#include "./sem.h"
#include "./cond.h"
using namespace std;
template<typename T>
class threadpool
{
public:
    threadpool(const int & thread_num=8,const int & request_num=10000);
    ~threadpool();
    append(T* request);
private:
    bool close();
    bool reset();
    static void* worker(void* arg);
    void run();
private:
    std::list<T> m_queue;
    sem m_queue_sem;
    locker m_queue_locker;
    int m_thread_num;
    int m_request_num;
    bool m_stop;
    thread_t *m_threads;
};
template<typename T>
threadpool<T>::threadpool(const int &thread_num,const int & request_num)
{
    m_stop=false;
    reset();
}
template<typename T>
threadpool<T>::~threadpool()
{
    m_stop=true;
    close()
}
template<typename T>
bool threadpool<T>::reset()
{
    if (m_threads)
    {
        
    }
}
template<typename T>
bool threadpool<T>::close()
{

}
#endif