#include "msg_types.h"

const char * entity_type_name(int type)
{
    switch (type)
    {
        case ENTITY_TYPE_CLIENT: return "client";
        case ENTITY_TYPE_SERVER: return "server";
        default: return "unknown";
    }
}

