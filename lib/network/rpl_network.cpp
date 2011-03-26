#include "rpl.h"
#include "slavenetwork.h"
#include "rpl_network.h"
#include "rpqueue.h"
#include "jabposter.h"

#define MAXLINKS 100

rpl_network::rpl_network()
{
    in_queue = new rpqueue();
    jbp = new jabposter(in_queue);
}


rpl_network::~rpl_network()
{
    free(in_queue);
    free(jbp);
}

void rpl_network::post(Post &post)
{
    jbp->sendpost(&post);
}

std::vector<Link> rpl_network::getLinks()
{
    Link arr_link[MAXLINKS];
    std::vector<Link> links;
    int numret = 0, x;

    numret = jbp->getlinks(arr_link, MAXLINKS);
    for( x = 0; x<numret; x++)
    {
        links.push_back(arr_link[x]);
    }
    return links;
}

void rpl_network::addlink(Link &link)
{
    jbp->addlink(link);
}

Post *rpl_network::getpost()
{
    return this->in_queue->get();
}

void rpl_network::add_jab(string user, string pass, string post)
{
     this->jbp->add_jab(user,pass,post);
}

void rpl_network::add_bon(string user)
{
     this->jbp->add_bon(user);
}

void rpl_network::go()
{
    this->jbp->go();
}

void rpl_network::stop()
{
}
