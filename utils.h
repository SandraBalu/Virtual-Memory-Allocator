#pragma once
#include <inttypes.h>
#include <stddef.h>
#include "vma.h"

// function that creates and returns a doubly linked list
list_t *dll_create(size_t data_size);
// function that returns the nth node in a doubly linked list
node_t *dll_get_nth_node(list_t *list, uint64_t n);
// function that adds a new node to the nth position in a doubly linked list
void dll_add_nth_node(list_t *list, uint64_t n, void *new_data);
// function that adds removes the nth node from a doubly linked list
node_t *dll_rm_nth_node(list_t *list, uint64_t n);
// function that frees the allocated memory from a list of miniblocks
void dll_free_mini(list_t **pp_list);

// auxiliary function that verifies if a block with a certain
// start address and size can be added to the arena
int verif_address(arena_t *arena, const uint64_t address, const uint64_t size);
// function that checks and adds a new miniblock at
// the beginning of a block's corresponding list
int add_mb_to_left(arena_t *arena, const uint64_t address,
				   const uint64_t size);
// function that checks and adds a new miniblock at
// the end of a block's corresponding list
int add_mb_to_right(arena_t *arena, const uint64_t address,
					const uint64_t size);

// function that determines the total number of miniblocks in the arena
int no_minib(const arena_t *arena);
// function that determines the memory occupied by all elements in the arena
uint64_t memory_used(const arena_t *arena);

// function that verifies if the commands are volid
int verif_command(char *command);
