#ifndef RPL_H_
#define RPL_H_

#include <string>
#include <vector>

#include "link.h"
#include "post.h"
#include "acct.h"
#include "postuiops.h"
#include "networkuiops.h"

#ifdef WIN32
#define PATH_SEPARATOR "\\"
#else
#define PATH_SEPARATOR "/"
#endif

class rpl_network;
class rpl_storage;
class rpl_con;

class rePoster{
public:
    rePoster(): pnet_(NULL), pstore_(NULL), pcon_(NULL){};
    ~rePoster();
    void init();
    void startRepost();
    void stopRepost();

    void sendPost(Post p);
    
    void setPostUiOps(PostUiOps postuiops); 
    void setNetworkUiOps(NetworkUiOps networkuiops);

    std::vector<Account> getAccounts();
    void addAccount(Account newaccount);
    void rmAccount(Account account);

    std::vector<Link> getLinks();
    void addLink(Link newlink);
    void rmLink(Link link);

    void getInitialPosts();
    void getPosts(int postfrom, int postto);

    std::string getUserDir(void);
    std::vector<std::string> getUserLogs(void);

    void upboat(std::string uuid);
    void downboat(std::string uuid);

private:
    rpl_network *pnet_;
    rpl_storage *pstore_;
    rpl_con *pcon_;
    PostUiOps postuiops_;
    NetworkUiOps networkuiops_;

    void InitUserDir();
};

#endif
