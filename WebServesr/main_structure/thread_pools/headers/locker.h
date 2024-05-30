#ifndef LOCKER_H
#define LOCKER_H
#include<pthread.h>
#include<exception>
class locker
{
public:
    locker();
    ~locker();
    bool lock();
    bool unlock();
    pthread_mutex_t *get();
private:
    pthread_mutex_t m_mutex;
};
#endif