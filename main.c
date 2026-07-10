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

int main(int argc, char *argv[])
{
    if(argc < 2){
        return 1;
    }

    fd_set rfds;
    struct timeval tv;
    int retval;

    /* Surveiller stdin (fd 0) en attente d'entrées */
    FD_ZERO(&rfds);
    FD_SET(0, &rfds);

    /* Pendant 5 secondes maxi */
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    int udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    int value = 1;
    socklen_t sockSize = sizeof(value);
    if(setsockopt(udpSocket, IPPROTO_IP, IP_TTL, &value, sockSize) < 0){
        strerror(errno);
        return(errno);
    }
    
    char buffer[80] = "\0";
    struct sockaddr_in destAddr;
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = 33434;
    
    int ret = inet_pton(AF_INET, argv[1], &(destAddr.sin_addr));
    if(ret < 1){
        strerror(errno);
        return errno;
    }
    printf("the argument is %s and the buffer is %d\n",argv[1], destAddr.sin_addr.s_addr);

    retval = select(1, &rfds, NULL, NULL, &tv);
    socklen_t lenDest = sizeof(destAddr);
    ret = sendto(udpSocket, buffer, 80, 0,(struct sockaddr*)(&destAddr), sizeof(struct sockaddr));
    if(ret < 0){
        printf("impossible to send");
    }
    else{
        if(recvfrom(udpSocket, buffer, 80, 0, (struct sockaddr*)(&destAddr), &lenDest) < 0){
            strerror(errno);
            return errno;
        }
    }
    /* Considérer tv comme indéfini maintenant ! */
    // Dans un while tant que retval n'est pas nul
        // Créer le socket
        // sendto le paquet 
        // select le socket
        // recvfrom

    if (retval == -1)
        perror("select()");
    else if (retval)
        printf("Des données sont disponibles maintenant\n");
        /* FD_ISSET(0, &rfds) est vrai */
    else
        printf("Aucune données durant les 5 secondes\n");

    exit(EXIT_SUCCESS);
}