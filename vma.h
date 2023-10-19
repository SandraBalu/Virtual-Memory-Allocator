#pragma once
#include <inttypes.h>
#include <stddef.h>

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define DIE(assertion, call_description)                                       \
	do {                                                                       \
		if (assertion) {                                                       \
			fprintf(stderr, "(%s, %d): ", __FILE__, __LINE__);                 \
			perror(call_description);                                          \
			exit(errno);                                                       \
		}                                                                      \
	} while (0)

typedef struct node_t node_t;
struct node_t {
	void *data;
	node_t *prev, *next;
};

typedef struct {
	node_t *head;
	uint64_t data_size;
	int size;
} list_t;

typedef struct {
	uint64_t start_address;
	size_t size;
	list_t *mb_list;
} block_t;

typedef struct {
	uint64_t start_address;
	size_t size;
	uint8_t perm;
	void *rw_buffer;
} miniblock_t;

typedef struct {
	uint64_t arena_size;
	list_t *alloc_list;
} arena_t;

// function that creates and returns a new arena
arena_t *alloc_arena(const uint64_t size);
// function that frees all memory occupied by the arena
void dealloc_arena(arena_t *arena);

// function that adds a new block to the arena at a given address
void alloc_block(arena_t *arena, const uint64_t address, const uint64_t size);
// function that frees the memory allocated to an entire block
void free_block(arena_t *arena, const uint64_t address);
// function that releases a single miniblock
void free_miniblock(arena_t *arena, const uint64_t address);

// function that receives as parameter an address and a size
// and determines if they correspond to a block. if so, it browses
// the corresponding miniblocks and if allowed, reads their allocated
// buffers. if the size given as parameter is bigger than the read
// size, it displays a corresponding message
void read(arena_t *arena, uint64_t address, uint64_t size);

// function that writes to a certain address in the arena a
// number of characters given as a parameter. if the address
// has not been allocated or the appropriate permissions do
// not exist, the function displays an invalid message and
// exits, otherwise it writes as many characters as there is
// space in the appropriate block. if the number of characters
// written is less than the desired number, a message is displayed as such
void write(arena_t *arena, const uint64_t address, const uint64_t size,
		   char *data);

// function that displays all details about the arena and its components
void pmap(const arena_t *arena);

// function that modifies the permisiions of an miniblock
void mprotect(arena_t *arena, uint64_t address, int8_t *permission);
