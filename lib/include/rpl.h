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

    void sendPost(Post p);

    void setNewPostCB(NewPostCB *newPostCB){this->newPostCB = newPostCB;}; /* set cb */
    static void cb_wrap(void *reposter, Post *p);   

    std::vector<Account> getAccounts();
    void addAccount(Account newaccount);
    void rmAccount(Account account);

    void getInitialPosts(NewPostCB *newPostCB);
    std::vector<Link> getLinks();
    void addLink(Link newlink, Account acct);
    void rmLink(Link link);

private:
    void cb(Post *p);   
    NewPostCB *newPostCB; /* callback made when we get new post */
    rpl_network *pnet;
    rpl_storage *pstore;
    rpl_con *pcon;

};

#endif
