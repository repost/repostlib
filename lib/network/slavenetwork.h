
#ifndef SLAVENETWORK_H_
#define SLAVENETWORK_H_

#include "rpl.h"
#include "pthread.h"

using namespace std;
/* so we don't need to know about xml shit */
namespace xercesc_3_1 //May have to update as version numbers increase
{
    class SAX2XMLReader;
}
typedef struct {
  void  (*newpost)();
  void  (*newlink)();
} sn_cb;

class slavenet
{
public:
    slavenet(): stopreq(false), running(false){init_xml();};
    virtual ~slavenet(){};
    virtual void go() = 0;
    void stop();
    virtual void sendpost(Post *post) = 0;
    bool stopreq;
    bool running;
    pthread_t m_thread;

protected:
    void uninit_xml();
    void xml2post(string *spost, Post *post);
    void post2xml(string *spost, Post *post);

private:
    xercesc_3_1::SAX2XMLReader * parser;
    void init_xml();
};

#endif
