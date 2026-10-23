#include "header.h"

void print_help(){
    printf("Usage: traceroute [OPTION...] HOST\n"
		"Print the route packets trace to network host.\n"
        "  --help\t\t\t print help and exit\n"
		"  -w VALUE  --wait=VALUE         Wait for a probe no more than VALUE\n"
		"  -m max_ttl  --max-hops=max_ttl Set the max number of hops\n"
		"  -n                             resolve IP addresses to their domain names\n"
		"  -q nqueries --queries=nqueries Set the number of probes per each hop\n"
	);
}

void print_packet(struct tracert_ctx *ctx, struct iphdr *ip){
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
		if(ctx->args.do_dns){
			printf("  %d   %s (%s)  %0.3fms  ", ctx->ttl, hostname, stringIp, (get_timestamp() - ctx->timePacketSend) * 1000);
		}
		else{
			printf("  %d  %s  %0.3fms  ", ctx->ttl, stringIp, (get_timestamp() - ctx->timePacketSend) * 1000);
		}
    }
    else
        printf("%0.3fms  ", (get_timestamp() - ctx->timePacketSend) * 1000);
}

void print_timeout(struct tracert_ctx *ctx){
    if(ctx->probeNumber == 0){
        printf("  %d  *  ", ctx->ttl);
    }
    else
        printf("*  ");
}