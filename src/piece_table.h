#ifndef PIECE_TABLE
#define PIECE_TABLE

typedef enum BufferType {
	ORIGINAL,
	ADD
} BufferType;

typedef struct Entry {
	BufferType buffer;
	unsigned int idx;
	unsigned int len;
	struct Entry* next;
} Entry;

typedef struct PieceTable {
	Entry* head;
	char* original_buffer;
	unsigned int original_buffer_size;
	char* new_buffer;
	unsigned int new_buffer_size;
} PieceTable;

int addCharacter(PieceTable* pieceTable, char c, unsigned int idx);

int deleteCharacter(PieceTable* pieceTable, unsigned int idx);

int initTable(PieceTable* pieceTable, char* original_buffer, unsigned int buffer_size);

int readContent(PieceTable* piece_table, char** buff);

#endif
