
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
    xmlDocPtr doc;
    doc = xmlReadMemory(spost->c_str(), spost->length(), "post.xml", NULL, 0);
    xmlNodePtr n, r = xmlDocGetRootElement(doc);/*post elemt */
    if(r == NULL){
        free(doc);
        return;
    }
    n = r->children; /* first item */
    while(n != NULL){
        char * name = (char *)n->name;
        xmlNodePtr child = n->children;
        if(strcmp(name,"uuid")==0 && child != NULL){
            post->set_uuid(string((char *)child->content));
        }else if(strcmp(name,"content")==0&& child != NULL){
            post->set_content(string((char *)child->content));
        }else if(strcmp(name,"certs")==0&child != NULL){
            post->set_certs(string((char *)child->content));
        }
        n = n->next;
        continue;
     }
     free(doc);
}

void slavenet::post2xml(string *spost, Post *post)
{
    xmlNodePtr n,r,c;
    xmlDocPtr doc;
    xmlChar *xmlbuff;
    int buffersize=500, x;

    /*
     * Create the document.
     */
    doc = xmlNewDoc(BAD_CAST "1.0");
    r = xmlNewNode(NULL, BAD_CAST "post");
    n = xmlNewNode(NULL, BAD_CAST "uuid");
    xmlNodeSetContent(n, BAD_CAST post->uuid().c_str());
    xmlAddChild(r,n);

    n = xmlNewNode(NULL, BAD_CAST "content");
    xmlNodeSetContent(n, BAD_CAST post->content().c_str());
    xmlAddChild(r,n);

    c = xmlNewNode(NULL, BAD_CAST "certs");
    xmlAddChild(r,c);
/*
    for(x=0; x < post->get_numcerts(); x++)
    {
        n = xmlNewNode(NULL, BAD_CAST "cert");
        xmlNodeSetContent(n, BAD_CAST post->certs(x).c_str());
        xmlAddChild(c,n);
    }
*/
    xmlDocSetRootElement(doc, r);

    /*
     * Dump the document to a buffer and print it
     * for demonstration purposes.
     */
    xmlDocDumpFormatMemoryEnc(doc, &xmlbuff, &buffersize,"ASCII", 1);

    spost->assign((char *)xmlbuff);

    /*
     * Free associated memory.
     */
    xmlFree(xmlbuff);
    xmlFreeDoc(doc);

}

void slavenet::uninit_xml(){

}

void slavenet::stop(){
    if(running==true){
         running=false;
         stopreq=true; 
         pthread_join(m_thread,0);
    }
}

