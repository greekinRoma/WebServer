#ifndef COND_H
#define COND_H
#include<pthread.h>
#include <exception>
//实现线程之间的等待通知的机制
class cond
{
public:
    cond();
    ~cond();
    bool wait(pthread_mutex_t *m_mutex);
    bool timewait(pthread_mutex_t *m_mutex,struct timespec t);
    bool signal();
    bool broadcast();
private:
    pthread_cond_t m_cond;
    
};
#endif