#if defined WIN32
#include <winsock.h>
typedef int socklen_t;
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
#include <ctype.h>
#include "protocol.h"

#define NO_ERROR 0

void errorhandler(char *msg) {
    printf("%s\n", msg);
}

void clearwinsock() {
#if defined WIN32
	WSACleanup();
#endif
}

int main(int argc, char *argv[]) {

#if defined WIN32
	// Initialize Winsock
	WSADATA wsa_data;
	int result = WSAStartup(MAKEWORD(2,2), &wsa_data);
	if (result != NO_ERROR) {
		printf("Error at WSAStartup()\n");
		return 0;
	}
#endif

	//controllo parametri di input
		char *server_hostname = "localhost";
	    int server_port = SERVER_PORT;
	    char *request_str = NULL;
	    char resolved_ip_str[16];
	    char resolved_hostname_str[256];

	    strncpy(resolved_ip_str, "127.0.0.1", sizeof(resolved_ip_str));
	        strncpy(resolved_hostname_str, "localhost", sizeof(resolved_hostname_str));
	        // --------------------------------------------------

	        for (int i = 1; i < argc; i++) {
	            if (strcmp(argv[i], "-s") == 0) {
	                if (i + 1 < argc) {
	                    server_hostname = argv[i+1]; // Punta all'argomento
	                    i++;
	                }
	        } else if (strcmp(argv[i], "-p") == 0) {
	            if (i + 1 < argc) {
	                server_port = atoi(argv[i+1]);
	                i++;
	            }
	        } else if (strcmp(argv[i], "-r") == 0) {
	            if (i + 1 < argc) {
	                request_str = argv[i+1];
	                i++;
	            }
	        }
	    }

	    if (request_str == NULL) {
	        errorhandler("Errore: Il parametro '-r request' è obbligatorio.\n");
	        printf("Utilizzo: ./client-project [-s server] [-p port] -r \"tipo città\"\n");
	        return 1;
	    }

	    if (strchr(request_str, '\t') != NULL) {
	            errorhandler("Errore: La richiesta non può contenere caratteri di tabulazione (\\t).\n");
	            return 1;
	        }

	    if (strlen(request_str) < 2) {
	        errorhandler("Errore: La richiesta deve contenere almeno tipo e città.\n");
	        return 1;
	    }

	    if(request_str[1]!=' '){
	    	errorhandler("Tipo errato");
	    	return 1;
	    }


	    //creazione richiesta
    weather_request_t req;
    memset(&req, 0, sizeof(req));

    req.type = request_str[0];

        int start_index = 1;
        char *city_ptr = request_str + start_index;

	        if (strlen(city_ptr) > 63) {
	            errorhandler("Errore: Nome città troppo lungo (max 63 caratteri). Richiesta annullata.");
	            return 1;
	        }
        while (request_str[start_index] == ' ') {
            start_index++;
        }
        snprintf(req.city, sizeof(req.city), "%s", request_str + start_index);

		//creazione socket
    int my_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (my_socket < 0) {
        errorhandler("creazione socket fallita");
        clearwinsock();
        return -1;
    }

    struct sockaddr_in echo_server_addr;
        memset(&echo_server_addr, 0, sizeof(echo_server_addr));
        echo_server_addr.sin_family = AF_INET;
        echo_server_addr.sin_port = htons(server_port);

        struct in_addr server_ip_bin;

        if (isalpha(server_hostname[0])) {
			struct hostent *host_info = gethostbyname(server_hostname);
			if (host_info == NULL) {
				errorhandler("Risoluzione DNS fallita");
				closesocket(my_socket);
				clearwinsock();
				return -1;
			}
			memcpy(&server_ip_bin.s_addr, host_info->h_addr_list[0], host_info->h_length);

			strncpy(resolved_hostname_str, server_hostname, sizeof(resolved_hostname_str) - 1);
			strncpy(resolved_ip_str, inet_ntoa(server_ip_bin), sizeof(resolved_ip_str) - 1);

		} else {
			server_ip_bin.s_addr = inet_addr(server_hostname);
			strncpy(resolved_ip_str, server_hostname, sizeof(resolved_ip_str) - 1);
			if (strcmp(server_hostname, "127.0.0.1") == 0) {
				 strncpy(resolved_hostname_str, "localhost", sizeof(resolved_hostname_str) - 1);
			} else {
				 struct hostent *host_reverse = gethostbyaddr((char *)&server_ip_bin, sizeof(struct in_addr), AF_INET);
				 if (host_reverse != NULL) {
					 strncpy(resolved_hostname_str, host_reverse->h_name, sizeof(resolved_hostname_str) - 1);
				 } else {
					 strncpy(resolved_hostname_str, server_hostname, sizeof(resolved_hostname_str) - 1);
				 }
			}
		}

		echo_server_addr.sin_addr.s_addr = server_ip_bin.s_addr;

    //invio dati al server
    if (sendto(my_socket, &req, sizeof(req), 0, (struct sockaddr*)&echo_server_addr, sizeof(echo_server_addr)) != sizeof(req)){
        errorhandler("send() fallito");
        closesocket(my_socket);
        clearwinsock();
        return -1;
    }

    //ricezione dati dal server
    struct sockaddr_in from_server_addr;
    socklen_t addr_len = sizeof(from_server_addr);
    weather_response_t resp;
    memset(&resp, 0, sizeof(resp));

    ssize_t bytes_rcvd = recvfrom(my_socket,&resp,sizeof(resp),0,(struct sockaddr*)&from_server_addr,&addr_len);

    if (bytes_rcvd != sizeof(weather_response_t)) {
        errorhandler("Risposta UDP di dimensione errata");
        return -1;
    }
    resp.status = ntohl(resp.status);

    //conversione del tipo
    uint32_t raw_bytes;

    memcpy(&raw_bytes, &resp.value, sizeof(resp.value));

    raw_bytes = ntohl(raw_bytes);

    memcpy(&resp.value, &raw_bytes, sizeof(resp.value));

    //creazione messaggio di output
    switch(resp.status){//fare if per vedere se è ip numeri o DNS
    case 0:
       if (req.type == 't') {
          printf("Ricevuto risultato dal server %s (ip %s). %s: Temperatura = %.1f°C\n", resolved_hostname_str, resolved_ip_str, req.city, resp.value);
        }
       else if (req.type == 'h') {
          printf("Ricevuto risultato dal server %s (ip %s). %s: Umidità = %.1f%%\n", resolved_hostname_str, resolved_ip_str, req.city, resp.value);
        }
       else if (req.type == 'w') {
          printf("Ricevuto risultato dal server %s (ip %s). %s: Vento = %.1fkm/h\n", resolved_hostname_str, resolved_ip_str, req.city, resp.value);
        }
       else if (req.type == 'p') {
          printf("Ricevuto risultato dal server %s (ip %s). %s: Pressione = %.1fhPa\n", resolved_hostname_str, resolved_ip_str, req.city, resp.value);
       }
       break;
       case 1:
          fprintf(stderr, "Ricevuto risultato dal server %s (ip %s). Città non disponibile\n", resolved_hostname_str, resolved_ip_str);
       break;
       case 2:
          fprintf(stderr, "Ricevuto risultato dal server %s (ip %s). Richiesta non valida\n", resolved_hostname_str, resolved_ip_str);
       break;
    }

    //chiusura socket
    closesocket(my_socket);
	clearwinsock();
	return 0;
} // main end
