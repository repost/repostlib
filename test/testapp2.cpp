
#include "rpl.h"
#include "rpl_network.h"
#include <iostream>
#include <sstream>

using namespace std;

int main( void )
{
    Post *post;
    rpl_network net;
    net.add_jab("test2@j.sideramota.com","password","");
    //net.add_bon("fuckyeah");
    net.go();
    while(1){
        string input;
        post = new Post();
        getline(cin,input);
        post->set_content(input);
        post->set_uuid("1");
        net.post(*post);
    }
    return 0;
}
