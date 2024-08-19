#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include "piece_table.h"

struct EditorConfig {
  struct termios orig_termios;
  int screen_rows;
  int screen_cols;
};

typedef struct Cursor {
  int x;
  int y;
} Cursor;

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


void writeContentInScreen(PieceTable* piece_table) {
  const unsigned int BUFF_SIZE = 512; 
  char* contents_buff;
  unsigned int contents_len = readContent(piece_table, &contents_buff);

  char write_buff[BUFF_SIZE];
  unsigned int contents_i = 0;
  unsigned int write_i = 0;

  while (contents_i < contents_len) {
    while (contents_i < contents_len && write_i < BUFF_SIZE) {
      if (contents_buff[contents_i] == '\n') {
        if (write_i == BUFF_SIZE -1) break;
        write_buff[write_i++] = '\r';
        write_buff[write_i++] = contents_buff[contents_i++];
      }
      else {
        write_buff[write_i++] = contents_buff[contents_i++];
      }
    }
    if (write(STDOUT_FILENO, write_buff, sizeof(char)*write_i) == -1)
      fail("write");
    write_i = 0;
  }
}

int readFile(char** buffer, char* file_name) {
  FILE* file = fopen(file_name, "r");
  if (file == NULL) fail("Could not open file");
  if (fseek(file, 0, SEEK_END) == -1) fail("Seek end");
  unsigned int end_pos = ftell(file);
  printf("%s end pos is %d", file_name, end_pos);
  unsigned int buff_size = end_pos + 1;
  (*buffer) = malloc(sizeof(char) * buff_size);
  if (fseek(file, 0, SEEK_SET) != 0) fail("Seek begin");
  fread(*buffer, sizeof(char), buff_size, file);
  if (ferror(file) != 0) fail("fread");
  fclose(file);
  (*buffer)[end_pos] = '\0';
  return buff_size;
}

int moveCursorX(Cursor* cursor, int x, PieceTable* piece_table) {
  if (cursor == NULL) {
    return -1;
  }
  if (x < 0 && cursor->x + x >= 0) {
    cursor->x += x;
  }

  if (x > 0) {
    int line_length = getLineLength(piece_table, cursor->y);
    cursor->x =
      cursor->x + x < line_length ? cursor->x + x : cursor->x; 
  }

  return 0;
}

int moveCursorY(Cursor* cursor, int y, PieceTable* piece_table) {
  if (cursor == NULL) {
    return -1;
  }
  if ((y < 0 && cursor->y + y >= 0) || y > 0) {
    int line_length = getLineLength(piece_table, cursor->y + y);
    if (line_length == 0) return 0;
    cursor->y += y;
    if (line_length - 1 < cursor->x) cursor->x = line_length - 1;
  }

  return 0;
}

int main(int argc, char** argv) {
  char* initial_content = ""; 
  int initial_content_size = 0;
  if (argc >= 2) {
    initial_content_size = readFile(&initial_content, argv[1]);
    printf("initial content  size: %d", initial_content_size);
  }

  enableRawMode();
  getScreenSize(&editor_config.screen_rows, &editor_config.screen_cols);

  PieceTable pieceTable;
  if (initTable(&pieceTable, initial_content, initial_content_size) == -1)
    fail("initTable");

  char c;

  clearScreen();
  while (1) {
    clearScreen();
    writeContentInScreen(&pieceTable);
    setCursor(cursor.x, cursor.y);

    if (read(STDIN_FILENO, &c, 1) == -1 && errno != EAGAIN)
      fail("read");
    if (mode == NORMAL) {
      if (c == 'q')
        break;
      else if (c == 'h' && cursor.x > 0)
        moveCursorX(&cursor, -1, &pieceTable);
      else if (c == 'l')
        moveCursorX(&cursor, +1, &pieceTable);
      else if (c == 'k' && cursor.y > 0)
        moveCursorY(&cursor, -1, &pieceTable);
      else if (c == 'j')
        moveCursorY(&cursor, +1, &pieceTable);
      else if (c == 'i')
        mode = INSERT;
      else if (c == 'a') {
        mode = INSERT;
        cursor.x++;
      }
      else if (c == 'x') {
        unsigned int position = getIndexFromPosition(&pieceTable, cursor.x, cursor.y);
        if (deleteCharacter(&pieceTable, position) == -1)
          fail("Delete character");
      }
      else if (c == 'X' && cursor.x > 0) {
        unsigned int position = getIndexFromPosition(&pieceTable, cursor.x, cursor.y);
        if (deleteCharacter(&pieceTable, position - 1) == -1)
          fail("Delete character");
        cursor.x--;
      }
    } else if (mode == INSERT) {
      if (c == '\x1b') {
        mode = NORMAL;
        moveCursorX(&cursor, -1, &pieceTable);
      }
      else {
        unsigned int position = getIndexFromPosition(&pieceTable, cursor.x, cursor.y);
        if (addCharacter(&pieceTable, c, position) == -1)
          fail("PieceTable addCharacter");
        cursor.x++;
      }
    }
  }
  char* contents = NULL;
  int content_len = readContent(&pieceTable, &contents);
  printf("\r\nContents len: %d\r\n", content_len);
  printf("Contents %s\r\n", contents);
  return 0;
}
