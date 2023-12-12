// Link layer header.
// NOTE: This file must not be changed.

#ifndef _LINK_LAYER_H_
#define _LINK_LAYER_H_

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>

typedef enum
{
    LlTx,
    LlRx,
} LinkLayerRole;

typedef struct
{
    char serialPort[50];
    LinkLayerRole role;
    int baudRate;
    int nRetransmissions;
    int timeout;
} LinkLayer;

typedef enum
{
    START,
    FLAG_T,
    A_T,
    C_T,
    BCC_T,
    END,
    DATA_T,
    ESC_T,
    R_PACKET,
} StateMachine;

// SIZE of maximum acceptable payload.
// Maximum number of bytes that application layer should send to link layer
#define MAX_PAYLOAD_SIZE 1000

// MISC
#define FALSE 0
#define TRUE 1

#define FLAG 0x7E

#define A 0x03
#define C 0x03
#define BCC (A ^ C)

#define A_UA 0x01
#define C_UA 0x07
#define BCC_UA (A_UA ^ C_UA)

#define C_DISC 0x0B
#define BCC_DISC (A ^ C_DISC)

#define BUF_SIZE 5

// Open a connection using the "port" parameters defined in struct linkLayer.
int llopen(LinkLayer connectionParameters);
// Send data in buf with size bufSize.
int llwrite(const unsigned char *buf, int bufSize);
// Receive data in packet.
int llread(unsigned char *packet, long int* p_size);
// Close previously opened connection.
int llclose(int showStatistics);

#endif // _LINK_LAYER_H_
