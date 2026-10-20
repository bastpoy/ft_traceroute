#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <getopt.h>

#define PRINT_HELP 3
#define DATA_SEND "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_"

struct arguments {
    bool verbose;
    int ttl;
};

struct tracert_ctx{
    int udp_socket;
    int imcp_socket;
    int index;
    int incrPort; //increment port on each ttl
    int ttl; // ttl that will be increment during the sending
    struct sockaddr_in destAddr; //dest addr that is in arg
    char destHostname[NI_MAXHOST];
    struct arguments args;
};

struct probe_ctx{

};

void print_help();
int init_ctx(struct tracert_ctx *ctx, int argc, char **argv);
int getDestAddr(char *argv, struct tracert_ctx *ctx);

//utils
size_t	ft_strlcpy(char *dest, char *src, size_t size);