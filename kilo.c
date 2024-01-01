/* includes */
#include <asm-generic/errno-base.h>
#include <asm-generic/ioctls.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h> // Io control/ to get current terminal size
#include <termios.h> //terminal behaviour modifier
#include <unistd.h>

/* defines */
#define CTRL_KEY(k) ((k) & 0x1f) //register ctrl key with whatever key was pressed
#define KILO_VERSION "0.0.1"
/* func declarations, not in order*/
int editorReadKey();
void editorMoveCursor(int key);

/* misc */

char buffer[32];



/* data */
struct editorConf { 
  int cx, cy;
  int screenrows;
  int screencols;
  struct termios org_term; // original terminal state
};

struct editorConf E;

/* Append buffr */

struct abuf{
  char* b;
  int len;
};
#define ABUF_INIT {NULL, 0};

enum editorKey{
  ARROW_LEFT = 1000,
  ARROW_RIGHT,
  ARROW_UP,
  ARROW_DOWN
};



void abFree(struct abuf *ab){
  free(ab->b);
}

void abAppend(struct abuf *ab, const char*s, int len){

  char* new = realloc(ab->b, ab->len + len);
  if(new == NULL) return;
  memcpy(&new[ab->len], s, len); //copy mem(s) from ab-> to the beginning of the new bffr, and then pass the len of s
  ab->b = new; // since new returns the same ptr as b it is safe to reallocate.
  ab->len+=len;
}

/* terminal */

void die(const char* s){ //error handling func
  //refreshes screen upon error, so there's no leftover code in the screen
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3); //positions cursor in the first row of our screen

  perror(s); //useful debugger exit code, prints error msg
  exit(1);
}

void disableRawMode() { 
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.org_term); 
  die("tcsetattr");
}

void enableRawMode() { 
  if(tcgetattr(STDIN_FILENO, &E.org_term) == -1) // read term attributes into struct
    die("tcgetattr");

  atexit(disableRawMode);

  struct termios raw = E.org_term;
  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP |
                   IXON);  // stop term from freezing when pressing Ctrl-S
  raw.c_oflag &= ~(OPOST); // turn off carriage return translation when \n.
                           // which is default in the term
  raw.c_cflag |= (CS8);    // miscellaneous flag
  raw.c_lflag &=
      ~(ECHO | ICANON | IEXTEN | ISIG); // modifying the struct by hand
  // and shift bits in flag to disable echoing && turning off CANNON mode.
  // canon mode allows the program to read one bit at the time instead of
  // reading them line by line, eg: we no longer have to press q and enter
  // to exit, instead just by pressing q we exit the program as stated in
  // main.
  // ISIG disables the behaviour of the exiting the program upon pressing
  // Ctrl-C or Ctrl-z

  raw.c_cc[VMIN] =
      0; // minimum num of bytes before read() can return, set to the min
  raw.c_cc[VTIME] = 1; // time to wait before read() returns.

  if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw)) // passing the modified struct to set our changes.
    die("tctsetattr");

  assert(raw.c_lflag != E.org_term.c_lflag);
}

