#include "header.h"

double get_timestamp() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec + ((double)tv.tv_usec) / 1000000;
}

int is_timeout(struct tracert_ctx *ctx) {
	double actualTime = get_timestamp();
	if (actualTime >= ctx->timePacketSend + ctx->args.timeout){
		print_timeout(ctx);
		return 1;
	}
	set_select(ctx, ctx->timePacketSend + ctx->args.timeout - actualTime);
	return 0;
}

void close_program(struct tracert_ctx *ctx) {
	close(ctx->imcp_socket);
	close(ctx->udp_socket);
}

static int excedeed_unreach(struct icmphdr *icmp, struct tracert_ctx *ctx) {
  	struct iphdr *inaddr = (struct iphdr *)(icmp + 1);
	int in_hlen = inaddr->ihl * 4;
	struct udphdr *udpheader = (struct udphdr *)((char *)inaddr + in_hlen);

	if (ntohs(udpheader->uh_dport) != ctx->port)
		return 1;
	if (inaddr->protocol != IPPROTO_UDP) {
		return 1;
	}
	return 0;
}

int manage_response(struct tracert_ctx *ctx) {
	char buffer[1024] = "\0";
	int size = recvfrom(ctx->imcp_socket, buffer, sizeof(buffer), 0, NULL, NULL);
	if (size < 0) {
		close_program(ctx);
		// signal
		if (errno == EINTR) {
		return errno;
		}
		printf("Error receviving message: %s\n", strerror(errno));
		return -1;
	}

	struct iphdr *ip = (struct iphdr *)buffer;
	int ip_header_len = ip->ihl * 4;
	struct icmphdr *icmp = (struct icmphdr *)(buffer + ip_header_len);

	int ret = excedeed_unreach(icmp, ctx);
	if (ret) {
		return 1; // not for me
	}
	if (icmp->type == ICMP_TIME_EXCEEDED)
		print_packet(ctx, ip);
	else if (icmp->type == ICMP_DEST_UNREACH) { // done
		print_packet(ctx, ip);
		ctx->end = true;
	}
	ctx->mypacket = true;
	return 0;
}