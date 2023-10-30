// Link layer protocol implementation

#include "link_layer.h"

int fd_global;
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
    
    char bcc2 = 0x00;
    for(int i = 0; i < BUF_SIZE; i++) bcc2 = bcc2 ^ buf[i];

    int index = 4;

    for(int i = 0; i < BUF_SIZE; i++){
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

    frame[index] = bcc2;
    index++;
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
            printf("frame sent");
        }

        if(read(fd_global, v, 5) > 0){
            if(v[2] != (!tr << 7 | 0x05)) alarmEnabled = FALSE;
            else if (v[3] != (v[1] ^ v[2])) alarmEnabled = FALSE;
            else STOP = TRUE;
        }
    }

    if(STOP == FALSE){
        printf("alarm timed out");
        return -1;
    }

    previous = tr;
    if(tr) tr = 0;
    else tr = 1;

    free(frame);
    return 0;
}

int llread(unsigned char *packet) {
    unsigned char single, c;
    StateMachine state = START;
    int size = 0;
    unsigned char infoFrame[999];

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
                    if(single == A){
                        state = A_T;
                        infoFrame[size] = single,
                        size++;
                    }    
                    else if(single == FLAG) state = FLAG_T;
                    else state = START;
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
        receiverFrame[2] = (tr << 7) | 0x01;
        receiverFrame[3] = A ^ receiverFrame[2];
        write(fd_global, receiverFrame, 5);

        return -1;
    }
    else if(infoFrame[3] != (A ^ infoFrame[2])){
        receiverFrame[2] = (tr << 7) | 0x01;
        receiverFrame[3] = A ^ receiverFrame[2];
        write(fd_global, receiverFrame, 5);

        return -1;
    }

    int index = 0;
    for(int i = 0; i < size; i++){

        if(i = size - 1){
            packet[index] = infoFrame[i];
            index++;
            break;
        }


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
        data_control = 0;
        packetSize = 9 + (256*packet[6]) + packet[7];
    }    
    else{
        data_control = 1;
        packetSize += packet[6] + 7;
        packetSize += packet[size + 2] + 4;
    }

    int checkbcc2 = 4;
    while(checkbcc2 < packetSize - 1){
        bcc2 = bcc2 ^ packet[checkbcc2];
        checkbcc2++;
    }
    
    if(packet[packetSize - 1] == bcc2){
        if(!data_control){
            if(infoFrame[5] == previous){
                printf("duplicate frame");
                receiverFrame[2] = (!tr << 7) | 0x05;
                receiverFrame[3] = receiverFrame[1] ^ receiverFrame[2];
                write(fd_global, receiverFrame, 5);
                if(tr) tr = 0;
                else tr = 1;
                return -1;
            }
            else previous = infoFrame[5];
        }

        receiverFrame[2] = (!tr << 7) | 0x05;
        receiverFrame[3] = receiverFrame[1] ^ receiverFrame[2];
        write(fd_global, receiverFrame, 5); 
    }
    else{
        receiverFrame[2] = (tr << 7) | 0x01;
        receiverFrame[3] = receiverFrame[1] ^ receiverFrame[2];
        write(fd_global, receiverFrame, 5);

        return -1;
    }


    previous = tr;
    if(tr) tr = 0;
    else tr = 1;
    return 0;
}

int llclose(int showStatistics) {
    StateMachine state;

    unsigned char single;

    if(role == LlTx){
        state = START;
        (void)signal(SIGALRM, alarmHandler);

        while(tries != 0 && state != END) {

            unsigned char buf[BUF_SIZE] = {FLAG, A, C_DISC, BCC_DISC, FLAG};
            write(showStatistics, buf, BUF_SIZE);

            alarm(maxtime);
            alarmEnabled = FALSE;

            while(alarmEnabled == FALSE && state != END){
                if(read(showStatistics, &single, 1) != 0) {
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
        }
    }
    else if(role == LlRx){
        state = START;

        while(alarmEnabled == FALSE && state != END){
            if(read(showStatistics, &single, 1) != 0) {
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
        unsigned char buf[BUF_SIZE] = {FLAG, A, C_DISC, BCC_DISC, FLAG};
        write(showStatistics, buf, BUF_SIZE);
    }
    else return -1;

    if(close(showStatistics) < 0) return -1;
    
    return 1;
}
