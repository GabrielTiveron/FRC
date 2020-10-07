#include <sys/time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define T_MX 15
#define PORT 53

void ChangeToDnsNameFormat(unsigned char *dns, unsigned char *host);

typedef struct {
    unsigned short int query;
    uint16_t flags;
    uint16_t question;
    uint16_t answer;
    uint16_t authority;
    uint16_t additional;
} udp_header;

typedef struct {
    uint16_t type;
    uint16_t class;
} infos;

void ChangeToDnsNameFormat(unsigned char *dns, unsigned char *host) {
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

    unsigned char buffer[65536];
    struct sockaddr_in dest;
    struct timeval timeout = {6, 0};

    memset(buffer, 0, 65536);
    udp_header *payload = (udp_header *) buffer;

    payload->query = htons(rand());
    payload->flags = htons(0x0100);
    payload->question = htons(1);
    payload->answer = htons(0);
    payload->authority = htons(0);
    payload->additional = htons(0);

    char *data = (buffer + sizeof(udp_header));
    ChangeToDnsNameFormat(data, msg);
    unsigned int len = strlen(data) + 1;
    infos *end = (infos *) (buffer + sizeof(udp_header) + len);
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

    const int package = sizeof(udp_header) + len + sizeof(infos);

    int n, s, i;
    int server_addr_len;
    for (i = 0; i < 3; i++) {
        s = sendto(socket_fd, buffer, package, 0, (struct sockaddr *) &dest, sizeof(dest));
        n = recvfrom(socket_fd, (char *) buffer, 65536, 0, (struct sockaddr *) &dest, &server_addr_len);
        sleep(2);

        if (n > 0)
            break;
    }
    if (i == 3) {
        printf("Nao foi possível coletar entrada MX para %s\n", msg);
        return 0;
    } else if (buffer[35] % 38) {
        printf("Dominio %s nao encontrado\n", msg);
    } else {
//        if (n > 75) {
  //          printf("Dominio %s nao possui entrada MX\n", msg);
    //        return 0;
      //  }
      
      int len = 1, index = 0, marker = 0;
      char * domain = malloc(100*sizeof(char*));
      for(int k = 12; len > 0; k += len+1){
        len = (int) buffer[k];
        printf("K[%2x] = len[%d]\n", buffer[k], len);
        if(buffer[len+2] == 0xc){printf("BUFFER COMPRESSAO = %02x\n", buffer[(int)buffer[len+3]]);break;}
        for(int l = k+1; l < k+len+1; l++, index++){
          domain[index] = buffer[l];
          marker = l;
        }
        domain[index++] = '.';
      }
      domain[index] = '\0';
      marker += 20;
      char*mail = malloc(200*sizeof(char*));
      len = 1;
      index = 0;

      for(int k = marker; len > 0; k += len+1){
        len = (int)buffer[k];
        for(int l = k+1; l < k+len+1; l++, index++){
          mail[index] = buffer[l];

        }
        mail[index++] = '.';
      }
      mail[index] = '\0';
      printf("MAIL-CERTO: %s\n", mail);
      printf("ANSWERS: %d\n", buffer[7]);















      printf("RES_STRING = %s\n", domain);
      printf("INDEX: %d BUFFER: %c\n", marker, buffer[marker]);
      printf("MAIL: %c%c%c%c%c%c\n", buffer[marker+16], buffer[marker+17], buffer[marker+18], buffer[marker+19], buffer[marker+20], buffer[marker+21]);
        printf("%s <> %c%c%c%c%c%c.%c%c%c%c.%c%c%c%c%c%c%c%c%c%c.%c%c%c%c%c%c%c.%c%c%c\n",
               msg,
               buffer[n - 35], buffer[n - 34], buffer[n - 33], buffer[n - 32], buffer[n - 31], buffer[n - 30],
               buffer[n - 28], buffer[n - 27], buffer[n - 26],
               buffer[n - 25], buffer[n - 23], buffer[n - 22],
               buffer[n - 21], buffer[n - 20], buffer[n - 19], buffer[n - 18], buffer[n - 17], buffer[n - 16],
               buffer[n - 15], buffer[n - 14], buffer[n - 12],
               buffer[n - 11], buffer[n - 10], buffer[n - 9], buffer[n - 8], buffer[n - 7], buffer[n - 6],
               buffer[n - 4], buffer[n - 3], buffer[n - 2]);
    }
    printf("-------------------------------------------------\n");
    for(int g = 0; g < n; g++)
    printf("%02x |", buffer[g]);
    printf("\n");
    printf("-------------------------------------------------\n");
    for(int g = 0; g < n; g++)
    printf("%c|", (char)buffer[g]);
    printf("\n");
    printf("BYTE 47: %02x\n", buffer[12]);
    printf("N: %d\n", n);
    close(socket_fd);
    return 0;
}
