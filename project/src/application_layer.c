// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

LinkLayer linklayer;

void applicationLayer(const char *serialport, const char *role, int baudrate,
                      int nTries, int timeOut, const char *filename)
{
    linklayer.baudRate = baudrate;
    linklayer.nRetransmissions = nTries;
    linklayer.timeout = timeOut;
    *linklayer.serialPort = (char)*serialport;

    if (strcmp(role, "tx") == 0){
        linklayer.role = LlTx;
        enviaFile(filename);
    }
    else if (strcmp(role, "rx") == 0){
       linklayer.role = LlRx;
       recebeFile(filename); 
    }
    else{
        printf("Invalid role\n");
        return;
    }
}


int enviaFile(const char *filename)
{
    // open file to read
    FILE *file = fopen(filename, "r");

    if (file == NULL)
    {
        printf("Couldn't open file\n");
        return -1;
    }

    // open port
    if (llopen(linklayer) < 0)
    {
        printf("Couldn't establish connection with receiver\n");
        closePort();
        return -1;
    }

    printf("Port open and comunication done\n");

    // calculate file size by going to the end of the file and asking for the position
    fseek(file, 0, SEEK_END);
    int fileLength = ftell(file);
    // return to the beginning of the file
    rewind(file);

    // build START control packet
    unsigned char controlStart[MAX_BUF_SIZE];
    int controlStartSize = constructControlPacket(controlStart, START, filename, fileLength);

    printf("Sending initial control packet\n");
    if (llwrite(controlStart, controlStartSize) < 0)
    {
        printf("Couldn't write initial control packet\n");
        closePort();
        return -1;
    }

    unsigned char data[MAX_DATA_SIZE];
    unsigned char dataPacket[MAX_DATA_SIZE];

    int dataSize = 0;
    int dataPacketSize = 0;

    while (TRUE)
    {
        dataSize = fread(data, sizeof(char), DATA_SIZE, file);

        // build information read from the file
        dataPacketSize = constructDataPacket(dataPacket, data, dataSize);

        // If this happens we are in the last data
        if (dataSize < DATA_SIZE)
        {
            if (feof(file))
            {
                printf("Sending last info packet\n");
                if (llwrite(dataPacket, dataPacketSize) < 0)
                {
                    printf("Couldn't write file info\n");
                    closePort();
                    return -1;
                }
                break;
            }
            else
            {
                printf("Reading less bytes than supposed and it is not end of file\n");
                closePort();
                return -1;
            }
        }

        printf("Sending info packet\n");
        if (llwrite(dataPacket, dataPacketSize) < 0)
        {
            printf("Couldn't write file info\n");
            closePort();
            return -1;
        }
    }

    // build END control packet
    unsigned char controlEnd[DATA_SIZE + 4];
    int controlEndSize = constructControlPacket(controlEnd, END, filename, fileLength);

    printf("Sending ending control packet\n");
    if (llwrite(controlEnd, controlEndSize) < 0)
    {
        printf("Couldn't write ending control packet\n");
        closePort();
        return -1;
    }

    if (fclose(file) != 0)
    {
        printf("Error closing file\n");
        closePort();
        return -1;
    }

    // close port
    if (llclose(TRANSMITTER) < 0)
    {
        printf("Couldn't close process\n");
        return -1;
    }
    printf("Port closed\n");

    return 0;
}

int recebeFile(const char *filename)
{
    // open port
    if (llopen(linklayer) < 0)
    {
        printf("Couldn't establish connection with transmitter\n");
        closePort();
        return -1;
    }

    unsigned char control[MAX_DATA_SIZE];

    printf("Reading initial control packet\n");
    if (llread(control) < 0)
    {
        printf("Error reading control packet");
        closePort();
        return -1;
    }

    char fileName[MAX_BUF_SIZE];
    int fileLength = 0;

    // disassemble START control packet
    if (deconstructControlPacket(control, START, fileName, &fileLength) < 0)
    {
        printf("Initial control packet wrong\n");
        closePort();
        return -1;
    }

    // open file to write
    FILE *file = fopen(fileName, "w");

    if (file == NULL)
    {
        printf("Couldn't open file\n");
        closePort();
        return -1;
    }

    unsigned char dataPacket[MAX_DATA_SIZE];
    unsigned char data[MAX_DATA_SIZE];
    int dataSize = 0;

    while (TRUE)
    {
        printf("Reading info packet\n");
        if (llread(dataPacket) < 0)
        {
            printf("Error reading info packet");
            return -1;
        }

        // data to write to the file was read
        if (dataPacket[0] == DATA)
        {
            // deconstruct information read from the serial port
            deconstructDataPacket(dataPacket, data, &dataSize);

            printf("Writing information to file\n");
            fwrite(data, 1, dataSize, file);
        }

        // ending control packet was read
        else if (dataPacket[0] == END)
            break;
    }

    char fileNameEnd[MAX_BUF_SIZE];
    int fileLengthEnd = 0;

    printf("Ending control packet detected\n");
    // deconstruct START control packet
    if (deconstructControlPacket(dataPacket, END, fileName, &fileLength) < 0)
    {
        printf("Ending control packet wrong\n");
        return -1;
    }

    // compare if START control packet equals END control packet
    if (strcmp(fileName, fileNameEnd) != 0 || fileLength != fileLengthEnd)
    {
        printf("Control packets don't match\n");
        return -1;
    }

    if (fclose(file) != 0)
    {
        closePort();
        printf("Error closing file\n");
        return -1;
    }

    // close port
    if (llclose(RECEIVER) < 0)
    {
        printf("Couldn't close file\n");
        closePort();
        return -1;
    }

    printf("Port closed\n");

    return 0;
}
