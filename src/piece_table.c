#include "piece_table.h"
#include <stdlib.h>

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
  pieceTable->head = head;
  return 0;
}

int addCharacter(PieceTable* pieceTable, char c, unsigned int idx) {
  if (!pieceTable) return -1;
  char* new_buffer = malloc(pieceTable->new_buffer_size + 1);
  if (!new_buffer) return -1;
  if (pieceTable->new_buffer)
    free(pieceTable->new_buffer);
  pieceTable->new_buffer = new_buffer;
  pieceTable->new_buffer[pieceTable->new_buffer_size] = c;
  pieceTable->new_buffer_size++;
  return 0;
}
