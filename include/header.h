#pragma once
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
#include <netinet/udp.h>
#include <netinet/ip.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <getopt.h>
#include <signal.h>

#define PRINT_HELP 3
#define TIME_OUT -1
#define DATA_SEND "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_"

extern volatile sig_atomic_t interrupted;


struct arguments {
    bool verbose;
    int ttl;
    int timeout;
};

struct tracert_ctx{
    int udp_socket;
    int imcp_socket;
    int index;
    // int incrPort; //increment port on each ttl
    int port;
    int ttl; // ttl that will be increment during the sending
    int probeNumber;
    double timePacketSend;
    struct sockaddr_in destAddr; //dest addr that is in arg
    char destHostname[NI_MAXHOST];
    struct sigaction sig;
    struct arguments args;
};

//print
void print_help();
void print_packet(struct tracert_ctx *ctx, struct iphdr *ip, bool timeout);

//init
int init_ctx(struct tracert_ctx *ctx, int argc, char **argv);

//utils
long long int concatenate(long long int x, unsigned y);
double get_timestamp();
int readBuffer(struct tracert_ctx *ctx);