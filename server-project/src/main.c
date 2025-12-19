#if defined WIN32
#include <winsock.h>
#else
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#define closesocket close
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <time.h>

#include "protocol.h"

#define NO_ERROR 0

void clearwinsock() {
#if defined WIN32
    WSACleanup();
#endif
}

void errorhandler(char *errorMessage) {
    perror(errorMessage);
}

int main(int argc, char *argv[]) {

    srand(time(NULL));
    int port = SERVER_PORT;

    for (int i = 1; i < argc; i++){
        if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            port = atoi(argv[i+1]);
            i++;
            if (port <= 0) {
                printf("Numero di porta errato\n");
                return 0;
            }
        }
    }

#if defined WIN32
    WSADATA wsa_data;
    int result = WSAStartup(MAKEWORD(2,2), &wsa_data);
    if (result != NO_ERROR) {
        printf("Error at WSAStartup()\n");
        return 0;
    }
#endif

    int my_socket;
    my_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (my_socket < 0) {
        errorhandler("socket creation failed");
        clearwinsock();
        return -1;
    }

    struct sockaddr_in echoClntAddr;
    unsigned int cliAddrLen;
    char echoBuffer[ECHOMAX];
    int recvMsgSize;

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(my_socket, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0) {
        errorhandler("bind() failed");
        closesocket(my_socket);
        clearwinsock();
        return -1;
    }

    printf("Waiting for a client to connect...\n");

    while (1) {
        cliAddrLen = sizeof(echoClntAddr);

        weather_request_t req;
        weather_response_t res;

        res.status = 0;
        res.type = 'N';
        res.value = 0.0;

        const char *citta[] = {
            "BARI","ROMA","MILANO","NAPOLI","TORINO",
            "PALERMO","GENOVA","BOLOGNA","FIRENZE","VENEZIA"
        };

        recvMsgSize = recvfrom(my_socket, echoBuffer, ECHOMAX, 0,
            (struct sockaddr*)&echoClntAddr, &cliAddrLen);

        if (recvMsgSize != sizeof(weather_request_t)) continue;

        memcpy(&req, echoBuffer, sizeof(weather_request_t));

        struct hostent *host = gethostbyaddr(
            (char *)&echoClntAddr.sin_addr,
            sizeof(echoClntAddr.sin_addr),
            AF_INET);

        printf("Richiesta ricevuta da %s (ip %s): type='%c', city='%s'\n",
            host ? host->h_name : "unknown",
            inet_ntoa(echoClntAddr.sin_addr),
            req.type, req.city);

        int trovata = 0;
        strmMaiusc(req.city);

        for (int i = 0; i < 10; i++) {
            if (strcmp(req.city, citta[i]) == 0) {
                trovata = 1;
                break;
            }
        }

        if (!trovata) {
            res.status = 1;
            conversione(&res);
            if (sendto(my_socket, (char*)&res, sizeof(res), 0,
                (struct sockaddr*)&echoClntAddr, cliAddrLen) < 0) {
                errorhandler("sendto() failed");
            }
            continue;
        }

        switch(req.type) {
            case 't': res.value = get_temperature(); break;
            case 'h': res.value = get_humidity(); break;
            case 'w': res.value = get_wind(); break;
            case 'p': res.value = get_pressure(); break;
            default:
                res.status = 2;
                res.type = 'N';
                res.value = 0.0;
        }

        res.type = req.type;
        conversione(&res);

        if (sendto(my_socket, (char*)&res, sizeof(res), 0,
            (struct sockaddr*)&echoClntAddr, cliAddrLen) < 0) {
            errorhandler("sendto() failed");
        }
    }

    closesocket(my_socket);
    clearwinsock();
    return 0;
}

float get_temperature(void) {
    float min = -10.0, max = 40.0;
    return min + (rand() / (float)RAND_MAX) * (max - min);
}

float get_humidity(void){
    float min = 20.0, max = 100.0;
    return min + (rand() / (float)RAND_MAX) * (max - min);
}

float get_wind(void){
    float min = 0.0, max = 100.0;
    return min + (rand() / (float)RAND_MAX) * (max - min);
}

float get_pressure(void){
    float min = 950.0, max = 1050.0;
    return min + (rand() / (float)RAND_MAX) * (max - min);
}

void strmMaiusc(char *s) {
    while (*s) {
        *s = toupper(*s);
        s++;
    }
}

void conversione(weather_response_t *res){
    uint32_t status_net = htonl(res->status);
    memcpy(&(res->status), &status_net, sizeof(uint32_t));

    uint32_t raw_float;
    memcpy(&raw_float, &(res->value), sizeof(float));
    raw_float = htonl(raw_float);
    memcpy(&(res->value), &raw_float, sizeof(float));
}
