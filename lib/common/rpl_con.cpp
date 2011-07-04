#include <string>
#include <iostream>
#include "rpl_con.h"

using namespace std;

rpl_con::rpl_con(rpl_network *net, rpl_storage *store,
    void (*cb)(void *rp, Post *p, int rank), void *rp)
{
	this->running = false;
    this->reposter = rp;
    this->npCB = cb;
    this->pnet = net;
    this->pstore = store;
}

rpl_con::~rpl_con()
{
}

void rpl_con::stop()
{
    if(running==true)
    {
         running=false;
         pthread_join(m_thread,0);
    }
}   

void rpl_con::go()
{
    if(running==false)
    {
        running = true;
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
	while(running == true)
	{
		Post *p = pnet->getpost();
		if(p)
		{
			if(this->pstore->add_post(p))
			{
				npCB(reposter,p,0);
			}
			else
			{
				cout << "got it"  << endl;
			}
		}
        else
        {
            cout <<  "Consumer shutting down\n" << endl;
            return;
        }
	}
}
