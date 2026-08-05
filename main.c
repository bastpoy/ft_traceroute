#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
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


int main(int argc, char *argv[])
{
    if(argc < 2){
        return 1;
    }
	bool stop = false;

    fd_set rfds;
    int udpSocket = 0;
    int icmpSocket = 0;
    
    char buffer1[1024] = "\0";
        
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
            
            if(prob % 3 == 0)
                printf(" %d  ", ttl);

            // printf("packet sent!!!\n");
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
                        // ft_strlcpy(address[prob], "test", 5);
                        // times[prob] = time;
                        struct iphdr *recIp = (struct iphdr *)buffer;
                        struct icmphdr *recIcmp = (struct icmphdr *)(buffer + sizeof(struct iphdr));

                        //create address to have the src domain
                        struct sockaddr_in srcAddr;
                        srcAddr.sin_family = AF_INET;
                        srcAddr.sin_port = htons(11111);
                        srcAddr.sin_addr.s_addr = recIp->saddr;
                        char hbuf[1024], sbuf[1024];    
                        ret = getnameinfo((struct sockaddr*)&srcAddr, sizeof(struct sockaddr), hbuf, sizeof(hbuf), sbuf, sizeof(sbuf), NI_NAMEREQD);
                        if(ret != 0 && ret != EAI_NONAME){
                            printf("Error getting address Name: %s %d\n", gai_strerror(ret), ret);
                            return errno;
                        }
                        
						char *strAddr = inet_ntoa(srcAddr.sin_addr);

                        char buffer2[1024];
                        ft_strlcpy(buffer2, hbuf, ft_strlen(hbuf) + 1);
						char *strTemp = ft_strjoin(buffer2, " (");
						strTemp = ft_strjoin(strTemp, strAddr);
						strTemp = ft_strjoin(strTemp, ")");
                        printf("-%s- -%s-\n", buffer1, buffer2);
                        if(!ft_strncmp(buffer1, buffer2, ft_strlen(buffer2)))
                            printf("%0.3f ms  ", time);
                        else
                            printf("%s  %0.3f ms  ", strTemp, time);
                        ft_strlcpy(buffer1, buffer2, ft_strlen(buffer2) + 1);

                        if(recIcmp->code == 3 && recIcmp->type == 3){
                            stop = true;
                        }
                    }
                }
            }
            else{
                printf("*  ");
            }
            tv.tv_sec = 5;
            tv.tv_usec = 0;
        }
        prob++;
        if(prob >= 3){
            printf("\n");
			// print_line(ttl, address, times);
            if(stop)
				return 0;
			// resetValues(address, times);
            prob = 0;
			ttl++;
			if(setsockopt(udpSocket, IPPROTO_IP, IP_TTL, &ttl, 4) < 0){
				strerror(errno);
                return -1;
            }
        }
    }
    return(0);
}