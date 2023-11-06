#include "packet.h"

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

int constructControlPacket(unsigned char *packet, unsigned char control, const char *fName, long int fLength)
{
    packet[0] = control;

    // first TLV
    packet[1] = 0;
    int fSize = 0;

    long int auxFileSize = fLength;

    while (auxFileSize > 0)
    {
        auxFileSize >>= 8;
        fSize++;
    }

    packet[2] = fSize;

    // serialize the file length into control packet
    for (int i = fSize - 1; i >= 0; i--)
    {
        packet[3 + ((fSize - 1) - i)] = (fLength >> (i * 8)) & 0xFF;
    }

    // second TLV
    packet[3 + fSize] = 1;
    int fNameSize = strlen(fName);
    packet[4 + fSize] = fNameSize;

    // serialize the file name into control packet
    for (int i = 0; i < fNameSize; i++)
    {
        packet[6 + fSize - 1 + i] = fName[i];
    }

    return 6 + fSize - 1 + fNameSize;
}

int deconstructControlPacket(unsigned char *packet, unsigned char control, char *fName, int *fLength)
{
    
    
    if (packet[4] != control)
    {
        printf("Control packet incorrect, packet[0] = %02X\n", packet[0]);
        return -1;
    }

    int fSize;

    if (packet[5] == 0)
    {
        fSize = packet[6];

        // save file length
        for (int i = 0; i < fSize; i++)
        {
            *fLength = (*fLength) * 256 + packet[7 + i];
        }

        int fNameSize;

        if (packet[8 + packet[6] - 1] == 1)
        {
            fNameSize = packet[9 + packet[6] - 1];

            // save file name
            for (int i = 0; i < fNameSize; i++)
            {
                fName[i] = packet[10 + packet[6] - 1 + i];
            }
        }

        return 0;
    }
    return -1;
}

int constructDataPacket(unsigned char *packet,unsigned int size, unsigned int index)
{
    unsigned char buf[500] = {0};

    buf[0] = 0x01;
    buf[1] = index % 255;
    buf[2] = size/256;
    buf[3] = size%256;

    for(int i = 4; i < size; i++){
        buf[i] = packet[i];
    }

    for(int j = 0; j < (size + 4); j++){
        packet[j] = buf[j];
    }

    return (size + 4);
}

void deconstructDataPacket(unsigned char *packet, unsigned char *data, int *dataSize)
{
    *dataSize = packet[1] * 256 + packet[2];

    for (int i = 0; i < *dataSize; i++)
    {
        data[i] = packet[i + 4];
    }
}
