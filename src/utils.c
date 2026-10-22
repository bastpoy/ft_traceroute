#include "header.h"

long long int concatenate(long long int x, unsigned y) {
	// char *str = itoa(y);
	char buf[12];
	snprintf(buf, sizeof(buf), "%d", y);
	int len = strlen(buf);
	// free(str);
	if (len < 6) {
		while (len < 6) {
		x *= 10;
		len++;
		}
	}
	long long int pow = 10;
	while (y >= pow)
		pow *= 10;

	return x * pow + y;
}

double get_timestamp(){
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + ((double)tv.tv_usec) / 1000000;
}

int setTtl(struct tracert_ctx *ctx){
	if(setsockopt(ctx->udp_socket, IPPROTO_IP, IP_TTL, &ctx->ttl, sizeof(ctx->ttl)) < 0){
		fprintf(stderr, "Error setsocket: %s", strerror(errno));
		return errno;
	}
	return 0;
}

static int excedeed_unreach(struct icmphdr *icmp, struct tracert_ctx *ctx){
	struct iphdr *inaddr = (struct iphdr *)(icmp + 1);
	int             in_hlen = inaddr->ihl * 4;
	struct udphdr *udpheader = (struct udphdr*)((char *)inaddr + in_hlen);

	if(ntohs(udpheader->uh_dport) != ctx->port) 
		return 1;
	if(inaddr->protocol != IPPROTO_UDP){
		return 1;
	}
	return 0;
}

int manage_response(struct tracert_ctx *ctx){
	char buffer[1024] = "\0";
	int size = recvfrom(ctx->imcp_socket, buffer, sizeof(buffer), 0, NULL, NULL);
	if(size < 0){
		if (errno == EINTR) {
			close(ctx->imcp_socket);
			close(ctx->udp_socket);
			return errno;
		}
		printf("Error receviving message: %s\n", strerror(errno));
		return 1;
	}

	struct iphdr *ip = (struct iphdr *)buffer;
	int ip_header_len = ip->ihl * 4;
	struct icmphdr *icmp = (struct icmphdr *)(buffer + ip_header_len);

	int ret = excedeed_unreach(icmp, ctx);
	if(ret){
		return 1; // not for me
	}
	if(icmp->type == ICMP_TIME_EXCEEDED){
		print_packet(ctx, ip);
		init_select(ctx);
	}
	else if(icmp->type == ICMP_DEST_UNREACH){
		// done
		print_packet(ctx, ip);
		ctx->end = true;
	}
	return 0;
}