#include "header.h"

void print_help(){
    printf("Usage\n  ping [options] <destination>\n\nOptions:\n"  
        "<destination>\t\tdns name or ip address\n"
        "  -h\t\t\tprint help and exit\n"
        "  --ttl=N\t\tspecify N as time-to-live\n");
}

void print_packet(struct tracert_ctx *ctx, struct iphdr *ip, bool timeout){
    if(ctx->probeNumber == 0){
        char stringIp[INET_ADDRSTRLEN];
        char hostname[NI_MAXHOST];
        struct sockaddr_in addr;
        
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = ip->saddr;

        inet_ntop(AF_INET, &ip->saddr, stringIp, sizeof(stringIp));
        int ret = getnameinfo((struct sockaddr *)&addr, sizeof(struct sockaddr_in), hostname, 
            sizeof(hostname), NULL, 0, NI_NAMEREQD);
        if(ret != 0){
            memcpy(hostname, stringIp, sizeof(stringIp));
        }
        if(timeout)
            printf("%s (%s) * ", hostname, stringIp);
        else
            printf("%s (%s) %0.3f ms ", hostname, stringIp, (get_timestamp() - ctx->timePacketSend) * 1000);
    }
    else if(ctx->probeNumber == 1){
        printf("%0.3f ms", (get_timestamp() - ctx->timePacketSend) * 1000);
    }
    else{
        printf(" %0.3f ms \n", (get_timestamp() - ctx->timePacketSend) * 1000);
    }
}