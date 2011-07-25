#include <string>
#include <iostream>
#include "rpl_con.h"
#include "rpdebug.h"
#include "postuiops.h"

using namespace std;

rpl_con::rpl_con(rpl_network *net, rpl_storage *store, PostUiOps postuiops):
	pnet_(net), pstore_(store), postuiops_(postuiops), running_(false)
{
}

rpl_con::~rpl_con()
{
}

void rpl_con::stop()
{
    if(running_==true)
    {
         running_=false;
         pthread_join(m_thread,0);
         LOG(DEBUG) << "Consumer thread joined";
    }
}   

void rpl_con::go()
{
    if(running_==false)
    {
        running_ = true;
        pthread_create(&m_thread, 0, (&rpl_con::start_thread), this);
    }
}

void *rpl_con::start_thread(void *obj)
{
    reinterpret_cast<rpl_con *>(obj)->consume();
    return NULL;
}

void rpl_con::consume()
{
    while(running_ == true)
    {
        Post *p = pnet_->getpost();
        if(p)
        {
            if(pstore_->add_post(p))
            {
                LOG(INFO) << "Sending post to UI UUID = " << p->uuid();
                postuiops_.NewPost(p, 0);
            }
            else
            {
                LOG(INFO) << "Already received UUID " << p->uuid();
            }
        }
        else
        {
            LOG(INFO) <<  "Consumer shutting down";
            return;
        }
    }
}
