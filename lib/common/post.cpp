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

