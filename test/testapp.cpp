#include <dlfcn.h>
#include "rpl.h"
#include "rpl_network.h"
#include "rpl_storage.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

int main( void )
{
#if 0
    rp_post *post;
    rpl_nw_eventcb cbs;
    rpl_storage *store = new rpl_storage();

    rpl_network net(cbs);
    net.add_jab("reposttestacct@gmail.com","rpthatshit!","");
    net.add_jab("test@j.sideramota.com","password","");
    //net.add_bon("fuckyeah");
    net.go();
    rp_link link1(string("andrew@sideramota.com"));
    net.addlink(link1);
    rp_link link(string("lukehaslett@gmail.com"));
    net.addlink(link);
    while(1){
        post = net.getpost();
        store->add_post(*post);
        net.post(*post);
    }
#endif
    void * handle = dlopen("./plugin/librpl.dylib",RTLD_NOW);
if (!handle) {
        fprintf (stderr, "%s\n", dlerror());
        exit(1);
    }
    void *rp = dlsym(handle,"rp_maker");
    rePoster *p =( rePoster *)rp;
    p->getPost();
    while(1){
        sleep(1000);
    }    

    return 0;
}
