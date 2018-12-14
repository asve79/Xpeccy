#ifndef RS232_H
#define RS232_H

#include <termios.h> /* POSIX Terminal Control Definitions */

typedef struct {
    struct termios old_stdio;
    int    mode;   //0 - serial port emulation
    int    tty_fd; //port descriptor
} xRs232;

xRs232* rs232Create();
void rs232Destroy(xRs232*);

struct termios rs232_savestdio();
int  rs232_open(char *portname, unsigned int speed);
void rs232_close(int tty_fd);
void rs232_rollback(struct termios* old_stdio);
long rs232_write(int fd, unsigned char ch);
int  rs232_havein(int fd);
unsigned char rs232_canwrite(int fd);

#endif // RS232_H
