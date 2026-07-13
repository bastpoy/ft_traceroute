#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <netinet/ip_icmp.h>

double get_timestamp(){
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + ((double)tv.tv_usec) / 1000000;
}

uint16_t checksum(void *addr, size_t len) {
    uint16_t *word = addr;
    uint32_t result = 0;

    for (long unsigned int i = 0; i < len / sizeof(uint16_t); i++) {
        result += *(word + i);
    }
    if (len % 2 == 1) {
        result += *((uint8_t *)addr + len - 1);
    }
    result = (result >> 16) + (result & 0xffff);
    
    result = (result >> 16) + (result & 0xffff);
    return ~result;
}

void create_packet(struct icmphdr *packet, uint8_t *msg){
    packet->type = ICMP_ECHO; 
    packet->code = 0;
    packet->un.echo.id = getpid();
    packet->un.echo.sequence = 1;
    packet->checksum = 0;
    packet = (struct icmphdr*)msg;
    double *timestamp = (double *)(packet + 1);
    *timestamp = get_timestamp();
    packet->checksum = checksum(msg, 16);
}

int main(int argc, char *argv[])
{
    if(argc < 2){
        return 1;
    }

    fd_set rfds;
    // int retval;
    
    /* Surveiller stdin (fd 0) en attente d'entrées */
    FD_ZERO(&rfds);
    
    /* Pendant 5 secondes maxi */
    // struct timeval tv;
    // tv.tv_sec = 5;
    // tv.tv_usec = 0;

    int udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if(udpSocket < 0){
		printf("Error creating socket: %s\n", strerror(errno));
		return errno;
	}
    int value = 2;
    socklen_t sockSize = sizeof(value);
    if(setsockopt(udpSocket, IPPROTO_IP, IP_TTL, &value, sockSize) < 0){
        strerror(errno);
        return(errno);
    }
    
    FD_SET(udpSocket, &rfds);

	uint8_t *msg = malloc(sizeof(struct icmphdr) + 8);
    struct icmphdr *payload = (struct icmphdr*)msg;
	create_packet(payload, msg);

    struct sockaddr_in destAddr;
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = 33434;

	char buffer[1024] = "\0";
    
    // if(bind(udpSocket,(struct sockaddr *)&destAddr, sizeof(struct sockaddr_in)) < 0){
    //     printf("Error binding socket: %s\n", strerror(errno));
    //     return errno;
    // }
    int ret = inet_pton(AF_INET, argv[1], &(destAddr.sin_addr));
    if(ret < 1){
        strerror(errno);
        return errno;
    }

    // retval = select(1, &rfds, NULL, NULL, &tv);
    // socklen_t lenDest = sizeof(destAddr);
	ret = sendto(udpSocket, payload, 16, 0,(struct sockaddr*)(&destAddr), sizeof(struct sockaddr));
	if(ret < 0){
		printf("impossible to send: %s\n", strerror(errno));
	}
	else{
		printf("packet sent!!!\n");
		// ret = select(udpSocket + 1, &rfds, NULL, NULL, &tv);
		// if(ret < 0){
		//     printf("Error on select fd: %s\n", strerror(errno));
		//     return errno;
		// }
		// printf("ret is: %d\n", ret);
		int icmpSocket = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
		if(icmpSocket < 0){
			printf("Error creating socket: %s\n", strerror(errno));
			return errno;
		}
		if(recvfrom(icmpSocket, buffer, 1024, 0, NULL, NULL) < 0){
			strerror(errno);
			return errno;
		}
		else{
			printf("paquet recu!!!!\n");
			int i = 0;
			while(buffer[i] != '\0'){
				printf("%d-", buffer[i]);
				i++;
			}
		}
	}
    /* Considérer tv comme indéfini maintenant ! */
    // Dans un while tant que retval n'est pas nul
        // Créer le socket
        // sendto le paquet 
        // select le socket
        // recvfrom
    return(0);
}