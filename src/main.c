#include "header.h"

int main(int argc, char *argv[]) {
	struct tracert_ctx ctx;

	int ret = init_ctx(&ctx, argc, argv);
	if (ret != 0) {
		if (ret == PRINT_HELP)
			return 0;
		return ret;
	}

	printf("traceroute to %s (%s), 30 hops max, 60 byte packets\n",
		   ctx.hostname, ctx.ipDest);
	while (!interrupted && ctx.ttl <= ctx.args.ttl && !ctx.end) {
		int probes = 0;
		while (probes < 3) {
			if (set_var(&ctx, probes))
				return errno;
			int size = sendto(ctx.udp_socket, DATA_SEND, 32, 0,
							  (struct sockaddr *)&(ctx.destAddr),
							  sizeof(ctx.destAddr));
			if (size < 1) {
				fprintf(stderr, "Error sendto: %s\n", strerror(errno));
				return (1);
			}
			while (!ctx.mypacket) {
				if (is_timeout(&ctx) != 0) { // timeout
					print_timeout(&ctx);
					break;
				}
				int ret = select(ctx.imcp_socket + 1, &ctx.slct.readfs, NULL,
								 NULL, &ctx.slct.tv);
				// error on select
				if (ret < 0) {
					if (errno == EINTR) { //signal
						close_program(&ctx);
						return 0;
					}
					fprintf(stderr, "Error select %s\n", strerror(errno));
					return errno;
				}
				// timeout
				if (ret == 0) {
					print_timeout(&ctx);
					break;
				}
				// read actual packet
				ret = manage_response(&ctx);
				if (ret == -1)
					return errno;
			}
			ctx.port++;
			probes++;
		}
		printf("\n");
		ctx.ttl++;
	}
	close_program(&ctx);
	return 0;
}

// free comme il faut a un signal