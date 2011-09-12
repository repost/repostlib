#include "rpl.h"
#include "rpl_network.h"
#include "rpl_storage.h"
#include "rpl_con.h"
#include "rpdebug.h"
#include <string>
#include <iostream>
#include "glib.h"
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef WIN32
#include <dlfcn.h>
#include <dirent.h>
#else
#include "win32/win32dep.h"
#endif

using namespace std;

void rePoster::init() 
{
#ifdef LINUX
    /* We force load libpurple as the chrome plugin loader doesn't 
         do it properly */
    void *handle = dlopen("/usr/lib/libpurple.so",RTLD_LAZY| RTLD_GLOBAL); 
#endif
    InitUserDir();
    InitRepostLogging(getUserDir());
    LOG(INFO) << "Repost home directory - " << getUserDir();
    rpl_storage::init(getUserDir());
    pnet_ = new rpl_network(getUserDir(), networkuiops_);
}

void rePoster::startRepost()
{
    LOG(INFO) << "Starting repost...";

    /* Start the networks */
    pnet_->go();

    /* lets create bonjour user here */
    Account bacct;
    bacct.set_user("reposter");
    bacct.set_type("Bonjour");
    pnet_->addAccount(bacct);

    /* Create storage class here */
    pstore_ = rpl_storage::get_instance();

    /* Create consumer here */
    pcon_ = new rpl_con(pnet_, pstore_, postuiops_);
    pcon_->go();
}

void rePoster::stopRepost()
{
    LOG(INFO) << "Stopping repost";
    pnet_->stop();
    pcon_->stop();
    pstore_->uninit();
}

void rePoster::sendPost(Post p)
{
    LOG(INFO) << "Sending Post UUID " << p.uuid();
    pnet_->post(p);
    pstore_->add_post(&p);
}

void rePoster::setPostUiOps(PostUiOps postuiops)
{
    postuiops_ = postuiops;
}

void rePoster::setNetworkUiOps(NetworkUiOps networkuiops)
{
    networkuiops_ = networkuiops;
}

std::vector<Account> rePoster::getAccounts()
{
    LOG(INFO) << "Get Accounts";
    return pnet_->getAccounts();
}

void rePoster::InitUserDir()
{
    string home(getUserDir());
	if (!g_file_test(home.c_str(), G_FILE_TEST_EXISTS))
	{
		/* Folder doesn't exist so lets create it */
        if(g_mkdir_with_parents(home.c_str(), S_IRUSR | S_IWUSR | S_IXUSR) < 0)
        {
            fprintf(stderr, "COULD NOT CREATE HOME FOLDER"); /* Logging not avail yet */
        }
	}
}

std::string rePoster::getUserDir()
{
    string home(PATH_SEPARATOR ".repost");
#ifndef WIN32
    return home.insert(0, g_get_home_dir());
#else
    char *retval = NULL;
    wchar_t utf_16_dir[MAX_PATH + 1];

    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL,
                    SHGFP_TYPE_CURRENT, utf_16_dir))) 
    {
        retval = g_utf16_to_utf8((const gunichar2 *)utf_16_dir, -1, NULL, NULL, NULL);
    }

    return home.insert(0, retval);
#endif
}

std::vector<std::string> rePoster::getUserLogs()
{
    string path;
    string home(PATH_SEPARATOR ".repost");
    std::vector<std::string> logs;
#ifndef WIN32
    path = home.insert(0, g_get_home_dir());

    static struct dirent *dp;
    DIR *dfd = opendir(path.c_str());

    if ( dfd != NULL )
    {
        while((dp = readdir(dfd)) != NULL)
        {
            string f (dp->d_name);
            if ( f.find("repostlog") != string::npos )
                logs.push_back(dp->d_name);
        }
        closedir(dfd);
    }
#else
    char *retval = NULL;
    wchar_t utf_16_dir[MAX_PATH + 1];

    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL,
                    SHGFP_TYPE_CURRENT, utf_16_dir))) 
    {
        retval = g_utf16_to_utf8((const gunichar2 *)utf_16_dir, -1, NULL, NULL, NULL);
    }

    path = home.insert(0, retval);
#endif
    return logs;
}

void rePoster::addAccount(Account newaccount)
{
    LOG(INFO) << "Add Account " << newaccount.user()
        << " Type " << newaccount.type();
    pnet_->addAccount(newaccount);
}   

void rePoster::getInitialPosts()
{
    LOG(INFO) << "Get Initial Posts";
    int i = 0;
    Post *post[16];
    int rowsReturned = this->pstore_->get_post( post, 0, 16 );
    LOG(DEBUG) << "Posts returned " << rowsReturned;
    for ( int i = 0; i < rowsReturned; i++ )
    {
        postuiops_.NewPost(post[i],0);
    }
}

void rePoster::getPosts(int postfrom, int postto)
{
    LOG(INFO) << "Get Posts";
    /* stupid. workaround windows compiler, lets hope this works */
    Post *post = NULL;
    post = (Post *)calloc ( postto-postfrom, sizeof(Post *) );
    int rowsReturned = this->pstore_->get_post( &post, postfrom, postto );
    int min = rowsReturned < (postto-postfrom) ? rowsReturned : (postto-postfrom);
    LOG(DEBUG) << "Posts returned " << rowsReturned;
    for ( int i = 0; i < min; i++ )
    {
        postuiops_.NewPost(&post[i],0);
    }
    free ( post );
}


void rePoster::rmAccount(Account account)
{
    LOG(INFO) << "Remove Account " << account.user();
    pnet_->rmAccount(account);
}

std::vector<Link> rePoster::getLinks()
{
    LOG(INFO) << "Get Links";
    return pnet_->getLinks();
}

void rePoster::addLink(Link newlink)
{
    LOG(INFO) << "Add Link " << newlink.name();
    pnet_->addLink(newlink);
}

void rePoster::rmLink(Link link)
{
    LOG(INFO) << "Remove Link " << link.name();
    pnet_->rmLink(link);
}

void rePoster::upboat(string u) 
{
    LOG(INFO) << "Upboating post " << u;
    Post *uppost = NULL;
    pstore_->update_metric(u, 1);
    pstore_->get_post(&uppost, u);
    if(uppost)
    {
      LOG(DEBUG) << "content " << uppost->content();
      pnet_->post(*uppost);
    }
}

void rePoster::downboat(std::string uuid) 
{
    LOG(INFO) << "Downboating post UUID " << uuid;
    pstore_->delete_post(uuid);
}

rePoster::~rePoster()
{
    if(pnet_)
    {
        pnet_->stop();
    }
    if(pcon_)
    {
        pcon_->stop();
    }
    delete pnet_;
    delete pstore_;
    delete pcon_;
    ShutdownRepostLogging();
}
