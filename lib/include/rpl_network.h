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
    void post(Post &post);
    void addlink(Link &link);
    std::vector<Link> getLinks();

    /**
     * @brief Blocking call that when released returns a new post
     * from a network. User of function is responsible for deallocation
     *  of memory
     */
    Post *getpost(); /* Blocking call */
    
    void add_jab(std::string user, std::string pass, std::string port);
    void add_bon(std::string user);
    void go();
    void stop();
private:
    rpqueue *out_queue;
    rpqueue *in_queue;
    jabposter *jbp;
};

#endif
