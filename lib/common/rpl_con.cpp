#include <string>
#include <iostream>
#include "rpl_con.h"
#include "rpdebug.h"

using namespace std;

rpl_con::rpl_con(rpl_network *net, rpl_storage *store):
	pnet_(net), pstore_(store), running_(false)
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
						//LOG(DEBUG) << p->uuid();
						//LOG(DEBUG) << p->content();
            //if(pstore_->add_post(p))
						if(0)
            {
             //   LOG(INFO) << "Sending post to UI UUID = " << p->uuid();
            }
            else
            {
               // LOG(INFO) << "Already received UUID " << p->uuid();
            }
        }
        else
        {
            LOG(INFO) <<  "Consumer shutting down";
            return;
        }
    }
}
