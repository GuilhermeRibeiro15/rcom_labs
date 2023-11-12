// Link layer protocol implementation

#include "link_layer.h"

int fd_global = 0;
int alarmCount = 0;
int alarmEnabled = FALSE;
int tries = 0;
int maxtime = 0;
int tr = 0;
int previous = 1;

LinkLayerRole role;


#define _POSIX_SOURCE 1 

void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
    alarmCount++;
}

int llopen(LinkLayer connectionParameters) {
    StateMachine state;

    int fd = open(connectionParameters.serialPort, O_RDWR | O_NOCTTY);
    
    
    if (fd < 0){
        printf("reached here \n");
        return -1;
    }

    fd_global = fd;

    struct termios oldtio;
    struct termios newtio;

    if (tcgetattr(fd, &oldtio) == -1)
    {
        perror("tcgetattr");
        exit(-1);
    }

    memset(&newtio, 0, sizeof(newtio));

    newtio.c_cflag = connectionParameters.baudRate | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    // Set input mode (non-canonical, no echo,...)
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 30; // Inter-character timer unused
    newtio.c_cc[VMIN] = 0;

    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        return -1;
    }

    unsigned char single;
    tries = connectionParameters.nRetransmissions;
    maxtime = connectionParameters.timeout;
    role = connectionParameters.role;

    if(connectionParameters.role == LlTx) {

        (void)signal(SIGALRM, alarmHandler);
        state = START;

        while(connectionParameters.nRetransmissions != 0 && state != END) {

            unsigned char buf[BUF_SIZE] = {FLAG, A, C, BCC, FLAG};
            printf("Sendidng SET\n");
            int bytes = write(fd, buf, BUF_SIZE);

            if(bytes < 0) return -1;
            else printf("Sent successfully\n");

            alarm(connectionParameters.timeout);
            alarmEnabled = FALSE;

            while(alarmEnabled == FALSE && state != END){
                if(read(fd, &single, 1) != 0) {
                    switch(state){
                        case START:
                            if(single == FLAG) state = FLAG_T;
                            break;
                        case FLAG_T:
                            if(single == A_UA) state = A_T;
                            else if(single == FLAG) break;
                            else state = START;
                            break;
                        case A_T:
                            if(single == C_UA) state = C_T;
                            else if(single == FLAG) state = FLAG_T;
                            else state = START;
                            break;
                        case C_T:
                            if(single == BCC_UA) state = BCC_T;
                            else if(single == FLAG) state = FLAG_T;
                            else state = START;
                            break;  
                        case BCC_T:
                            if(single == FLAG) state = END; 
                            else state = START;
                            break;   
                        default:
                            break;                 
                    }
                }
            }
            connectionParameters.nRetransmissions -= 1;
        }
        
        if(state != END) return -1;
        else printf("Received UA\n");
    }
    else if(connectionParameters.role == LlRx){
        state = START;

        while(state != END){
            if(read(fd, &single, 1) != 0) {
                switch(state){
                    case START:
                        if(single == FLAG) state = FLAG_T;
                        break;
                    case FLAG_T:
                        if(single == A) state = A_T;
                        else if(single == FLAG) break;
                        else state = START;
                        break;
                    case A_T:
                        if(single == C) state = C_T;
                        else if(single == FLAG) state = FLAG_T;
                        else state = START;
                        break;
                    case C_T:
                        if(single == BCC) state = BCC_T;
                        else if(single == FLAG) state = FLAG_T;
                        else state = START;
                        break;  
                    case BCC_T:
                        if(single == FLAG) state = END; 
                        else state = START;
                        break;   
                    default:
                        break;                 
                }
            }
        }    

        if(state == END) printf("Received SET\n");

        unsigned char buf1[BUF_SIZE] = {FLAG, A_UA, C_UA, BCC_UA, FLAG};
        printf("Sending UA\n");
        int bytes = write(fd, buf1, BUF_SIZE);
        if(bytes < 0) return -1;
        else printf("Sent successfully\n");
    }
    else return -1;

    return fd;
}



