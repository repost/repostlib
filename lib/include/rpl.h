#ifndef RPL_H_
#define RPL_H_

#include <string>
#include <vector>

#include "link.h"
#include "post.h"
#include "acct.h"

class rpl_network;
class rpl_storage;
class rpl_con;

class rePoster{
public:
    class NewPostCB{
    public:
        virtual void Run(const Post&  p)=0;
    };

    rePoster() {};
    void init();
    void startRepost();
    void stopRepost();

    Post newPost();
    void sendPost(Post p);

    void setNewPostCB(NewPostCB *newPostCB){this->newPostCB = newPostCB;}; /* set cb */
    static void cb_wrap(void *reposter, Post *p);   

    Account newAccount(){Account a; return a;};
    std::vector<Account> getAccounts();
    void addAccount(Account newaccount);

    std::vector<Link> getLinks();
    void addLink(Link newlink);

private:
    void cb(Post *p);   
    NewPostCB *newPostCB; /* callback made when we get new post */
    rpl_network *pnet;
    rpl_storage *pstore;
    rpl_con *pcon;

};

#endif
