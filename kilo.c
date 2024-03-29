#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>

struct termios orig_termios;

void disableRawMode(){
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enablerawMode(){

    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disableRawMode);

    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON); //reading input byte-by-byte instead of line-by-line

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int main(){

    enablerawMode();

    char c;
    while(read(STDIN_FILENO, &c, 1) == 1 && c != 'q'){
        if(iscntrl(c)){
            printf("%d\n", c);
        }else{
            printf("%d ('%c')\n", c, c);
        }
    }

    return 1;
}