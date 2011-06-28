#ifndef LOCKSTEP_H_
#define LOCKSTEP_H_

#include <pthread.h>
#include <semaphore.h>

class LockStep
{
public:
    LockStep();
    ~LockStep();
    void CheckBoss();
    void LockBoss();
    void UnlockBoss();
    void CheckSpinner();
    void UnlockSpinner();
    void LockSpinner();

private:
    sem_t* boss;
    sem_t* spinner;
    sem_t* lock;
    int semval;
};


#endif
