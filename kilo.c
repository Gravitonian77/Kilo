/*** includes ***/

#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>

/*** defines ***/
#define CTRL_KEY(k) ((k) & 0x1f)

/*** data ***/

struct editorConfig{
    struct termios orig_termios;
};

struct editorConfig E;
/*** terminal ***/

void editorRefreshScreen();
void editorDrawRows();

void die(const char *s) {
  editorRefreshScreen();
  perror(s);
  exit(1);
}

void disableRawMode(){
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
        die("tcsetattr");
}

void enablerawMode(){

    if(tcgetattr(STDIN_FILENO, &E.orig_termios))
        die ("tcgetattr");

    atexit(disableRawMode);

    struct termios raw = E.orig_termios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN); //reading input byte-by-byte instead of line-by-line
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}

char editorReadKey(){
    int nread;
    char c;

    while((nread = read(STDIN_FILENO, &c, 1)) != 1){
        if(nread == -1 && errno != EAGAIN)
            die("read");
    }

    return c;
}

/*** output ***/

void editorRefreshScreen() {
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);    //Repositions the cursor to the top-left corner
                                        //Dont ask me how, idk.
  editorDrawRows();

  write(STDOUT_FILENO, "\x1b[H", 3);
}

void editorDrawRows(){
    for(int i = 0; i < 24; i++){
        write(STDOUT_FILENO, "~\r\n", 3);
    }
}

/*** input ***/

void editorProcessKeypress(){
    char c = editorReadKey();

    switch(c){
        case CTRL_KEY('q'):
            editorRefreshScreen();
            exit(0);
            break;
    }
}

/*** init ***/

int main(){

    enablerawMode();
    
    while(1){
        editorRefreshScreen();
        editorProcessKeypress();
    }
 
    return 1;
}