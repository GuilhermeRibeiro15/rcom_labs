// Link layer protocol implementation

#include "link_layer.h"

int alarmCount = 0;
int alarmEnabled = FALSE;
int max_time = 0;
int retries = 0;

#define _POSIX_SOURCE 1 

int llopen(LinkLayer connectionParameters) {
    StateMachine state;

    int fd = open(connectionParameters.serialPort, O_RDWR | O_NOCTTY);
    
    if (fd < 0){
        perror(serialPortName);
        return -1;
    }

    struct termios oldtio;
    struct termios newtio;

    if (tcgetattr(fd, &oldtio) == -1)
    {
        perror("tcgetattr");
        exit(-1);
    }

    memset(&newtio, 0, sizeof(newtio));

    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
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
    max_time = connectionParameters.timeout;

    if(connectionParameters.role == LlTx) {

        (void)signal(SIGALRM, alarmHandler);
        state = START;

        while(connectionParameters.nRetransmissions != 0 && state != END) {

            unsigned char buf[BUF_SIZE] = {FLAG, A, C, BCC, FLAG};
            write(fd, buf, 5);

            alarm(connectionParameters.timeout);
            alarmEnabled = TRUE;

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
        printf("could not pass the state machine");
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
        unsigned char buf1[BUF_SIZE] = {FLAG, A_UA, C_UA, BCC_UA, FLAG};
        write(fd, buf1, 5);
    }
    else return -1;

    return fd;
}

void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
    alarmCount++;
}

int llwrite(const unsigned char *buf, int bufSize)
{
    int f_size  = 6 + bufSize;
    unsigned char* frame = malloc(f_size);
    
    frame[0] = FLAG;
    frame[1] = A;
    frame[2] = C;
    frame[3] = BCC;
    



    return 0;
}

int llread(unsigned char *packet)
{
    // TODO

    return 0;
}

int llclose(int showStatistics)
{
    // TODO

    return 1;
}