int llwrite(const unsigned char *buf, int bufSize)
{
    int f_size  = 6 + bufSize;
    unsigned char* frame = malloc(f_size);
    
    frame[0] = FLAG;
    frame[1] = A;
    frame[2] = (tr << 6);
    frame[3] = (A ^ (tr << 6));

    printf("bufSize = %d\n", bufSize);

    char bcc2 = 0x00;
    for (int i = 0; i < bufSize; i++) {
        //printf("buf[%d] = %02X\n", i,buf[i]);
        bcc2 = bcc2 ^ buf[i];
    }
    printf("bcc2 = %x \n", bcc2);

    int index = 4;

    for(int i = 0; i < bufSize; i++){

        if(buf[i] == FLAG){
            frame[index] = 0x7D;
            index++;
            frame[index] = 0x5E;
            index++;
        }
        else if(buf[i] == 0x7D){
            frame[index] = 0x7D;
            index++;
            frame[index] = 0x5D;
            index++;
        }
        else{
            frame[index] = buf[i];
            index++;
        }
    }
    
    if (bcc2 == 0x7E) {
        frame[index] = 0x7D;
        index++;
        frame[index] = 0x5E;
        index++;
    }
    else if (bcc2 == 0x7D) {
        frame[index] = 0x7D;
        index++;
        frame[index] = 0x5D;
        index++;
    }
    else {
        frame[index] = bcc2;
        index++;
    }
    frame[index] = FLAG;
    index++;

    alarmCount = 0;
    int STOP = FALSE;
    alarmEnabled = FALSE;
    unsigned char v[5];

    while(alarmCount < tries && STOP == FALSE) {
        if(alarmEnabled == FALSE){
            alarm(maxtime);
            alarmEnabled = TRUE;
            write(fd_global, frame, index);
        }

        if(read(fd_global, v, 5) > 0){
            if(v[2] != (!tr << 7 | 0x05)){
                alarmEnabled = FALSE;
            } 
            else if (v[3] != (v[1] ^ v[2])){
                alarmEnabled = FALSE;
            } 
            else STOP = TRUE;
        }
    }

    if(STOP == FALSE){
        printf("alarm timed out \n");
        return -1;
    }

    previous = tr;
    if(tr) tr = 0;
    else tr = 1;

    free(frame);
    return 0;
}
/*
int llread(unsigned char *packet)
{
    int i = 0;
    unsigned char byte;
    StateMachine state = START ;
    unsigned char a, c;
    while(state != END){
        int read_bytes = read(fd_global, &byte, 1);
        if(read_bytes < 0){
            break;
        }
        switch(state){
            case START:
                if (byte == FLAG) state = FLAG_T;
                break;
            case FLAG_T:
                if (byte == 0x01) state = A_T;
                else if (byte != FLAG) state = FLAG_T;
                else state = START;
                break;
            case A_T:
                if(byte == 0x40 || byte == 0x00){
                    state = C_T;
                    c = byte;
                }
                else if (byte == FLAG) state = FLAG_T;
                else if (byte == 0x0B) {
                    unsigned char disc[5] = {FLAG, 0x01, 0x0B, 0x01 ^ 0x0B, FLAG};
                    if(write(fd_global, disc, 5) < 0){
                        printf("LLREAD- Failed to send DISC");
                        return 1;
                    }
                    else{
                        printf("LLREAD- DISC sent successfully!\n");
                        return 0;
                    }
                }
                else state = START;
                break;
            case C_T:
                if(byte == (0x01 ^ c)){
                    state = DATA_T;
                    unsigned char RR;
                    if(tr == previous){
                        if(tr == 0) RR = 0x05;
                        else RR = 0x85;
                        unsigned char resp[5]  = {FLAG, 0x01, RR, 0x01 ^ RR, FLAG};
                        if(write(fd_global, resp, 5) < 0){
                            printf("LLREAD- Failed to send RR");
                            return 1;
                        }
                        else{
                            printf("LLREAD- RR sent successfully!\n");
                            return 0;
                        }
                    }
                }
                else if(byte == FLAG) state = FLAG_T;
                else state = START;
                break;
            case DATA_T:
                if(byte == ESC_T) state = ESC_T;
                else if(byte == FLAG){
                    unsigned char BCC2 = packet[i-1];
                    i--;
                    unsigned char acc = packet[0];

                    for (unsigned int j = 1; j < i; j++){
                        acc ^= packet[j];
                    }

                    if(BCC2 == acc){
                        state = END;
                        unsigned char RR;
                        if(tr == 0) RR = 0x05;
                        else RR = 0x85;
                        unsigned char rr[5] = {FLAG,0x01,RR, 0x01^RR, FLAG};
                        if(write(fd_global, rr,5)<0){
                            printf("LLREAD- Failed to send RR");
                            return -1;
                        }
                        else{
                            previous = tr;
                            tr = (tr + 1) % 2;
                            printf("LLREAD- RR sent successfully!\n");
                            return i;
                        }

                    }
                    else{
                        unsigned char rej;
                        if (tr == 0) rej = 0x01;
                        else rej = 0x81;
                        unsigned char REJ[5] = {FLAG, 0x01, REJ, 0x01 ^ rej, FLAG};
                        if (write(fd_global, REJ, 5) < 0){
                            printf("LLREAD- Failed to send REJ");
                            return -1;
                        }
                        else{
                            printf("LLREAD- REJ sent successfully!\n");
                            return -1;
                        }
                        return -1;
                    }
                }
                else {
                    packet[i++] = byte;
                }
                break;
            case ESC_T:
                state = DATA_T;
                if (byte == (ESC_T^0x20)) packet[i++]=ESC_T;
                else if(byte == (FLAG^0x20)) packet[i++]=FLAG;
                break;
            default:
                break;
        }
    }
    return -1;
}
*/

