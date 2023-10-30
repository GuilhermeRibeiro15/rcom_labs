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

int constructControlPacket(unsigned char *packet, unsigned char control, const char *fName, unsigned char fLength)
{
    packet[0] = control;

    // first TLV
    packet[1] = 0;
    int fSize = 0, auxiliar_length = fLength;

    while (auxiliar_length > 256)
    {
        auxiliar_length /= 256;
        fSize++;
    }
    if (auxiliar_length > 0)
        fSize++;

    packet[2] = fSize;

    // serialize the file length into control packet
    for (int i = fSize - 1; i >= 0; i--)
    {
        packet[3 + ((fSize - 1) - i)] = (fLength >> (i * 8)) & 0xFF;
    }

    // second TLV
    packet[4 + fSize - 1] = 1;
    int fNameSize = strlen(fName) + 1;
    packet[5 + fSize - 1] = fNameSize;

    // serialize the file name into control packet
    for (int i = 0; i < fNameSize; i++)
    {
        packet[6 + fSize - 1 + i] = fName[i];
    }

    return 6 + fSize - 1 + fNameSize;
}

int deconstructControlPacket(unsigned char *packet, unsigned char control, char *fName, int *fLength)
{
    if (packet[0] != control)
    {
        printf("Control packet incorrect\n");
        return -1;
    }

    int fSize;

    if (packet[1] == 0)
    {
        fSize = packet[2];

        // save file length
        for (int i = 0; i < fSize; i++)
        {
            *fLength = (*fLength) * 256 + packet[3 + i];
        }

        int fNameSize;

        if (packet[4 + packet[2] - 1] == 1)
        {
            fNameSize = packet[5 + packet[2] - 1];

            // save file name
            for (int i = 0; i < fNameSize; i++)
            {
                fName[i] = packet[6 + packet[2] - 1 + i];
            }
        }

        return 0;
    }
    return -1;
}

int constructDataPacket(unsigned char *packet, unsigned char *data, int dataSize)
{
    packet[0] = 1;
    packet[1] = dataSize / 256;
    packet[2] = dataSize % 256;

    memcpy(packet + 3, data, dataSize); 
    // memcpy- Copies the values of num bytes from the location pointed to by source directly to the memory block pointed to by destination.

    return 4 + dataSize;
}

void deconstructDataPacket(unsigned char *packet, unsigned char *data, int *dataSize)
{
    *dataSize = packet[1] * 256 + packet[2];

    for (int i = 0; i < *dataSize; i++)
    {
        data[i] = packet[i + 4];
    }
}
