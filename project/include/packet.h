#ifndef _PACKET_H_
#define _PACKET_H_

#define DATA 1
#define START_PACKET 2
#define END_PACKET 3

#define MAX_DATA_SIZE 256
#define DATA_SIZE 100

// construct control packet - retorna a length
int constructControlPacket(unsigned char *packet, unsigned char control, const char *fName, long int fLength);
// deconstruct control packet - retorna 0 se funcionar, -1 se der erro
int deconstructControlPacket(unsigned char *packet, unsigned char control, char *fName, int *fLength);

// construct data packet - retorna o seu size
int constructDataPacket(unsigned char *packet, unsigned int size, unsigned int index);
// deconstruct data packet
void deconstructDataPacket(unsigned char *packet, unsigned char *data, int *dataSize);

#endif
