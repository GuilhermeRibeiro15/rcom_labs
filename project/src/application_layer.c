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
    
    strcpy(linklayer.serialPort, serialport);



    if (strcmp(role, "tx") == 0){
        linklayer.role = LlTx;
        printf("TRANSMITTER");
        printf("%s", filename);            
        enviaFile(filename);
    }
    else if (strcmp(role, "rx") == 0){
       linklayer.role = LlRx;
        printf("RECEIVER");
        printf("%s", filename);            
       recebeFile(filename); 
    }
    else{
        printf("Invalid role\n");
        return;
    }
}


int enviaFile(const char *filename)
{

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
        llclose(TRANSMITTER);
        return -1;
    }

    printf("Port open and comunication done\n");

    // calculate file size by going to the end of the file and asking for the position
    fseek(file, 0L, SEEK_END);
    long int fileLength = ftell(file);
    fseek(file, 0L, SEEK_SET);

    // build START control packet
    unsigned char controlStart[MAX_BUF_SIZE];
    int controlStartSize = constructControlPacket(controlStart, START_PACKET, filename, fileLength);

    for(int i = 0; i < controlStartSize; i++){
        printf("Control packet start[%d] = %x \n", i, controlStart[i]);
    }

    printf("Sending initial control packet\n");
    if (llwrite(controlStart, controlStartSize) < 0)
    {
        printf("Couldn't write initial control packet\n");
        llclose(TRANSMITTER);
        return -1;
    }
    
    printf("Wrote initial control packet\n");

    

    unsigned char data[MAX_PAYLOAD_SIZE]; 
    long int bytes;
    printf("----------------------------------------------------------------------- \n");
    int count = 0;
    while ( (bytes = fread(data , 1, sizeof(data), file)) > 0){
        long int dataPacketSize;
        unsigned char *packet = constructDataPacket(bytes, data, &dataPacketSize);
        printf("dataPacketSize %ld \n", dataPacketSize);

        for(int i = 0; i < 10; i++){
            //printf("Data packet[%d] = %x", i, packet[i]);
        }

        if(llwrite(packet, dataPacketSize) < 0){
            printf("Writing data packet fail \n");
            llclose(TRANSMITTER);
            return -1;
        }
        else {
            printf("Sent data packet %d \n", count);
            count++;
        }

        free(packet);
    }
    printf("----------------------------------------------------------------------- \n");

    // build END control packet
    unsigned char controlEnd[DATA_SIZE + 4];
    int controlEndSize = constructControlPacket(controlEnd, END_PACKET, filename, fileLength);

    
    printf("Sending ending control packet\n");
    if (llwrite(controlEnd, controlEndSize) < 0)
    {
        printf("Couldn't write ending control packet\n");
        llclose(TRANSMITTER);
        return -1;
    }
    

    if (fclose(file) != 0)
    {
        printf("Error closing file\n");
        llclose(TRANSMITTER);
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
        llclose(RECEIVER);
        return -1;
    }
    
    unsigned char control[MAX_DATA_SIZE];
    long int initialControlSize;

    
    printf("Reading initial control packet\n");
    if (llread(control, &initialControlSize) < 0)
    {
        printf("Error reading control packet");
        llclose(RECEIVER);
        return -1;
    }
    printf("Read the control packet\n");
    
    char fileName[MAX_BUF_SIZE];
    int fileLength = 0;

    // disassemble START control packet
    if (deconstructControlPacket(control, START_PACKET, fileName, &fileLength) < 0)
    {
        printf("Initial control packet wrong\n");
        llclose(RECEIVER);
        return -1;
    }
    else printf("Initial control packet correct\n");
    
    // open file to write
    FILE *file = fopen(filename, "wb");

    if (file == NULL)
    {
        printf("Couldn't open file\n");
        llclose(RECEIVER);
        return -1;
    }

    unsigned char dataPacket[MAX_PAYLOAD_SIZE];
    int dataSize = 0;
    int count = 1;
    long int packetSize;

    while (TRUE)
    {

        int r = llread(&dataPacket, &packetSize);

        for(int i = 0; i < 15; i++){
            //printf("dataPacket[%d] = %x  ", i, dataPacket[i]);
        }
        printf("\nPacketSize : %ld \n", packetSize);
        if (r < 0)
        {
            printf("Error reading info packet");
            break;
        }

        if(dataPacket[4] == 0x02){
            printf("Start packet \n");
        }


        // data to write to the file was read
        /*
        if (dataPacket[4] == DATA)
        {
            printf("Writing information to file\n");

            dataSize = dataPacket[2];
            fwrite(dataPacket + 3, 1, dataSize, file);
        }
        */

        // ending control packet was read
        else if (dataPacket[4] == END_PACKET){
            //printf("dataPacket[6] = %x  ", dataPacket[6]);
            printf("\nEnd control\n");
            break;
        }
        else if(r == 0){
            printf("Receiving data packet %d\n", count);
            count++;
            fwrite(&dataPacket[3], 1, packetSize-3, file);
        }   
    }

    printf("Ending control packet detected\n");

    if (fclose(file) != 0)
    {
        llclose(RECEIVER);
        printf("Error closing file\n");
        return -1;
    }
    printf("Closed the file\n");

    // close port
    if (llclose(RECEIVER) < 0)
    {
        printf("Couldn't close file\n");
        return -1;
    }

    printf("Port closed\n");

    return 0;
}
