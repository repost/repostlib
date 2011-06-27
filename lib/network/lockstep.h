#ifndef LOCKSTEP_H_
#define LOCKSTEP_H_

#include <pthread.h>
#include <semaphore.h>

class lockstep
{
public:
    lockstep();
    ~lockstep();
    void checkBoss();
    void lockBoss();
    void unlockBoss();
    void checkSpinner();
    void unlockSpinner();
    void lockSpinner();

private:
    sem_t* boss;
    sem_t* spinner;
    sem_t* lock;
    int semval;
};


#endif
