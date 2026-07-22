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
#include <netinet/ip.h>
#include <ifaddrs.h>
#include "./include/libft/libft.h"

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

struct sockaddr_in *get_my_ip(char *buffer){
    struct ifaddrs *host_addrs = NULL;
    struct sockaddr_in *addr= NULL;

    
    int ret = getifaddrs(&host_addrs);
    void *ptrmec = host_addrs;
    if(ret == -1){
        printf("Error getting host addresses");
        return(NULL);
    }
    while(host_addrs){

        sa_family_t family = host_addrs->ifa_addr->sa_family;
        addr = (struct sockaddr_in*)host_addrs->ifa_addr;

        if(family == AF_INET){
            if(strcmp(host_addrs->ifa_name, "wlp2s0") == 0){
                if(!inet_ntop(AF_INET, &addr->sin_addr, buffer, INET_ADDRSTRLEN)){
                    printf("Error converting address too string\n");
                    freeifaddrs(ptrmec);
                    return NULL;
                }
                freeifaddrs(ptrmec);
                return addr;
            }
        }
        host_addrs = host_addrs->ifa_next;
    }
    freeifaddrs(ptrmec);
    return NULL;
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

unsigned int reverse_number(unsigned int number){
    unsigned int reversed_number = 0;
    while (number != 0) {
        int remainder = number % 10;
        reversed_number = reversed_number * 10 + remainder;
        number /= 10;
    }
    return reversed_number;
}

long long int join_time(struct timeval time){
    long long int res = time.tv_sec;
    time.tv_usec = reverse_number(time.tv_usec);
    while(time.tv_usec != 0){
        res = (res * 10) + time.tv_usec % 10;
        time.tv_usec /= 10;
    }
    return res;
}

int setSocket(int value){
    int udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if(udpSocket < 0){
		printf("Error creating socket: %s\n", strerror(errno));
		return -1;
	}
    socklen_t sockSize = sizeof(value);
    if(setsockopt(udpSocket, IPPROTO_IP, IP_TTL, &value, sockSize) < 0){
        strerror(errno);
        return -1;
    }
    return udpSocket;
}

void print_line(struct iphdr *recIp, struct icmphdr *recIcmp){
    (void)recIp;
    (void)recIcmp;
}

int main(int argc, char *argv[])
{
    if(argc < 2){
        return 1;
    }

    fd_set rfds;
    
    /* Surveiller stdin (fd 0) en attente d'entrées */
    FD_ZERO(&rfds);
    
    //Set Time
    struct timeval tsend;
    // struct timeval trec;
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    tsend.tv_sec = 0;
    tsend.tv_usec = 0;
    // trec.tv_sec = 0;
    // trec.tv_usec = 0;

    int value = 1;
    int addPort = 0;
    int threeProb = 1;

    struct sockaddr_in destAddr;
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(33434);

	unsigned char buffer[1024] = "\0";
    char bufferSend[32] = "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_";
    int ret = inet_pton(AF_INET, argv[1], &(destAddr.sin_addr));
    if(ret < 1){
        strerror(errno);
        return errno;
    }
    int udpSocket = setSocket(value);
    if(udpSocket < 0){
        return errno;
    }
    int icmpSocket = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if(icmpSocket < 0){
        printf("Error creating socket: %s\n", strerror(errno));
        return errno;
    }

    while(value < 8){
        destAddr.sin_port = htons(33434 + addPort);
        addPort++;
        ret = sendto(udpSocket, bufferSend, 32, 0,(struct sockaddr*)(&destAddr), sizeof(struct sockaddr));
        if(ret < 0){
            printf("impossible to send: %s\n", strerror(errno));
            return errno;
        }
        else{
            printf("packet sent!!!\n");
            if(gettimeofday(&tsend, NULL) < 0){
                printf("error getting time %s\n", strerror(errno));
                return errno;
            }
            // calcul time
            long long int timeSend = join_time(tsend);
            FD_SET(icmpSocket,&rfds);

            // ajout dans le fd
            ret = select(icmpSocket + 1, &rfds, NULL, NULL, &tv);
            if(ret){
                if(FD_ISSET(icmpSocket, &rfds)){
                    printf("j'ai de la lecture dans le socket: %d %d\n", ret, icmpSocket);
                    ssize_t n = read(icmpSocket, buffer, 1024);
                    if(n < 0){
                        strerror(errno);
                        return errno;
                    }
                    else{
                        struct iphdr *recIp = (struct iphdr *)buffer;
                        struct icmphdr *recIcmp = (struct icmphdr *)(buffer + sizeof(struct iphdr));
                        printf("rec ip: %u\n", recIp->daddr);
                        printf("rec icmp: code %d, type %d\n\n", recIcmp->code, recIcmp->type);
                        if(recIcmp->code == 3 && recIcmp->type == 3){
                            return 0;
                        }
                    }
                }
            }
            else{
                printf("error timeout ret is %d\n", ret);
            }
            tv.tv_sec = 5;
            tv.tv_usec = 0;
        }
        threeProb++;
        if(threeProb >= 4){
            value++;
            if(setsockopt(udpSocket, IPPROTO_IP, IP_TTL, &value, 4) < 0){
                strerror(errno);
                return -1;
            }
            threeProb = 1;
        }
    }
    return(0);
}