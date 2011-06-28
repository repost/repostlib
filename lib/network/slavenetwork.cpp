
#include "rpl.h"
#include "slavenetwork.h"
#include <string.h>
extern "C" {
#include "libxml/parser.h"
}
/**
 * @brief initialises the networks 
 */

void slavenet::init_xml()
{
}

void slavenet::xml2post(string *spost, Post *post)
{
    xmlDocPtr doc = NULL;
    xmlNodePtr n = NULL, r = NULL;

    doc = xmlReadMemory(spost->c_str(), spost->length(), "post.xml", NULL, 0);
    r = xmlDocGetRootElement(doc);/*post elemt */
    if(r == NULL)
    {
        free(doc);
        return;
    }

    n = r->children; /* first item */
    while(n != NULL)
    {
        char* name = (char *)n->name;
        xmlNodePtr child = n->children;
        if(!strncmp(name,"uuid", sizeof("uuid")) && child )
        {
            post->set_uuid(string((char *)child->content));
        }
        else if(!strncmp(name,"content", sizeof("content")) && child)
        {
            post->set_content(string((char *)child->content));
        }
        else if(!strncmp(name,"certs", sizeof("certs")) && child)
        {
            post->set_certs(string((char *)child->content));
        }
        n = n->next;
    }
    xmlFreeDoc(doc);
}

void slavenet::post2xml(string *spost, Post *post)
{
    xmlNodePtr n = NULL, r = NULL, c = NULL;
    xmlDocPtr doc = NULL;
    xmlChar* xmlbuff = NULL;
    int buffersize = 0, x = 0;

    /*
     * Create the document.
     */
    doc = xmlNewDoc(BAD_CAST "1.0");
    r = xmlNewNode(NULL, BAD_CAST "post");
    n = xmlNewNode(NULL, BAD_CAST "uuid");
    /* figure if we can allocate 3 we should be able to allocate the rest */
    if(!doc || !r || !n)
    {
        xmlFree(xmlbuff);
        xmlFreeDoc(doc);
        return;
    }
    xmlNodeSetContent(n, BAD_CAST post->uuid().c_str());
    xmlAddChild(r,n);

    n = xmlNewNode(NULL, BAD_CAST "content");
    /* AddContent used here so we can abuse special chars */
    xmlNodeAddContent(n, BAD_CAST post->content().c_str());
    xmlAddChild(r,n);

    c = xmlNewNode(NULL, BAD_CAST "certs");
    xmlAddChild(r,c);
/*
    for(x=0; x < post->get_numcerts(); x++)
    {
        /* Need to do allocation checks here as cert lists could get massive 
        n = xmlNewNode(NULL, BAD_CAST "cert");
        xmlNodeSetContent(n, BAD_CAST post->certs(x).c_str());
        xmlAddChild(c,n);
    }
*/
    xmlDocSetRootElement(doc, r);
    xmlDocDumpFormatMemoryEnc(doc, &xmlbuff, &buffersize,"ASCII", 1);

    spost->assign((char *)xmlbuff);

    xmlFree(xmlbuff);
    xmlFreeDoc(doc);
}

void slavenet::uninit_xml(){

}

void slavenet::Stop(){
    if(running == true)
    {
         running = false;
         stopreq = true; 
         pthread_join(m_thread,0);
    }
}

