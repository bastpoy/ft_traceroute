#include <arpa/inet.h>
#include <errno.h>
#include <float.h>
#include <getopt.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/udp.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define PRINT_HELP 3
#define TIME_OUT -1
#define DATA_SEND "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_"

extern volatile sig_atomic_t interrupted;

struct arguments {
  bool verbose;
  int ttl;
  double timeout;
};

struct select_struct {
  fd_set readfs;
  struct timeval tv;
};

struct tracert_ctx {
  int udp_socket;
  int imcp_socket;
  int index;

  // int incrPort; //increment port on each ttl
  int port;
  int ttl; // ttl that will be increment during the sending
  int probeNumber;
  double timePacketSend;
  bool end;
  bool mypacket;
  struct sockaddr_in destAddr; // dest addr that is in arg
  char ipDest[INET_ADDRSTRLEN];
  char hostname[NI_MAXHOST];
  struct sigaction sig;
  struct arguments args;
  struct select_struct slct;
};

// print
void print_help();
void print_packet(struct tracert_ctx *ctx, struct iphdr *ip);
void print_timeout(struct tracert_ctx *ctx);

// init
int init_ctx(struct tracert_ctx *ctx, int argc, char **argv);
void set_select(struct tracert_ctx *ctx, double timeout);
int set_packet(struct tracert_ctx *ctx, int probes);
int set_var(struct tracert_ctx *ctx, int probes);

// utils
double get_timestamp();
int is_timeout(struct tracert_ctx *ctx);
int manage_response(struct tracert_ctx *ctx);
void close_program(struct tracert_ctx *ctx);