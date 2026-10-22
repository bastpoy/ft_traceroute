#include "header.h"

int main(int argc, char *argv[]) {
	struct tracert_ctx ctx;
	int ret = init_ctx(&ctx, argc, argv);
	if (ret != 0) {
		if (ret == PRINT_HELP)
			return 0;
		return ret;
	}

  	printf("traceroute to %s (%s) , 30 hops max, 60 byte packets\n", argv[ctx.index], ctx.destHostname);
	while(!interrupted && ctx.ttl <= ctx.args.ttl && !ctx.end){
		for (int i = 0; i < 3; i++){
			ctx.probeNumber = i;
			ctx.timePacketSend = get_timestamp();
			int size = sendto(ctx.udp_socket, DATA_SEND, 32, 0, (struct sockaddr *)&(ctx.destAddr), sizeof(ctx.destAddr)); 
			if(size < 1){
				fprintf(stderr, "Error sendto: %s\n", strerror(errno));
				return(1);
			}
			int ret = select(ctx.imcp_socket + 1, &ctx.slct.readfs, NULL, NULL, &ctx.slct.tv);
			if(ret < 0){
				if (errno == EINTR){
					break;
				}
				fprintf(stderr, "Error select %s\n", strerror(errno));
				return errno;
			}
			else if(ret > 0){
				if(FD_ISSET(ctx.imcp_socket, &ctx.slct.readfs)){
					ret = manage_response(&ctx);
					if(ret)
						return ret;
				}
			}
			else if (ret == 0){
				print_timeout(&ctx);				
				init_select(&ctx);
			}
			// ctx.port++;
		}
		printf("\n");
		ctx.ttl++;
		setTtl(&ctx);
	}
	return (0);
}

// free comme il faut a un signal