int llread(unsigned char *packet, long int* p_size) {
    unsigned char single;
    StateMachine state = START;
    int size = 0;
    unsigned char infoFrame[MAX_PAYLOAD_SIZE * 2];

    while(state != END){
        if(read(fd_global, &single, 1) > 0){
            switch(state){
                case START:
                    if(single == FLAG){
                        state = FLAG_T;
                        infoFrame[size] = single;
                        size++;
                    }    
                    break;
                case FLAG_T:
                    if(single == FLAG){
                        state = FLAG_T;
                    }    
                    else {
                        state = A_T;
                        infoFrame[size] = single;
                        size++;
                    }
                    break;
                case A_T:
                    if(single == FLAG){
                        state = END;
                        infoFrame[size] = single;
                        size++;
                    }
                    else{
                        infoFrame[size] = single;
                        size++;
                    }
                    break;
                default:
                    break;                 
            }
        }
    }

    unsigned char receiverFrame[5];
    receiverFrame[0] = FLAG;
    receiverFrame[1] = A;
    receiverFrame[4] = FLAG;


    if(infoFrame[2] != (tr << 6)){
        printf("infoFrame[2] != (tr << 6) \n");
        receiverFrame[2] = (tr << 7) | 0x01;
        receiverFrame[3] = A ^ receiverFrame[2];
        write(fd_global, receiverFrame, 5);

        return -1;
    }
    else if(infoFrame[3] != (A ^ infoFrame[2])){
        printf("infoFrame[3] != (A ^ infoFrame[2]) \n");
        receiverFrame[2] = (tr << 7) | 0x01;
        receiverFrame[3] = A ^ receiverFrame[2];
        write(fd_global, receiverFrame, 5);

        return -1;
    }

    int index = 0;
    for(int i = 0; i < size; i++){

        if(infoFrame[i] == 0x7D && infoFrame[i+1] == 0x5e){
            i++;
            packet[index] = FLAG;
            index++;
        }
        else if(infoFrame[i] == 0x7D && infoFrame[i+1] == 0x5d){
            i++;
            packet[index] = 0x7d;
            index++;
        }
        else{
            packet[index] = infoFrame[i];
            index++;
        }
        
    }

    int data_control = 0;
    unsigned char bcc2 = 0x00;
    int packetSize = 0;

    if(packet[4] == 0x01){
        printf("data\n");
        data_control = 0;
        
        for(int i = 4; i < index - 2; i++){
            bcc2 ^= packet[i];
        }

        packetSize = index - 1;
    }    
    else{
        printf("control\n");
        data_control = 1;

        packetSize = index - 1;

        for(int i = 4; i < index - 2; i++){
            bcc2 ^= packet[i];
        }
    }

    //printf("bcc2 = %x, packetSize = %x", bcc2, packet[packetSize -1]);

    if(packet[packetSize - 1] == bcc2){
        if(!data_control){
            if(infoFrame[5] == previous){
                printf("duplicate frame \n");
                receiverFrame[2] = (!tr << 7) | 0x05;
                receiverFrame[3] = receiverFrame[1] ^ receiverFrame[2];
                write(fd_global, receiverFrame, 5);
                if(tr) tr = 0;
                else tr = 1;
                return -1;
            }
            else previous = infoFrame[5];
        }
        //printf("control frame\n");
        receiverFrame[2] = (!tr << 7) | 0x05;
        receiverFrame[3] = receiverFrame[1] ^ receiverFrame[2];
        write(fd_global, receiverFrame, 5); 
    }
    else{
        printf("error in bcc2\n");
        receiverFrame[2] = (tr << 7) | 0x01;
        receiverFrame[3] = receiverFrame[1] ^ receiverFrame[2];
        write(fd_global, receiverFrame, 5);
        return -1;
    }

    previous = tr;
    if(tr) tr = 0;
    else tr = 1;
    *p_size = packetSize;
    return 0;
}

