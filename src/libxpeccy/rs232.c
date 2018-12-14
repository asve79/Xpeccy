//#include <QtSerialPort/QSerialPort>
#include <fcntl.h>   /* File Control Definitions           */
#include <termios.h> /* POSIX Terminal Control Definitions */
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>

#include "rs232.h"

xRs232* rs232Create() {
    xRs232* rs = (xRs232*)malloc(sizeof(xRs232));
    memset(rs,0x00,sizeof(xRs232));
    rs->tty_fd = -1;    //default volue
    if (!rs) return NULL;
    rs->old_stdio=rs232_savestdio();
    rs->mode = 0; //serial port connection is default
    return rs;
}

void rs232Destroy(xRs232* rs) {
//    if (rs->tty_fd != -1){
//        rs232_close(rs->tty_fd);
//        rs232_rollback(&rs->old_stdio);
//    }
    free(rs);
}

struct termios rs232_savestdio(){
    struct termios old_stdio;
    tcgetattr(STDOUT_FILENO,&old_stdio);
    return old_stdio;
}

int rs232_open(char *portname, unsigned int speed){
    struct termios tio;
    struct termios stdio;
    int tty_fd;
    speed_t cspeed;

    if (speed > 7) speed=7;

    switch (speed) {
        case 0:
            cspeed = B1200;
        case 1:
            cspeed = B2400;
        case 2:
            cspeed = B4800;
        case 3:
            cspeed = B9600;
        case 4:
            cspeed = B19200;
        case 5:
            cspeed = B38400;
        case 6:
            cspeed = B57600;
        case 7:
            cspeed = B115200;
        default:
            cspeed = B115200;
    }

    char *speeds[] = {"1200","2400","4800","9600","19200","38400","57600","115200"};

    unsigned char c='D';

    memset(&stdio,0,sizeof(stdio));
    stdio.c_iflag=0;
    stdio.c_oflag=0;
    stdio.c_cflag=0;
    stdio.c_lflag=0;
    stdio.c_cc[VMIN]=1;
    stdio.c_cc[VTIME]=0;
    tcsetattr(STDOUT_FILENO,TCSANOW,&stdio);
    tcsetattr(STDOUT_FILENO,TCSAFLUSH,&stdio);
    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);       // make the reads non-blocking

    memset(&tio,0,sizeof(tio));
    tio.c_iflag=0;
    tio.c_oflag=0;
    tio.c_cflag=CS8|CREAD|CLOCAL;           // 8n1, see termios.h for more information
    tio.c_lflag=0;
    tio.c_cc[VMIN]=1;
    tio.c_cc[VTIME]=5;

    tty_fd=open(portname, O_RDWR | O_NONBLOCK);
    if (tty_fd != -1){
        cfsetospeed(&tio,cspeed);            // cspeed baud
        cfsetispeed(&tio,cspeed);            // cspeed baud

        tcsetattr(tty_fd,TCSANOW,&tio);

        //int a = write(tty_fd,"i",1);
        //printf("sended %d",a);

        printf("Serial port %s opened on speed %s descriptor (%d)\n",portname,speeds[speed],tty_fd);
    } else {
        printf("Serial port '%s' is not open. Error %d\n",portname,errno);
    }
    return tty_fd;
}


void rs232_rollback(struct termios* old_stdio){
    printf("rs232. repair configuration\n");
    tcsetattr(STDOUT_FILENO,TCSANOW, old_stdio);
}

void rs232_close(int tty_fd){
    if (tty_fd >= 0){
        printf("rs232. Close port (%d)\n",tty_fd);
        close(tty_fd);
    } else {
        printf("rs232: notring to close.");
    }
}

long rs232_write(int fd, unsigned char ch){

    if (fd == -1){
        printf("no rs232 port assigned\n");
        return(-1);
    }

    long bytes_written = write(fd,&ch,1);/* use write() to send data to port                                            */
                                     /* "fd"                   - file descriptor pointing to the opened serial port */
                                     /*	"write_buffer"         - address of the buffer containing data	            */
                                     /* "sizeof(write_buffer)" - No of bytes to write                               */
//    printf("\n  \\%02hhx send to descr %d, written %d bytes\n", ch, fd, bytes_written);

    if ((bytes_written == -1)||(errno != 0)){
        printf("error no %d, descriptor %d\n",errno, fd);
    }
    return bytes_written;
}

int rs232_havein(int fd){
    int ret;
    unsigned char val;
    val=0;
    if (ioctl( fd, FIONREAD, &ret)< 0) {
       perror("rs232_havein: ioctl error (%d)",fd);
    }
    return ret;
}

unsigned char rs232_canwrite(int fd){
    if ( fd == -1 ){
        return 0;
    } else {
        return 1;
    }
}
