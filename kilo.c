/*** includes ***/

#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <sys/ioctl.h>

/*** defines ***/
#define CTRL_KEY(k) ((k) & 0x1f)

/*** data ***/

struct editorConfig{
    int screenrows;
    int screencols;
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

int getCursorPosition(int *rows, int *cols){
    char buff[32];
    unsigned int i = 0;

    if(write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;

    while(i < sizeof(buff)-1){
        if(read(STDIN_FILENO, &buff[i], 1) != 1)
            break;
        if(buff[i] == 'R')
            break;
        i++; 
    }

    buff[i] = '\0';

    //printf("\r\n&buff[1]: '%s'\r\n", &buff[1]);
 
    if(buff[0] != '\x1b' || buff[1] != '[') return -1;
    if(sscanf(&buff[2], "%d;%d", rows, cols) != 2) return -2;

    return 0;
}

int getWindowSize(int *rows, int *columns){
    struct winsize ws;

    if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0){
        if(write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
        return getCursorPosition(rows, columns);
    }else{
        *rows = ws.ws_row;
        *columns = ws.ws_col;
        return 0;
    }
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
    for(int i = 0; i < E.screenrows; i++){
        write(STDOUT_FILENO, "~", 1);
        if(i < E.screenrows - 1){
            write(STDOUT_FILENO, "\r\n", 2);
        }
        
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

void initEditor(){
    if(getWindowSize(&E.screenrows, &E.screencols) == -1) die("getWindowSize");
}

int main(){

    enablerawMode();
    initEditor();

    while(1){
        editorRefreshScreen();
        editorProcessKeypress();
    }
 
    return 1;
}