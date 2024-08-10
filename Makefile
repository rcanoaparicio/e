main: src/main.c src/piece_table.c src/piece_table.h
	$(CC) -o main src/main.c src/piece_table.c -Wall -Wextra -pedantic -std=c99

