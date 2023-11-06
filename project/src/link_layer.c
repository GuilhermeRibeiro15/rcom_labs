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

    char bcc2 = 0x00;
    printf("buf[0] = %02X\n", buf[0]);
    for (int i = 0; i < bufSize; i++) {
        printf("buf[%d] = %02X\n", i,buf[i]);
        bcc2 = bcc2 ^ buf[i];
    }

    printf("bcc2 write = %x\n", bcc2);

    int index = 4;

    printf("before the first for cicle \n");
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
    printf("After the first for cicle \n");
    printf("BufSize = %d\n", bufSize);
    
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

    printf("before the alarm cicle \n");
    while(alarmCount < tries && STOP == FALSE) {
        if(alarmEnabled == FALSE){
            printf("sending frame \n");
            alarm(maxtime);
            alarmEnabled = TRUE;
            write(fd_global, frame, index);
            printf("frame sent \n");
        }

        if(read(fd_global, v, 5) > 0){
            if(v[2] != (!tr << 7 | 0x05)){
                alarmEnabled = FALSE;
                printf("tr = %d\n", tr);
                printf("v[2] maybe = %u \n", (unsigned int) (tr << 7 | 0x01));
                printf("v[2] expected = %u \n", (unsigned int) (!tr << 7 | 0x05));
                printf("v[2] actual = %u \n", (unsigned int) v[2]);
                printf("First error\n");
            } 
            else if (v[3] != (v[1] ^ v[2])){
                alarmEnabled = FALSE;
                printf("Second error\n");
            } 
            else STOP = TRUE;
        }
    }
    printf("after the alarm cicle \n");

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

int llread(unsigned char *packet) {
    unsigned char single;
    StateMachine state = START;
    int size = 0;
    unsigned char infoFrame[MAX_PAYLOAD_SIZE * 2];

    printf("Before the first while\n");
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
    printf("After the first while\n");
    if(state == END) printf("state end, size = %d\n", size);

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
    else printf("No errors in infoFrame\n");

    int index = 0;
    for(int i = 0; i < size; i++){

        if(i == size - 1){
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
    printf("After the destuffing, index = %d\n", index);

    int data_control = 0;
    unsigned char bcc2 = 0x00;
    int packetSize = 0;
    if(packet[4] == 0x01){
        printf("data\n");
        data_control = 0;
        packetSize = 9 + (256*packet[6]) + packet[7];
        printf("packetsize = %d \n",packetSize);
    }    
    else{
        printf("control\n");\
        data_control = 1;
        packetSize += packet[6] + 7;
        packetSize += packet[packetSize + 2] + 4;

        packetSize = index - 1;
        printf("packetsize = %d, index = %d \n", packetSize, index);
    }

    for(int i = 4; i < packetSize -1; i++){
        printf("packet[%d] = %02X\n", i - 4,packet[i]);
        bcc2 ^= packet[i];
    }



    printf("packetsize = %d \n",index);
    printf("bbc2 read = %x\n", bcc2);
    printf("bcc2 in act packet = %x\n", packet[packetSize-1]);
    
    printf("before the packetSize == bcc2 if\n");
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
        printf("reached here \n");
        receiverFrame[2] = (!tr << 7) | 0x05;
        receiverFrame[3] = receiverFrame[1] ^ receiverFrame[2];
        write(fd_global, receiverFrame, 5); 
    }
    else{
        printf("packet[packetSize - 1] != bcc2\n");
        receiverFrame[2] = (tr << 7) | 0x01;
        receiverFrame[3] = receiverFrame[1] ^ receiverFrame[2];
        write(fd_global, receiverFrame, 5);
        printf("packet[packetSize - 1] != bcc2 2\n");
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

    printf("LLCLOSE\n");

    if(role == LlTx){
        printf("This is the transmitter \n");
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
        printf("This is the receiver \n");
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
