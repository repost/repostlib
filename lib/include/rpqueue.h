#ifndef RPQUEUE_H_
#define RPQUEUE_H_

#include "rpsemaphore.h"

#define QUEUE_SIZE 64

template <class obj>
class rpqueue
{
public:
    rpqueue();
    ~rpqueue();
    void add(obj newobj);
    obj get();

private:
    RpSemaphore *empty_;
    RpSemaphore *lock_;
    RpSemaphore *usd_;
    obj rpq[QUEUE_SIZE];
};

#endif
