#include "piece_table.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int initTable(PieceTable* pieceTable, char* original_buffer, unsigned int buffer_size) {
  if (!pieceTable) return -1;
  pieceTable->original_buffer = original_buffer;
  pieceTable->original_buffer_size = buffer_size;
  pieceTable->new_buffer = NULL;
  pieceTable->new_buffer_size = 0;
  Entry* head = malloc(sizeof(Entry));
  if (!head) return -1;
  head->buffer = ORIGINAL;
  head->idx = 0;
  head->len = buffer_size;
  head->next = NULL;
  pieceTable->head = head;
  return 0;
}

int addToEntry(Entry* head, unsigned int position, unsigned int idx) {
  Entry* entry = head;
  Entry* prev = NULL;
  unsigned int currPos = 0;
  while (entry) {
    if (currPos + entry->len >= position)
      break;
    currPos += entry->len;
    prev = entry;
    entry = entry->next;
  }

  if (!entry) {
    Entry* newEntry = malloc(sizeof(Entry));
    newEntry->buffer = ADD;
    newEntry->idx = idx;
    newEntry->len = 1;
    newEntry->next = NULL;
    prev->next = newEntry;
    return 0;
  }

  if (entry->idx + entry->len == position) {
    if (entry->buffer == ADD) {
      entry->len++;
      return 0;
    }
    Entry* newEntry = malloc(sizeof(Entry));
    newEntry->buffer = ADD;
    newEntry->idx = idx;
    newEntry->len = 1;
    newEntry->next = entry->next;
    entry->next = newEntry;
    return 0;
  }

  unsigned int leftLen = position - currPos;
  unsigned int rightLen = entry->len - leftLen;

  Entry* midEntry = malloc(sizeof(Entry));
  Entry* rightEntry = malloc(sizeof(Entry));

  midEntry->buffer = ADD;
  midEntry->idx = idx;
  midEntry->len = 1;
  midEntry->next = rightEntry;

  rightEntry->buffer = entry->buffer;
  rightEntry->idx = entry->idx + leftLen;
  rightEntry->len = rightLen;
  rightEntry->next = entry->next;

  entry->len = leftLen;
  entry->next = midEntry;

  return 0;
}

unsigned int getIndexFromPosition(PieceTable* piece_table, unsigned int char_pos, unsigned int line) {
  unsigned int current_line = 0;
  unsigned int current_idx = 0;
  unsigned int i;
  char c;
  Entry* entry = piece_table->head;
  while (entry && current_line < line) {
    char* buff;
    if (entry->buffer == ORIGINAL)
      buff = piece_table->original_buffer;
    else
      buff = piece_table->new_buffer;
    for (i = 0; i < entry->len && current_line < line; i++) { 
      c = buff[entry->idx + i];
      if (c == '\n') {
        current_line++;
      }
      current_idx++;
    }
    if (current_line < line) entry = entry->next;
  }
  return current_idx + char_pos;
}

int addCharacter(PieceTable* pieceTable, char c, unsigned int position) {
  if (!pieceTable) return -1;
  pieceTable->new_buffer = realloc(pieceTable->new_buffer, sizeof(char) * (pieceTable->new_buffer_size + 1));
  pieceTable->new_buffer[pieceTable->new_buffer_size] = c;
  pieceTable->new_buffer_size++;
  addToEntry(pieceTable->head, position, pieceTable->new_buffer_size - 1);
  return 0;
}

int getLineLength(PieceTable* piece_table, unsigned int line) {
  unsigned int current_line = 0;
  char c;
  Entry* entry = piece_table->head;
  unsigned int i = 0;
  while (current_line < line && entry != NULL) {
    char* buff = NULL;
    if (entry->buffer == ORIGINAL)
      buff = piece_table->original_buffer;
    else
      buff = piece_table->new_buffer;
    for (i = 0; i < entry->len && current_line < line; i++) { 
      c = buff[entry->idx + i];
      if (c == '\n') {
        current_line++;
      }
    }
    if (current_line != line)
      entry = entry->next;
  }

  if (current_line != line) return 0;
  unsigned int length = 0;
  while (entry != NULL) {
    char* buff = NULL;
    if (entry->buffer == ORIGINAL)
      buff = piece_table->original_buffer;
    else
      buff = piece_table->new_buffer;
    for (; i < entry->len; i++) { 
      c = buff[entry->idx + i];
      if (c == '\n') {
        return length;
      }
      length++;
    }
    i = 0;
    entry = entry->next;
  }
  return length;
}

int deleteCharacter(PieceTable* pieceTable, unsigned int position) {
  if (!pieceTable) return -1;
  Entry* entry = pieceTable->head;
  unsigned int current_pos = 0;
  while (entry) {
    if (entry->len + current_pos > position) {
      break;
    }
    current_pos += entry->len;
    entry = entry->next;
  }
  if (!entry) return -1;

  if (position == current_pos) {
    entry->idx++;
    entry->len--;
    return 0;
  }
  
  if (position == current_pos + entry->len - 1) {
    entry->len--;
    return 0;
  }

  Entry* rightEntry = malloc(sizeof(Entry));
  if (!rightEntry) return -1;

  unsigned int left_length = position - current_pos;
  rightEntry->buffer = entry->buffer;
  rightEntry->idx = entry->idx + left_length + 1;
  rightEntry->len = entry->len - left_length - 1;
  rightEntry->next = entry->next;

  entry->len = left_length;
  entry->next = rightEntry;

  return 0;
}



int readContent(PieceTable* piece_table, char** buff) {
  Entry* entry = piece_table->head;
  char* original_buffer = piece_table->original_buffer;
  char* new_buffer = piece_table->new_buffer;
  unsigned int len = 0;

  while (entry) {
    len += entry->len;
    entry = entry->next;
  }

  *buff = malloc(sizeof(char) * len + 1);
  (*buff)[len] = '\0';

  entry = piece_table->head;
  unsigned int i = 0;
  while (entry) {
    for (unsigned int j = 0; j < entry->len; j++) {
      if (entry->buffer == ADD) {
        (*buff)[i] = new_buffer[entry->idx + j];
      }
      else {
        (*buff)[i] = original_buffer[entry->idx + j];
      }
      i++;
    }
    entry = entry->next;
  }
  return len + 1;
}
