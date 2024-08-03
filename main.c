#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>


struct termios orig_termios;

void fail(const char* s) {
    perror(s);
    exit(1);
}

void disableRawMode(void) {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1)
	fail("tcsetattr");
}

void enableRawMode(void) {
    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1)
	fail("tcgetattr");
    atexit(disableRawMode);

    struct termios raw = orig_termios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
	fail("tcsetattr");
}

void setCursor(int x, int y) {
    char* str;
    int len = asprintf(&str, "\x1b[%d;%dH", y + 1, x + 1);
    if (len == -1)
	fail("asprintf");
    if (write(STDOUT_FILENO, str, len) == -1)
	fail("write");
    free(str);
}

void clearScreen(void) {
    if (write(STDOUT_FILENO, "\x1b[2J", 4) == -1)
	fail("write");
    setCursor(0, 0);
}

int main(void) {
    enableRawMode();

    char c;
    int x = 0;
    int y = 0;

    clearScreen();
    while (1) {
	clearScreen();
	setCursor(x, y);
	if (read(STDIN_FILENO, &c, 1) == -1 && errno != EAGAIN)
	    fail("read");
	if (c == 'q') break;
	else if (c == 'h' && x > 0) x--;	
	else if (c == 'l') x++;	
	else if (c == 'k' && y > 0) y--;	
	else if (c == 'j') y++;	
    }
    return 0;
} 

