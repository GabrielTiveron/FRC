#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <netinet/in.h>

#define T_MX 15
#define PORT 53

void DnsFormat(unsigned char *dns, unsigned char *host);

typedef struct {
    uint16_t type;
    uint16_t class;
} query;

typedef struct {
    unsigned short int transaction;
    uint16_t flags;
    uint16_t question;
    uint16_t answer;
    uint16_t authority;
    uint16_t additional;
} udp;


void DnsFormat(unsigned char *dns, unsigned char *host) {
    int lock = 0, i;
    char *host_aux = (char *) (malloc(sizeof(host) + 1));
    strcpy(host_aux, host);
    strcat((char *) host_aux, ".");

    for (i = 0; i < strlen((char *) host_aux); i++) {
        if (host_aux[i] == '.') {
            *dns++ = i - lock;
            for (; lock < i; lock++) {
                *dns++ = host_aux[lock];
            }
            lock = i + 1;
        }
    }
}

int main(int argc, char **argv) {
    unsigned char *msg = argv[1];
    unsigned char *address = argv[2];

    unsigned char package[65536];
    struct sockaddr_in dest;
    struct timeval timeout = {6, 0};

    memset(package, 0, 65536);
    udp *payload = (udp *) package;

    payload->transaction = htons(rand());
    payload->flags = htons(0x0100);
    payload->question = htons(1);
    payload->answer = htons(0);
    payload->authority = htons(0);
    payload->additional = htons(0);

    char *data = (package + sizeof(udp));
    DnsFormat(data, msg);
    unsigned int len = strlen(data) + 1;
    query *end = (query *) (package + sizeof(udp) + len);
    end->type = htons(T_MX);
    end->class = htons(1);

    int socket_fd;
    if ((socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        printf("Erro na criacao do socket\n");
        return 0;
    }

    dest.sin_family = AF_INET;
    dest.sin_port = htons(PORT);
    dest.sin_addr.s_addr = inet_addr(address);
    setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(struct timeval));

    const int package_len = sizeof(udp) + len + sizeof(query);

    int n, s, i;
    int server_addr_len;
    for (i = 0; i < 3; i++) {
        s = sendto(socket_fd, package, package_len, 0, (struct sockaddr *) &dest, sizeof(dest));
        n = recvfrom(socket_fd, (char *) package, 65536, 0, (struct sockaddr *) &dest, &server_addr_len);
        sleep(2);

        if (n > 0)
            break;
    }
    if (i == 3) {
        printf("Nao foi possível coletar entrada MX para %s\n", msg);
        return 0;
    } else if (package[35] % 38) {
        printf("Dominio %s nao encontrado\n", msg);
    } else {
        if (n > 75) {
            printf("Dominio %s nao possui entrada MX\n", msg);
            return 0;
        }

        printf("%s <> %c%c%c%c%c%c.%c%c%c%c.%c%c%c%c%c%c%c%c%c%c.%c%c%c%c%c%c%c.%c%c%c\n",
               msg,
               package[n-35], package[n-34], package[n-33], package[n-32], package[n - 31], package[n - 30],
               package[n-28], package[n-27], package[n-26],
               package[n-25], package[n-23], package[n-22],
               package[n-21], package[n-20], package[n-19], package[n-18], package[n-17], package[n-16],
               package[n-15], package[n-14], package[n-12],
               package[n-11], package[n-10], package[n-9], package[n - 8], package[n - 7], package[n - 6],
               package[n-4],  package[n-3],  package[n-2]);
    }
    close(socket_fd);
    return 0;
}
