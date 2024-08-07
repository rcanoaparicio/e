#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

struct EditorConfig {
  struct termios orig_termios;
  int screen_rows;
  int screen_cols;
};

struct Cursor {
  int x;
  int y;
};

enum Mode { NORMAL, INSERT };

struct EditorConfig editor_config;
struct Cursor cursor;

enum Mode mode = NORMAL;

int getScreenSize(int *rows, int *cols) {
  struct winsize ws;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
    return -1;
  }
  *rows = ws.ws_row;
  *cols = ws.ws_col;
  return 0;
}

void fail(const char *s) {
  perror(s);
  exit(1);
}

void setCursor(int x, int y) {
  char *str;
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

void disableRawMode(void) {
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &editor_config.orig_termios) == -1)
    fail("tcsetattr");
}

void enableRawMode(void) {
  if (tcgetattr(STDIN_FILENO, &editor_config.orig_termios) == -1)
    fail("tcgetattr");
  atexit(disableRawMode);

  struct termios raw = editor_config.orig_termios;
  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  raw.c_oflag &= ~(OPOST);
  raw.c_cflag |= (CS8);
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
    fail("tcsetattr");
}

int main(void) {
  enableRawMode();
  getScreenSize(&editor_config.screen_rows, &editor_config.screen_cols);

  char c;

  clearScreen();
  while (1) {
    setCursor(cursor.x, cursor.y);

    if (read(STDIN_FILENO, &c, 1) == -1 && errno != EAGAIN)
      fail("read");
    if (mode == NORMAL) {
      if (c == 'q')
        break;
      else if (c == 'h' && cursor.x > 0)
        cursor.x--;
      else if (c == 'l')
        cursor.x++;
      else if (c == 'k' && cursor.y > 0)
        cursor.y--;
      else if (c == 'j')
        cursor.y++;
      else if (c == 'i' && mode == NORMAL)
        mode = INSERT;
    } else if (mode == INSERT) {
      if (c == '\x1b') {
        mode = NORMAL;
      }
      else {
        if (write(STDOUT_FILENO, &c, 1) == -1)
          fail("write");
        cursor.x++;
      }
    }
  }
  return 0;
}
