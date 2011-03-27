#ifndef RPL_NETWORK_H_
#define RPL_NETWORK_H_

#include <vector>

#include "rpl.h"

class rpqueue;
class jabposter;

class rpl_network 
{
public:

    rpl_network();
    ~rpl_network();
    /**
     * @brief Synhcronous call to post a message on the various 
     * registered networks.
     */
    Post *getpost(); /* Blocking call */
    void post(Post &post);

    std::vector<Account> getAccounts();
    void addAccount(Account& acct);
    void rmAccount(Account& acct);

    std::vector<Link> getLinks();
    void addLink(Link& link, Account& acct);
    void rmLink(Link& link);

    /**
     * @brief Blocking call that when released returns a new post
     * from a network. User of function is responsible for deallocation
     *  of memory
     */
    
    void go();
    void stop();
private:
    rpqueue *out_queue;
    rpqueue *in_queue;
    jabposter *jbp;
};

#endif
