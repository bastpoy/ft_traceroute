#include "header.h"

void print_help(){
    printf("Usage\n  ping [options] <destination>\n\nOptions:\n"  
        "<destination>\t\tdns name or ip address\n"
        "  -h\t\t\tprint help and exit\n"
        "  --ttl=N\t\tspecify N as time-to-live\n");
}