int getCursorPosition(int* rows, int* cols){
  char buf[32];
  unsigned int i = 0;

  if(write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1; // writing bytes to get cursor position query
  //
  while(i < sizeof(buf) - 1){ //query usually terminates with the format : ESC [ Pn ; Pn R.
    if(read(STDIN_FILENO, &buf[i],1) != 1) break;
    if(buf[i] == 'R') break;
    i++;
  }
  buf[i] = '\0';

  if(buf[0] != '\x1b' || buf[1] != '[') return -1;
  if(sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1; //reading cursor position to arguments.
  return 0;

}

int getWindowSize(int* rows, int* cols){

  struct winsize ws;

  // getting window(term) size value
  // if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0)
  //   return -1;
  // else{
  //   *cols = ws.ws_col;
  //   *rows = ws.ws_row;
  //   return 0;
  // }

  //get size by placing cursor on the buttom right of the screen, so we get the max
  //rows 

  if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0){
    if(write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
    editorReadKey();
    return getCursorPosition(rows, cols);
  } else{
    *cols = ws.ws_col;
    *rows = ws.ws_row;
    return 0;
  }
}

void editorProcessKeypress(){
  int c = editorReadKey();

  switch(c){
    case CTRL_KEY('q'):
      write(STDOUT_FILENO, "\x1b[2J", 4);
      write(STDOUT_FILENO, "\x1b[H", 3);
      exit(0);
      break;

    //TODO: Process arrow keys

    case ARROW_UP:
    case ARROW_DOWN:
    case ARROW_LEFT:
    case ARROW_RIGHT:
    editorMoveCursor(c);
    break;
  }

}

/* output */

void drawRows(struct abuf *ab){
  int y;
  for (y = 0; y < E.screenrows; y++) {


    if(y == 1){ //will show message in the third of the screen
      char welcome[80];
      int welcomelen = snprintf(welcome, sizeof(welcome),
                                "Kilo editor --version %s", KILO_VERSION);


      //need to study this logically
      if(welcomelen > E.screencols) welcomelen = E.screencols; //truncate str len to terminal width
      int padding = (E.screencols - welcomelen) / 2;

      if(padding){
        abAppend(ab, "~", 1);
        padding--;
      }
      while (padding--)  abAppend(ab, " ", 1);
      
      abAppend(ab, welcome , welcomelen);

    }else{
      abAppend(ab , "~", 1);
    }



    abAppend(ab, "\x1b[K", 3); //esc sequence to clear cursor to end of  current line, more optimal than clearing the entire screen
    if(y < E.screenrows){
      abAppend(ab, "\r\n", 2);
    }
  }


}


void editorRefreshScreen(){ //write 'esc' to the screen

  struct abuf ab = ABUF_INIT;

  abAppend(&ab, "\x1b[?25l", 6); //hides cursor to prevent flicker
  // abAppend(&ab, "\x1b[2J", 4); 
  abAppend(&ab, "\x1b[H", 3); //positions cursor in the first row of our screen
  drawRows(&ab);

  char buf[32]; //buff to append to
  snprintf(buf, sizeof(buf), "\x1b[%d;%dH", E.cy + 1, E.cx + 1); //term is indexed starting at 1
  abAppend(&ab , buf, strlen(buf));


  // abAppend(&ab, "\x1b[H", 3); //positions cursor in the first row of our screen
  abAppend(&ab, "\x1b[?25h", 6); //unhides cursor to prevent flicker

  write(STDOUT_FILENO, ab.b, ab.len);
  abFree(&ab);
  
  
}

/* input */
void editorMoveCursor(int key){

  switch (key) {
    case ARROW_LEFT:
    E.cx--;
    break;

    case ARROW_RIGHT:
    E.cx++;
    break;

    case ARROW_UP:
    E.cy--;
    break;

    case ARROW_DOWN:
    E.cy++;
    break;
  }

}

int editorReadKey(){

  int nread = 0; //tracks if read() registered a key succesfully
  char c; //byte we're reading to

  while((nread == read(STDIN_FILENO, &c, 1) != 1)){
    if(nread == 1 && errno != EAGAIN)
      die("read");
  }

  //reading arrow sequences:
  if(c == '\x1b'){ 
    char seq[3];

    if(read(STDIN_FILENO, &seq[0], 1) != 1)return '\x1b';
    if(read(STDIN_FILENO, &seq[1], 1) != 1)return '\x1b';

    if(seq[0] == '['){
      switch(seq[1]){
        case 'A': return ARROW_UP;
        case 'B': return ARROW_DOWN;
        case 'C': return ARROW_RIGHT;
        case 'D': return ARROW_LEFT;
      }
    }

    return '\x1b';
  }else{
    return c;
  }


}


/* init */

void initEditor(){

  E.cx = 0;
  E.cy = 0;
  
  if(getWindowSize(&E.screenrows, &E.screencols) == -1)
    die("getWindowSize");
}

int main() {

  enableRawMode();
  initEditor(); //its function for now is to get curr term window size

  while (1) {

    editorRefreshScreen();
    editorProcessKeypress();
  }

  return 0;
}