int llclose(int showStatistics) {
    StateMachine state;

    unsigned char single;

    printf("LLCLOSE\n");

    if(role == LlTx){
        printf("This is the transmitter \n");
        state = START;
        signal(SIGALRM, alarmHandler);

        printf("Before the first while \n");
        while(tries != 0 && state != END) {
            unsigned char buf[BUF_SIZE] = {FLAG, A, C_DISC, BCC_DISC, FLAG};

            printf("Before writing DISC \n");

            printf("showStatistics = %d", fd_global);
            write(fd_global, buf, BUF_SIZE);
            printf("Wrote DISC \n");

            alarm(maxtime);
            alarmEnabled = FALSE;

            printf("Before state machine \n");
            while(alarmEnabled == FALSE && state != END){
                if(read(fd_global, &single, 1) != 0) {
                    switch(state){
                        case START:
                            if(single == FLAG) state = FLAG_T;
                            break;
                        case FLAG_T:
                            if(single == A) state = A_T;
                            else if(single == FLAG) break;
                            else state = START;
                            break;
                        case A_T:
                            if(single == C_DISC) state = C_T;
                            else if(single == FLAG) state = FLAG_T;
                            else state = START;
                            break;
                        case C_T:
                            if(single == BCC_DISC) state = BCC_T;
                            else if(single == FLAG) state = FLAG_T;
                            else state = START;
                            break;  
                        case BCC_T:
                            if(single == FLAG) state = END; 
                            else state = START;
                            break;   
                        default:
                            break;                 
                    }
                }
            }
            tries -= 1;

            printf("after the state machine \n");
        }
        printf("End of llclose \n");
    }
    else if(role == LlRx){
        printf("This is the receiver \n");
        state = START;

        printf("Before the first while \n");

        while(alarmEnabled == FALSE && state != END){
            if(read(fd_global, &single, 1) != 0) {
                switch(state){
                    case START:
                        if(single == FLAG) state = FLAG_T;
                        break;
                    case FLAG_T:
                        if(single == A) state = A_T;
                        else if(single == FLAG) break;
                        else state = START;
                        break;
                    case A_T:
                        if(single == C_DISC) state = C_T;
                        else if(single == FLAG) state = FLAG_T;
                        else state = START;
                        break;
                    case C_T:
                        if(single == BCC_DISC) state = BCC_T;
                        else if(single == FLAG) state = FLAG_T;
                        else state = START;
                        break;  
                    case BCC_T:
                        if(single == FLAG) state = END; 
                        else state = START;
                        break;   
                    default:
                        break;                 
                }
            }
        }
        printf("After the first while \n");
        unsigned char buf[BUF_SIZE] = {FLAG, A, C_DISC, BCC_DISC, FLAG};
        write(fd_global, buf, BUF_SIZE);
        printf("Wrote DISC \n");
    }
    else return -1;

    if(close(fd_global) < 0) return -1;
    
    return 1;
}
