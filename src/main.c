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

int initialize_socket(int *udpSocket, int *icmpSocket) {
  int ttl = 1;
  *udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
  if (*udpSocket < 0) {
	return errno;
  }
  if (setsockopt(*udpSocket, IPPROTO_IP, IP_TTL, &ttl, 4) < 0) {
	strerror(errno);
	return errno;
  }

  *icmpSocket = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
  if (*icmpSocket < 0) {
	printf("Error creating socket: %s\n", strerror(errno));
	return errno;
  }
  return 0;
}

// int get_and_print_packet(unsigned char *buffer, size_t buffer_size, char
// *buffer1, size_t buffer1_size, bool *stop, float time){
// int get_and_print_packet(unsigned char *buffer, char *buffer1, bool *stop,
// 						 float time) {
//   struct iphdr *recIp = (struct iphdr *)buffer;
//   struct icmphdr *recIcmp = (struct icmphdr *)(buffer + sizeof(struct iphdr));

//   // create address to have the src domain
//   struct sockaddr_in srcAddr;
//   srcAddr.sin_family = AF_INET;
//   srcAddr.sin_port = htons(11111);
//   srcAddr.sin_addr.s_addr = recIp->saddr;
//   char hbuf[1024] = "\0";
//   char sbuf[1024] = "\0";
//   int ret = getnameinfo((struct sockaddr *)&srcAddr, sizeof(struct sockaddr),
// 						hbuf, 1024, sbuf, 1024, NI_NAMEREQD);
//   if (ret != 0 && ret != EAI_NONAME) {
// 	printf("Error getting address Name: %s %d\n", gai_strerror(ret), ret);
// 	return errno;
//   }

//   char *strAddr = inet_ntoa(srcAddr.sin_addr);
//   if (hbuf[0] == '\0')
// 	memcpy(hbuf, strAddr, strlen(strAddr));

//   char buffer2[1024];
//   ft_strlcpy(buffer2, hbuf, strlen(hbuf) + 1);
//   char *strTemp = ft_strjoin(buffer2, " (");
//   char *tmp[2];
//   tmp[0] = strTemp;
//   strTemp = ft_strjoin(strTemp, strAddr);
//   tmp[1] = strTemp;
//   strTemp = ft_strjoin(strTemp, ")");
//   free(tmp[0]);
//   free(tmp[1]);
//   if (!strncmp(buffer1, buffer2, strlen(buffer2)))
// 	printf("%0.3f ms  ", time);
//   else
// 	printf("%s  %0.3f ms  ", strTemp, time);
//   free(strTemp);
//   // printf("buffer %s %s\n", buffer1, buffer2);
//   ft_strlcpy(buffer1, buffer2, strlen(buffer2) + 1);
//   if (recIcmp->code == 3 && recIcmp->type == 3) {
// 	*stop = true;
//   }
//   return 0;
// }

int main(int argc, char *argv[]) {
  struct tracert_ctx ctx;

  int ret = init_ctx(&ctx, argc, argv);
  if (ret != 0) {
	if (ret == PRINT_HELP) {
	  return 0;
	}
	return ret;
  }
  bool stop = false;

  fd_set rfds;

  char buffer1[1024] = "\0";

  FD_ZERO(&rfds);

  // Set Time
  struct timeval tsend;
  struct timeval trec;
  struct timeval tv;
  tv.tv_sec = 1;
  tv.tv_usec = 0;
  tsend.tv_sec = 0;
  tsend.tv_usec = 0;
  trec.tv_sec = 0;
  trec.tv_usec = 0;

  int ttl = 1;
  int addPort = 0;
  int prob = 0;

  unsigned char buffer[1024] = "\0";
  ret = getDestAddr(argv[ctx.index], &ctx);
  if (ret != 0) {
	fprintf(stderr, "ft_traceroute: unknown host\n");
	return ret;
  }

  printf("traceroute to %s (%s) , 30 hops max, 60 byte packets\n", argv[1],
		 ctx.destHostname);
  // loop ttl first
  // then loop prob
  for (int ttl = 1; ttl < 10; ttl++) {
  }
  while (ttl < 10) {
	ctx.destAddr.sin_port = htons(33434 + addPort);
	addPort++;
	ret = sendto(ctx.udp_socket, DATA_SEND, 32, 0,
				 (struct sockaddr *)(&(ctx.destAddr)), sizeof(struct sockaddr));
	if (ret < 0) {
	  fprintf(stderr, "ft_traceroute: sendto: %s\n", strerror(errno));
	  return errno;
	}
	if (gettimeofday(&tsend, NULL) < 0) {
	  printf("error getting time %s\n", strerror(errno));
	  return errno;
	}
	long long int timeSend = concatenate(tsend.tv_sec, tsend.tv_usec);

	if (prob % 3 == 0)
	  printf(" %d  ", ttl);

	FD_SET(ctx.imcp_socket, &rfds);

	ret = select(ctx.imcp_socket + 1, &rfds, NULL, NULL, &tv);
	if (ret) {
	  if (FD_ISSET(ctx.imcp_socket, &rfds)) {
		if (gettimeofday(&trec, NULL) < 0) {
		  printf("error getting time %s\n", strerror(errno));
		  return errno;
		}
		long long int timeRec = concatenate(trec.tv_sec, trec.tv_usec);
		// float time = (float)(timeRec - timeSend) / 1000;
		ssize_t n = read(ctx.imcp_socket, buffer, 1024);
		if (n < 0) {
		  strerror(errno);
		  return errno;
		} else {
		//   ret = get_and_print_packet(buffer, buffer1, &stop, time);
		//   if (ret)
			// return ret;
		}
	  }
	} else {
	  printf("*  ");
	}
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	prob++;
	if (prob >= 3) {
	  printf("\n");
	  memset(buffer1, 0, 1024);
	  if (stop)
		return 0;
	  prob = 0;
	  ttl++;
	  if (setsockopt(ctx.udp_socket, IPPROTO_IP, IP_TTL, &ttl, 4) < 0) {
		strerror(errno);
		return -1;
	  }
	}
  }
  return (0);
}