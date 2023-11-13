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
        printf("%s", filename);            
        enviaFile(filename);
    }
    else if (strcmp(role, "rx") == 0){
       linklayer.role = LlRx;
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

    if (llopen(linklayer) < 0)
    {
        printf("Couldn't establish connection with receiver\n");
        llclose(TRANSMITTER);
        return -1;
    }

    fseek(file, 0L, SEEK_END);
    long int fileLength = ftell(file);
    fseek(file, 0L, SEEK_SET);

    unsigned char controlStart[MAX_BUF_SIZE];
    int controlStartSize = constructControlPacket(controlStart, START_PACKET, filename, fileLength);

    if (llwrite(controlStart, controlStartSize) < 0)
    {
        printf("\nCouldn't write initial control packet\n");
        llclose(TRANSMITTER);
        return -1;
    }
    
    printf("Wrote initial control packet\n");

    unsigned char data[MAX_PAYLOAD_SIZE]; 
    long int bytes;
    int count = 0;
    while ( (bytes = fread(data , 1, sizeof(data), file)) > 0){
        long int dataPacketSize;

        unsigned char *packet = constructDataPacket(bytes, data, &dataPacketSize);
        printf("dataPacketSize %ld \n", dataPacketSize);

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

    // build END control packet
    unsigned char controlEnd[DATA_SIZE + 4];
    int controlEndSize = constructControlPacket(controlEnd, END_PACKET, filename, fileLength);

    if (llwrite(controlEnd, controlEndSize) < 0)
    {
        printf("Couldn't write ending control packet\n");
        llclose(TRANSMITTER);
        return -1;
    }
    
    printf("Sent ending control packet\n");

    if (fclose(file) != 0)
    {
        printf("Error closing file\n");
        llclose(TRANSMITTER);
        return -1;
    }

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
    if (llopen(linklayer) < 0)
    {
        printf("Couldn't establish connection with transmitter\n");
        llclose(RECEIVER);
        return -1;
    }
    
    unsigned char control[MAX_DATA_SIZE];
    long int initialControlSize;

    if (llread(control, &initialControlSize) < 0)
    {
        printf("Error reading control packet");
        llclose(RECEIVER);
        return -1;
    }
    printf("\nRead the initial control packet\n");
    
    char fileName[MAX_BUF_SIZE];
    int fileLength = 0;

    if (deconstructControlPacket(control, START_PACKET, fileName, &fileLength) < 0)
    {
        printf("Initial control packet wrong\n");
        llclose(RECEIVER);
        return -1;
    }
    else printf("Initial control packet correct\n");
    
    FILE *file = fopen(filename, "wb");

    if (file == NULL)
    {
        printf("Couldn't open file\n");
        llclose(RECEIVER);
        return -1;
    }

    unsigned char dataPacket[MAX_PAYLOAD_SIZE*2];
    int count = 1;
    long int packetSize;

    while (TRUE)
    {

        int r = llread(&dataPacket, &packetSize);

        printf("\nPacketSize : %ld \n", packetSize);
        if (r < 0)
        {
            printf("Error reading info packet");
            break;
        }

        if(dataPacket[4] == 0x02){
            printf("Start packet \n");
        }

        else if (dataPacket[4] == END_PACKET){
            printf("\nEnd control\n");
            break;
        }
        else if(r == 0){
            printf("Receiving data packet %d\n", count);
            count++;

            unsigned char final_data_packet[MAX_PAYLOAD_SIZE*2];
            size_t count = 0;
            for(int i = 7; i < packetSize - 1; i++){
                count++;
                final_data_packet[i - 7] = dataPacket[i];
            }

            fwrite(final_data_packet, sizeof(unsigned char), count, file);
        }   
    }

    if (fclose(file) != 0)
    {
        llclose(RECEIVER);
        printf("Error closing file\n");
        return -1;
    }

    if (llclose(RECEIVER) < 0)
    {
        printf("Couldn't close file\n");
        return -1;
    }

    return 0;
}
