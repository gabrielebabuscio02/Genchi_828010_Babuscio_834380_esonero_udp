#ifndef PROTOCOL_H_
#define PROTOCOL_H_

#include <stdint.h>

// Shared application parameters
#define SERVER_PORT 56700  // Server port (change if needed)
#define BUFFER_SIZE 512    // Buffer size for messages
#define QUEUE_SIZE 5       // Size of pending connections queue
#define ECHOMAX 255
#define PORT 48000



/*
 * Example function to implement:
 * int handle_client(int client_socket);
 */
typedef struct {
    char type;        // Weather data type: 't', 'h', 'w', 'p'
    char city[64];    // City name (null-terminated string)
} weather_request_t;


typedef struct {
    unsigned int status;  // Response status code
    char type;            // Echo of request type
    float value;          // Weather data value
} weather_response_t;

// Function prototypes
// Add here the signatures of the functions implemented by students
float get_temperature(void);    // Range: -10.0 to 40.0 °C
float get_humidity(void);       // Range: 20.0 to 100.0 %
float get_wind(void);           // Range: 0.0 to 100.0 km/h
float get_pressure(void);       // Range: 950.0 to 1050.0 hPa
void strmMaiusc(char *s);       //Forza una stinga in maiuscolo
void errorhandler(char *errorMessage);
void conversione(weather_response_t *res);



#endif /* PROTOCOL_H_ */
