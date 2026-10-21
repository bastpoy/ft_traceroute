#include "header.h"

int main(int argc, char *argv[]) {
	struct tracert_ctx ctx;
	int ret = init_ctx(&ctx, argc, argv);
	if (ret != 0) {
		if (ret == PRINT_HELP)
			return 0;
		return ret;
	}

	fd_set readfds;
	struct timeval tv;

	FD_ZERO(&readfds);
	FD_SET(ctx.imcp_socket, &readfds);

	tv.tv_sec  = (time_t)ctx.args.timeout;
    tv.tv_usec = (ctx.args.timeout - tv.tv_sec) * 1000000;

  	printf("traceroute to %s (%s) , 30 hops max, 60 byte packets\n", argv[ctx.index], ctx.destHostname);

	for (int i = 0; i < 3; i++){
		ctx.probeNumber = i;
		ctx.timePacketSend = get_timestamp();
		int size = sendto(ctx.udp_socket, DATA_SEND, 32, 0, (struct sockaddr *)&(ctx.destAddr), sizeof(ctx.destAddr)); 
		if(size < 1){
			fprintf(stderr, "Error sendto: %s\n", strerror(errno));
			return(1);
		}
		int ret = select(ctx.imcp_socket + 1, &readfds, NULL, NULL, &tv);
		if(ret < 0){
			fprintf(stderr, "Error select %s\n", strerror(errno));
			return errno;
		}
		else if(ret > 0){
			if(FD_ISSET(ctx.imcp_socket, &readfds)){
				readBuffer(&ctx);
			}
		}
		else if (ret == 0){
			printf("ca a timeout\n");
		}
	}
	return (0);
}