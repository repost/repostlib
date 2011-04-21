#include "rpl.h"
#include "rpl_storage.h"
#include <iostream>

extern "C"
{
#ifdef WIN32
#include <Rpc.h>
#else
#include <uuid/uuid.h>
#endif

std::string Post::gen_uuid(){
    #ifdef WIN32
        UUID uuid;
        UuidCreate ( &uuid );

        unsigned char * str;
        UuidToStringA ( &uuid, &str );

        std::string s( ( char* ) str );

        RpcStringFreeA ( &str );
    #else
        uuid_t uuid;
        uuid_generate_random ( uuid );
        char s[37];
        uuid_unparse ( uuid, s );
    #endif
        return s;
    }
}

void Post::upboat(std::string uuid) {
    std::cout << "upboated! update metric!" << std::endl;
    rpl_storage *store = rpl_storage::get_instance();
    store->update_metric(uuid);
}

void Post::downboat(std::string uuid) {
    std::cout << "downboated! delete that shit" << std::endl;
    rpl_storage *store = rpl_storage::get_instance();
    store->delete_post(uuid);
}
