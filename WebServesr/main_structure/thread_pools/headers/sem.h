#ifndef SEM_H
#define SEM_H
#include <stdlib.h>
#include<iostream>
#include <semaphore.h>
#include <iostream>
using namespace std;
class sem
{
public:
    sem();
    sem(int num);
    ~sem();
    bool wait();
    bool post();
    int get_sem();
private:
    sem_t m_sem;
};
#endif 