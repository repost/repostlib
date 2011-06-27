#include "rpl.h"
#include "slavenetwork.h"
#include "rpl_network.h"
#include "rpqueue.h"
#include "lockstep.h"
#include "jabposter.h"

#define MAXLINKS 100
#define MAXACCS  100

rpl_network::rpl_network()
{
    in_queue = new rpqueue();
    lock = new lockstep();
    jbp = new jabposter(in_queue, lock);
}

rpl_network::~rpl_network()
{
    delete in_queue;
    delete jbp;
}

Post *rpl_network::getpost()
{
    return this->in_queue->get();
}

void rpl_network::post(Post &post)
{
    lock->lockSpinner(); // lock the spinner
    jbp->sendpost(&post);
    lock->unlockSpinner(); // release the spinner
}

std::vector<Link> rpl_network::getLinks()
{
    Link arr_link[MAXLINKS];
    std::vector<Link> links;
    int numret = 0, x = 0;

    numret = jbp->getlinks(arr_link, MAXLINKS);
    for( x = 0; x<numret; x++)
    {
        links.push_back(arr_link[x]);
    }
    return links;
}

void rpl_network::addLink(Link& link)
{
    jbp->addlink(link);
}

void rpl_network::rmLink(Link& link)
{
    jbp->rmlink(link);
}

std::vector<Account> rpl_network::getAccounts()
{
    Account arr_acc[MAXACCS];
    std::vector<Account> accts;
    int numret = 0, x = 0;

    numret = jbp->getaccounts(arr_acc, MAXACCS);
    for( x = 0; x<numret; x++)
    {
        accts.push_back(arr_acc[x]);
    }
    return accts;
}

void rpl_network::addAccount(Account& acct)
{
    if(acct.type() == "XMPP")
    {
        this->jbp->addJabber(acct.user(),acct.pass());
    }
    else if(acct.type() == "Gtalk")
    {
        this->jbp->addGtalk(acct.user(),acct.pass());
    }
    else if(acct.type() == "Bonjour")
    {
        this->jbp->addBonjour(acct.user());
    }
}

void rpl_network::rmAccount(Account& acct)
{
    this->jbp->rmAccount(acct);
}

std::string rpl_network::get_userdir()
{
    return this->jbp->get_repostdir();
}

void rpl_network::go()
{
    this->jbp->go();
}

void rpl_network::stop()
{
    if(this->jbp)
        this->jbp->stop();
}
