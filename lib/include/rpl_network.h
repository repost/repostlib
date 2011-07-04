#ifndef RPL_NETWORK_H_
#define RPL_NETWORK_H_

#include <vector>
#include <string>

#include "rpl.h"

template <class> class rpqueue;
class JabPoster;
class Post;

class rpl_network 
{
public:
    rpl_network();
    ~rpl_network();
    Post *getpost(); /* Blocking call */
    void post(Post &post);

    std::vector<Account> getAccounts();
    void addAccount(Account& acct);
    void rmAccount(Account& acct);

    std::vector<Link> getLinks();
    void addLink(Link& link);
    void rmLink(Link& link);
    std::string get_userdir();

    void go();
    void stop();
private:
    rpqueue<Post*> *out_queue;
    rpqueue<Post*> *in_queue;
    JabPoster *jbp;
};

#endif
