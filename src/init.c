#include "header.h"

volatile sig_atomic_t interrupted = 0;

static const struct option long_opts[] = {{"help", no_argument, NULL, 'h'},
											{"max-hops", required_argument, NULL, 'm'},
											{"wait", required_argument, NULL, 'w'},
											{"queries", required_argument, NULL, 'q'},
										  {NULL, 0, NULL, 0}};

static void sigint_handler(int signal) {
	if (signal == SIGINT) {
		interrupted = 1;
	}
}

static int parse_int(const char *str, int min, int max, int *out)
{
    char *end;
    long val;

    errno = 0;
    val = strtol(str, &end, 10);
    if (errno != 0 || end == str || *end != '\0')
        return (-1);
    if (val < min || val > max)
        return (-1);
    *out = (int)val;
    return (0);
}

static int parse_double(const char *str, double min, double max, double *out) {
	char *end;
	double val;

	if (str == NULL || *str == '\0')
		return 1;
	errno = 0;
	val = strtod(str, &end);
	if (errno != 0 || end == str)
		return 1;
	if (*end != '\0' && *end != ',')
		return 1;
	if (!isfinite(val) || val < min || val > max)
		return 1;
	*out = val;
	return 0;
}

static int check_args(int argc, char *argv[], struct tracert_ctx *ctx) {
	int opt;

	ctx->args.ttl = 30;
	ctx->args.timeout = 3.0;
	ctx->args.do_dns = false;
	ctx->args.probeNbr = 3;
	opterr = 0;

	while ((opt = getopt_long(argc, argv, ":m:hw:nq:", long_opts, NULL)) != -1) {
		switch (opt) {
		case 'm':
			if (parse_int(optarg, 1, 255, &ctx->args.ttl) != 0){
				fprintf(stderr, "traceroute: invalid value (`%s' near `%s')\n", optarg, optarg);
				return 1;
			}
			break;
		case 'h':
			print_help();
			return 3;
		case 'w':
			if (parse_double(optarg, 0.0, 86400.0, &ctx->args.timeout) != 0) {
				fprintf(stderr, "traceroute: invalid value (`%s' near `%s')\n", optarg, optarg);
				return 1;
			}
			break;
		case 'n':
			ctx->args.do_dns = true;
			break;
		case 'q':
			if (parse_int(optarg, 1, MAX_PROBES, &ctx->args.probeNbr) != 0) {
				fprintf(stderr, "traceroute: invalid value (`%s' near `%s')\n", optarg, optarg);
				return 1;
			}
			break;
		case ':':
			if (optopt)
				fprintf(stderr, "traceroute: option requires an argument -- '%c'\n", optopt);
			else
				fprintf(stderr, "traceroute: option '%s' requires an argument\n", argv[optind - 1]);
			print_help();
			return 3;
		case '?':
			if (optopt)
				fprintf(stderr, "traceroute: invalid option -- '%c'\n", optopt);
			else
				fprintf(stderr, "traceroute: unrecognized option '%s'\n", argv[optind - 1]);
			print_help();
			return 3;
		}
	}

	if (optind >= argc) {
		if(argc == 1){
			print_help();
			return 3;
		}
		fprintf(stderr, "traceroute: missing host operand\n");
		return 2;
	}
	if (optind + 1 < argc) {
		print_help();
		return 3;
	}
	ctx->index = optind;
	return 0;
}

static int setTtl(struct tracert_ctx *ctx) {
	if (setsockopt(ctx->udp_socket, IPPROTO_IP, IP_TTL, &ctx->ttl,
				   sizeof(ctx->ttl)) < 0) {
		fprintf(stderr, "Error setsocket: %s", strerror(errno));
		return errno;
	}
	return 0;
}

static int init_socket(struct tracert_ctx *ctx) {
	ctx->udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (ctx->udp_socket < 0) {
		return errno;
	}
	int ret = setTtl(ctx);
	if (ret != 0) {
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
			memcpy(ctx->ipDest, ipstr, strlen(ipstr));
			freeaddrinfo(result);
			return 0;
		}
	}
	freeaddrinfo(result);
	return 2;
}

void set_select(struct tracert_ctx *ctx, double timeout) {
	FD_ZERO(&ctx->slct.readfs);
	FD_SET(ctx->imcp_socket, &ctx->slct.readfs);
	ctx->slct.tv.tv_sec = (time_t)timeout;
	ctx->slct.tv.tv_usec = (timeout - ctx->slct.tv.tv_sec) * 1000000;
}

int set_var(struct tracert_ctx *ctx, int probes) {
	ctx->probeNumber = probes;
	ctx->timePacketSend = get_timestamp();
	ctx->mypacket = false;

	if (setTtl(ctx)) {
		close_program(ctx);
		return errno;
	}
	ctx->destAddr.sin_port = htons(ctx->port);
	return 0;
}

int init_ctx(struct tracert_ctx *ctx, int argc, char **argv) {
	memset(ctx, 0, sizeof(*ctx));
	int ret = check_args(argc, argv, ctx);
	if (ret != 0)
		return ret;

	// init tracert_ctx
	ctx->port = 33434;
	ctx->ttl = 1;
	ctx->probeNumber = 0;
	ctx->timePacketSend = 0;
	ctx->end = false;
	ctx->mypacket = false;

	// fill addr
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

	// init signals
	bzero(&(ctx->sig), sizeof(ctx->sig));
	ctx->sig.sa_handler = &sigint_handler;
	sigaction(SIGINT, &(ctx->sig), NULL);

	// init sockets
	ret = init_socket(ctx);
	if (ret != 0)
		return ret;

	return 0;
}