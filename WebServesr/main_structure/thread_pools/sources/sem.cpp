#include "../headers/sem.h"
sem::sem()
{
    if (sem_init(&m_sem,0,0)!=0)
    {
        throw std::exception();//0代表所有线程之间同步
    }
}
sem::sem(int num)
{
    if (sem_init(&m_sem,0,num)!=0)//代表相关的线程之间同步
    {
        throw std::exception();
    }
}
sem::~sem()
{
    sem_destroy(&m_sem);
}
bool sem::wait()
{
    cout<<"The number of request is "<<get_sem()<<endl;
    return sem_wait(&m_sem)==0;//等待信号量
}
bool sem::post()
{
    cout<<"The number of request is "<<get_sem()<<endl;
    return sem_post(&m_sem);//增加信号量
}
int sem::get_sem()
{
    int value;
    sem_getvalue(&m_sem,&value);
    return value;
}