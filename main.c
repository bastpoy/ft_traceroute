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
#include <netdb.h>

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

long long int concatenate(long long int x, unsigned y) {
    char *str = ft_itoa(y);
    int len = ft_strlen(str);
    if(len < 6){
        while(len < 6){
            x *= 10;
            len++;
        }
    }
    long long int pow = 10;
    while(y >= pow)
        pow *= 10;

    return x * pow + y;
}

void resetValues(char address[3][64], float times[3]){
    int i = 0;
    while(i < 3){
        address[i][0] = '\0';
        times[i] = 0;
        i++;
    }
}

int initialize_socket(int *udpSocket, int *icmpSocket){
    int ttl = 1;
    *udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if(*udpSocket < 0){
        return errno;
    }
    if(setsockopt(*udpSocket, IPPROTO_IP, IP_TTL, &ttl, 4) < 0){
        strerror(errno);
        return errno;
    }

    *icmpSocket = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if(*icmpSocket < 0){
        printf("Error creating socket: %s\n", strerror(errno));
        return errno;
    }
    return 0;
}

void print_line(int ttl, char address[3][64], float times[3]){
    printf("%d %s %0.3f ms %0.3f ms %0.3f ms\n", ttl, address[0], times[0], times[1], times[2]);
    return;
}

int main(int argc, char *argv[])
{
    if(argc < 2){
        return 1;
    }

    fd_set rfds;
    int udpSocket = 0;
    int icmpSocket = 0;
    
    char address[3][64] = {{0}};
    float times[3] = {0, 0, 0};
        
    FD_ZERO(&rfds);
    
    //Set Time
    struct timeval tsend;
    struct timeval trec;
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    tsend.tv_sec = 0;
    tsend.tv_usec = 0;
    trec.tv_sec = 0;
    trec.tv_usec = 0;

    int ttl = 1;
    int addPort = 0;
    int prob = 0;

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

    if(initialize_socket(&udpSocket, &icmpSocket) != 0){
        return 1;
    }

    while(ttl < 10){
        destAddr.sin_port = htons(33434 + addPort);
        addPort++;
        ret = sendto(udpSocket, bufferSend, 32, 0,(struct sockaddr*)(&destAddr), sizeof(struct sockaddr));
        if(ret < 0){
            printf("impossible to send: %s\n", strerror(errno));
            return errno;
        }
        else{
            if(gettimeofday(&tsend, NULL) < 0){
                printf("error getting time %s\n", strerror(errno));
                return errno;
            }
            long long int timeSend = concatenate(tsend.tv_sec, tsend.tv_usec);
            
            printf("packet sent!!!\n");
            FD_SET(icmpSocket,&rfds);
            
            // ajout dans le fd
            ret = select(icmpSocket + 1, &rfds, NULL, NULL, &tv);
            if(ret){
                if(FD_ISSET(icmpSocket, &rfds)){
                    if(gettimeofday(&trec, NULL) < 0){
                        printf("error getting time %s\n", strerror(errno));
                        return errno;
                    }
                    long long int timeRec = concatenate(trec.tv_sec, trec.tv_usec);
                    float time = (float)(timeRec - timeSend) / 1000;
                    ssize_t n = read(icmpSocket, buffer, 1024);
                    if(n < 0){
                        strerror(errno);
                        return errno;
                    }
                    else{
                        ft_strlcpy(address[prob], "test", 5);
                        times[prob] = time;
                        struct iphdr *recIp = (struct iphdr *)buffer;
                        struct icmphdr *recIcmp = (struct icmphdr *)(buffer + sizeof(struct iphdr));
                        struct sockaddr srcAddr;
                        srcAddr.sa_data
                        getnameinfo(recIp->saddr, );
                        printf("rec ip: %u, rec icmp: code %d, type %d, ttl %d\n", recIp->daddr, recIcmp->code, recIcmp->type, ttl);
                        if(recIcmp->code == 3 && recIcmp->type == 3){
                            return 0;
                        }
                    }
                }
            }
            else{
                printf("error timeout ret is %d\n", ret);
                ft_strlcpy(address[prob], "*", 1);
                times[prob] = 0;
            }
            tv.tv_sec = 5;
            tv.tv_usec = 0;
        }
        prob++;
        if(prob >= 3){
            ttl++;
            if(setsockopt(udpSocket, IPPROTO_IP, IP_TTL, &ttl, 4) < 0){
                strerror(errno);
                return -1;
            }
            print_line(ttl, address, times);
            resetValues(address, times);
            prob = 0;
        }
        else{

        }
    }
    return(0);
}