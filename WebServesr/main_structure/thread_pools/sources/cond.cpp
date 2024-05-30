#include"../headers/cond.h"
cond::cond()
{
    if (pthread_cond_init(&m_cond,NULL)!=0)
    {
        throw std::exception();
    }
}
cond::~cond()
{
    pthread_cond_destroy(&m_cond);
}
bool cond::wait(pthread_mutex_t *m_mutex)
{
    int ret=0;
    ret=pthread_cond_wait(&m_cond,m_mutex);//允许时间变量
    return ret==0;
}
bool cond::timewait(pthread_mutex_t *m_mutex,struct timespec t)
{
    int ret=0;
    ret=pthread_cond_timedwait(&m_cond,m_mutex,&t);//允许事件变量等
    return ret==0;
}
bool cond::signal()
{
    return pthread_cond_signal(&m_cond)==0;
}
bool cond::broadcast()
{
    return pthread_cond_broadcast(&m_cond)==0;
}