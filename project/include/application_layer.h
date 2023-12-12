// Application layer protocol header.
// NOTE: This file must not be changed.

#ifndef _APPLICATION_LAYER_H_
#define _APPLICATION_LAYER_H_

#include "link_layer.h"

#define MAX_BUF_SIZE 256
#define TRANSMITTER 0
#define RECEIVER 1

#define DATA 1
#define START_PACKET 2
#define END_PACKET 3

#define MAX_DATA_SIZE 256
#define DATA_SIZE 100

// Application layer main function.
void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename);
// Lógica da aplication layer para o transmissor
int enviaFile(const char *filename);
// Lógica da aplication layer para o recetor
int recebeFile(const char *filename);
// construct control packet - retorna o tamanho do packet
int constructControlPacket(unsigned char *packet, unsigned char control, const char *fName, long int fLength);
// deconstruct control packet - retorna 0 se tudo estiver ok e -1 se houver algum erro
int deconstructControlPacket(unsigned char *packet, unsigned char control, char *fName, int *fLength);
// construct data packet - retorna o seu tamanho
unsigned char* constructDataPacket(unsigned int size, unsigned char *packet, long int *packetSize);

#endif // _APPLICATION_LAYER_H_
