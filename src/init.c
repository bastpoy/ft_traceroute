#include "header.h"

static const struct option long_opts[] = {
    {"help", no_argument, NULL, 'h'},
    {NULL,  0,                 NULL, 0}
};

static int check_args(int argc, char *argv[], struct tracert_ctx *ctx){
    int opt;

    ctx->args.verbose = false;
    ctx->args.ttl = 0;
    opterr = 0;

    while ((opt = getopt_long(argc, argv, "vh", long_opts, NULL)) != -1){
        switch (opt){
        case 'v':
            ctx->args.verbose = true;
            break;
        case 'h':
            print_help();
            return 3;
        case '?':
            print_help();
            return 3;
        }
    }

    if (optind >= argc){
        printf("traceroute: missing host operand\n");
        return 2;
    }
    if (optind + 1 < argc){
        print_help();
        return 2;
    }
    ctx->index = optind;
    return 0;
}

static int init_socket(struct tracert_ctx *ctx)
{
    int ttl = 1;
    ctx->udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if(ctx->udp_socket < 0){
        return errno;
    }
    if(setsockopt(ctx->udp_socket, IPPROTO_IP, IP_TTL, &ttl, 4) < 0){
        strerror(errno);
        return errno;
    }

    ctx->imcp_socket = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if(ctx->imcp_socket < 0){
        printf("Error creating socket: %s\n", strerror(errno));
        close(ctx->udp_socket);
        return errno;
    }
    return 0;
}

int init_ctx(struct tracert_ctx *ctx, int argc, char **argv)
{
    memset(ctx, 0, sizeof(*ctx));
    ctx->destAddr.sin_family = AF_INET;
    ctx->destAddr.sin_port = htons(33434);

    int ret = check_args(argc, argv, ctx);
    if (ret != 0)
        return (ret);

    return (init_socket(ctx));
}

int getDestAddr(char *argv, struct tracert_ctx *ctx){
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    char ipstr[INET_ADDRSTRLEN];

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    
    int ret = getaddrinfo(argv, NULL, &hints, &result);
    if(ret != 0){
        return 2;
    }
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        if (rp->ai_family == AF_INET) {
            struct sockaddr_in *temp = (struct sockaddr_in *)rp->ai_addr;
            memcpy(&(ctx->destAddr), (struct sockaddr_in *)rp->ai_addr, sizeof(struct sockaddr_in));
            inet_ntop(rp->ai_family, &(temp->sin_addr), ipstr, sizeof(ipstr));
			memcpy(ctx->destHostname, ipstr, strlen(ipstr));
            freeaddrinfo(result);
            return 0;
        }
    }
    freeaddrinfo(result);
    return 2;
}