#include "rpl.h"
#include "slavenetwork.h"
#include "rpl_network.h"
#include "rpqueue.h"
#include "jabposter.h"
#include "rpdebug.h"

#define MAXLINKS 100
#define MAXACCS  100

rpl_network::rpl_network(std::string repostdir)
{
    in_queue = new rpqueue<Post*>();
    jbp = new JabPoster(in_queue, repostdir);
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
    jbp->SendPost(&post);
}

std::vector<Link> rpl_network::getLinks()
{
    Link arr_link[MAXLINKS];
    std::vector<Link> links;
    int numret = 0, x = 0;

    numret = jbp->GetLinks(arr_link, MAXLINKS);
    for( x = 0; x<numret; x++)
    {
        links.push_back(arr_link[x]);
    }
    return links;
}

void rpl_network::addLink(Link& link)
{
    jbp->AddLink(link);
}

void rpl_network::rmLink(Link& link)
{
    jbp->RmLink(link);
}

std::vector<Account> rpl_network::getAccounts()
{
    Account arr_acc[MAXACCS];
    std::vector<Account> accts;
    int numret = 0, x = 0;

    numret = jbp->GetAccounts(arr_acc, MAXACCS);
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
        this->jbp->AddJabber(acct.user(),acct.pass());
    }
    else if(acct.type() == "Gtalk")
    {
        this->jbp->AddGtalk(acct.user(),acct.pass());
    }
    else if(acct.type() == "Bonjour")
    {
        this->jbp->AddBonjour(acct.user());
    }
}

void rpl_network::rmAccount(Account& acct)
{
    this->jbp->RmAccount(acct);
}

std::string rpl_network::get_userdir()
{
    string rpdir = this->jbp->GetRepostDir();
    return rpdir;
}

void rpl_network::go()
{
    this->jbp->Go();
}

void rpl_network::stop()
{
    if(this->jbp)
    {
        this->jbp->Stop();
    }
}
