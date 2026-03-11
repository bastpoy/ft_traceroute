#include <stdio.h>
#include <sys/select.h>
#include <sys/types.h>

int main(void) {
    fd_set readfds;
    struct timeval timeout;
    (void)timeout;

    FD_ZERO(&readfds);
    FD_SET(0, &readfds); // surveille stdin (fd = 0)

    timeout.tv_sec  = 5; // attendre max 5 secondes
    timeout.tv_usec = 0;

    int ret = select(0 + 1, &readfds, NULL, NULL, NULL);

    if (ret == -1) {
        perror("select");
    } else if (ret == 0) {
        printf("Timeout : aucune saisie en 5 secondes.\n");
    } else {
        if (FD_ISSET(0, &readfds)) {
            char buf[128];
            fgets(buf, sizeof(buf), stdin);
            printf("Vous avez saisi : %s", buf);
        }
    }
    return 0;
}