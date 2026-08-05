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

int getDestAddr(char *argv, struct sockaddr_in *addrDest, char *buffer){
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    char ipstr[INET_ADDRSTRLEN];
    // char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    
    int ret = getaddrinfo(argv, NULL, &hints, &result);
    if(ret != 0){
        if(ret == -2){
            printf("ping: %s: Name or service not known\n", argv);
            freeaddrinfo(result);
        }
        else if (ret == -3){
            printf("ping: %s: Temporary failure in name resolution\n", argv);
            freeaddrinfo(result);
        }
        return 2;
    }
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        if (rp->ai_family == AF_INET) {
            struct sockaddr_in *temp = (struct sockaddr_in *)rp->ai_addr;
            memcpy(addrDest, (struct sockaddr_in *)rp->ai_addr, sizeof(struct sockaddr_in));
            inet_ntop(rp->ai_family, &(temp->sin_addr), ipstr, sizeof(ipstr));
			ft_memcpy(buffer, ipstr, ft_strlen(ipstr));
            // if (getnameinfo((struct sockaddr*)temp, sizeof(struct sockaddr), hbuf, sizeof(hbuf), sbuf,
            //         sizeof(sbuf), NI_NAMEREQD) == 0){
            //         ft_memcpy(buffer, hbuf, sizeof(hbuf));
			// 		printf("hbuf is %s\n", hbuf);
            // }
            freeaddrinfo(result);
            return 0;
        }
    }
    freeaddrinfo(result);
    printf("ping: %s: Temporary failure in name resolution\n", argv);
    return 2;
}

int main(int argc, char *argv[])
{
    if(argc < 2){
		printf("Usage:\n traceroute <domain or address to search>");
		return 0;
    }
	if(argc > 3){
		printf("My traceroute implementation accept only two arguments")
		return 2;
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

	char destHostname[NI_MAXHOST] = "\0";
    struct sockaddr_in destAddr;
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(33434);

	unsigned char buffer[1024] = "\0";
    char bufferSend[32] = "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_";
    int ret = inet_pton(AF_INET, argv[1], &(destAddr.sin_addr));
	if(!ret){
		ret = getDestAddr(argv[1], &destAddr, destHostname);
		if(ret != 0)
        	return ret;
	}
    else if(ret < 1){
        strerror(errno);
        return errno;
    }

    if(initialize_socket(&udpSocket, &icmpSocket) != 0){
        return 1;
    }

	printf("traceroute to %s (%s) , 30 hops max, 60 byte packets\n", argv[1], destHostname);

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

            FD_SET(icmpSocket,&rfds);
        
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
                        // printf("-%s- -%s-\n", buffer1, buffer2);
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
			ft_memset(buffer1, 0, 1024);
            if(stop)
				return 0;
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