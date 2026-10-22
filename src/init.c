#include "header.h"

volatile sig_atomic_t interrupted = 0;

static const struct option long_opts[] = {{"help", no_argument, NULL, 'h'},
                                          {NULL, 0, NULL, 0}};

static void sigint_handler(int signal) {
  if (signal == SIGINT) {
    interrupted = 1;
  }
}

static int check_args(int argc, char *argv[], struct tracert_ctx *ctx) {
  int opt;

  ctx->args.verbose = false;
  ctx->args.ttl = 30;
  ctx->args.timeout = 5;
  opterr = 0;

  while ((opt = getopt_long(argc, argv, "vh", long_opts, NULL)) != -1) {
    switch (opt) {
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

  if (optind >= argc) {
    printf("traceroute: missing host operand\n");
    return 2;
  }
  if (optind + 1 < argc) {
    print_help();
    return 2;
  }
  ctx->index = optind;
  return 0;
}

static int init_socket(struct tracert_ctx *ctx) {
  ctx->udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
  if (ctx->udp_socket < 0) {
    return errno;
  }
  int ret = setTtl(ctx);
  if(ret != 0){
    return errno;
  }

  ctx->imcp_socket = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
  if (ctx->imcp_socket < 0) {
    printf("Error creating socket: %s\n", strerror(errno));
    close(ctx->udp_socket);
    return errno;
  }
  return 0;
}

static int getDestAddr(char *argv, struct tracert_ctx *ctx) {
  struct addrinfo hints;
  struct addrinfo *result, *rp;
  char ipstr[INET_ADDRSTRLEN];

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  int ret = getaddrinfo(argv, NULL, &hints, &result);
  if (ret != 0) {
    fprintf(stderr, "%s: %s\n", argv, gai_strerror(ret));
    return 2;
  }
  for (rp = result; rp != NULL; rp = rp->ai_next) {
    if (rp->ai_family == AF_INET) {
      struct sockaddr_in *temp = (struct sockaddr_in *)rp->ai_addr;
      memcpy(&(ctx->destAddr), (struct sockaddr_in *)rp->ai_addr,
             sizeof(struct sockaddr_in));
      inet_ntop(rp->ai_family, &(temp->sin_addr), ipstr, sizeof(ipstr));
      memcpy(ctx->destHostname, ipstr, strlen(ipstr));
      freeaddrinfo(result);
      return 0;
    }
  }
  freeaddrinfo(result);
  return 2;
}

void init_select(struct tracert_ctx *ctx){
    FD_ZERO(&ctx->slct.readfs);
    FD_SET(ctx->imcp_socket, &ctx->slct.readfs);
    ctx->slct.tv.tv_sec = (time_t)ctx->args.timeout;
    ctx->slct.tv.tv_usec = (ctx->args.timeout - ctx->slct.tv.tv_sec) * 1000000;
}

int init_ctx(struct tracert_ctx *ctx, int argc, char **argv) {
    memset(ctx, 0, sizeof(*ctx));
    int ret = check_args(argc, argv, ctx);
    if (ret != 0)
        return (ret);
        
    // init tracert_ctx
    ctx->port = 33434;
    ctx->ttl = 1;
    ctx->probeNumber = 0;
    ctx->timePacketSend = 0;
        
    //fill addr
    ret = getDestAddr(argv[ctx->index], ctx);
    if (ret != 0) {
        return ret;
    }
    ctx->destAddr.sin_family = AF_INET;
    ctx->destAddr.sin_port = htons(ctx->port);

    // init select_ctx
    FD_ZERO(&ctx->slct.readfs);
    FD_SET(ctx->imcp_socket, &ctx->slct.readfs);
    ctx->slct.tv.tv_sec = (time_t)ctx->args.timeout;
    ctx->slct.tv.tv_usec = (ctx->args.timeout - ctx->slct.tv.tv_sec) * 1000000;

    //init signals
    bzero(&(ctx->sig), sizeof(ctx->sig));
    ctx->sig.sa_handler = &sigint_handler;
    sigaction(SIGINT, &(ctx->sig), NULL);
    
    //init sockets
    ret = init_socket(ctx);
    if(ret != 0)
        return ret;

    // init select_ctx
    init_select(ctx);

    return 0;
}