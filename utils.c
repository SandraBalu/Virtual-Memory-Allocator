#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "vma.h"

list_t *dll_create(size_t data_size)
{
	list_t *list = malloc(sizeof(list_t));
	DIE(!list, "Failed\n");

	list->size = 0;
	list->data_size = data_size;
	list->head = NULL;

	return list;
}

node_t *dll_get_nth_node(list_t *list, uint64_t n)
{
	n %= list->size;
	node_t *node = list->head;
	for (int i = 0; i < n; i++)
		node = node->next;

	return node;
}

void dll_add_nth_node(list_t *list, uint64_t n, void *new_data)
{
	if (n > list->size)
		n = list->size;

	node_t *new = (node_t *)malloc(sizeof(node_t));
	DIE(!new, "Malloc failed\n");

	new->data = new_data;
	if (list->size == 0) {
		list->head = new;
		new->next = new;
		new->prev = new;
		list->size++;
		return;
	}

	node_t *it = list->head;
	if (n == 0) {
		it = it->prev;
	} else {
		for (unsigned int i = 0; i < n - 1; ++i)
			it = it->next;
	}

	new->next = it->next;
	it->next->prev = new;
	it->next = new;
	new->prev = it;

	if (n == 0)
		list->head = new;
	list->size++;
}

node_t *dll_rm_nth_node(list_t *list, uint64_t n)
{
	if (n < 0)
		return NULL;

	if (n >= list->size)
		n = list->size - 1;

	node_t *ret, *it;
	if (n == 0) {
		n = list->size - 1;
		list->head = list->head->next;
	}
	it = list->head;
	for (unsigned int i = 0; i < n - 1; ++i)
		it = it->next;

	ret = it->next;
	it->next = ret->next;
	ret->next->prev = it;
	ret->next = NULL;
	ret->prev = NULL;
	list->size--;
	return ret;
}

void dll_free_mini(list_t **pp_list)
{
	if ((*pp_list)->size == 0) {
		free(*pp_list);
		return;
	}

	if ((*pp_list)->size == 1) {
		miniblock_t *mini = (*pp_list)->head->data;
		if (mini->rw_buffer)
			free(mini->rw_buffer);
		free((*pp_list)->head->data);
		free((*pp_list)->head);
		free(*pp_list);
		return;
	}

	node_t *it = (*pp_list)->head->prev;
	miniblock_t *mini;
	for (unsigned int i = 0; i < (*pp_list)->size - 1; ++i) {
		it = it->prev;
		mini = it->next->data;
		if (mini->rw_buffer)
			free(mini->rw_buffer);
		free(it->next->data);
		free(it->next);
	}

	if (((miniblock_t *)it->data)->rw_buffer)
		free(mini->rw_buffer);

	free(it->data);
	free(it);
	free(*pp_list);
}

int verif_address(arena_t *arena, const uint64_t address, const uint64_t size)
{
	if (address >= arena->arena_size) {
		printf("The allocated address is outside the size of arena\n");
		return 1;
	}

	if (address + size > arena->arena_size) {
		printf("The end address is past the size of the arena\n");
		return 1;
	}

	node_t *node = arena->alloc_list->head;
	for (int i = 0; i < arena->alloc_list->size; i++) {
		block_t *block = (block_t *)node->data;
		if (block->start_address == address) {
			printf("This zone was already allocated.\n");
			return 1;
		}

		if (block->start_address < address + size &&
			address + size < block->start_address + block->size) {
			printf("This zone was already allocated.\n");
			return 1;
		}
		if (block->start_address < address &&
			address < block->start_address + block->size) {
			printf("This zone was already allocated.\n");
			return 1;
		}
		if (block->start_address > address &&
			block->start_address + block->size < address + size) {
			printf("This zone was already allocated.\n");
			return 1;
		}
		node = node->next;
	}
	return 0;
}

// function that checks and adds a new miniblock at
// the beginning of a block's corresponding list
int add_mb_to_left(arena_t *arena, const uint64_t address,
				   const uint64_t size)
{
	node_t *node;
	node = arena->alloc_list->head;
	// go through all the blocks in the arena
	for (int i = 0; i < arena->alloc_list->size; i++) {
		block_t *block = (block_t *)node->data;

		// if the end address of the miniblock is equal to the
		// start address of the block, add the new node with the
		// miniblock data at position 0 and exit the function
		if (block->start_address == address + size) {
			miniblock_t *miniblock = malloc(sizeof(miniblock_t));

			*miniblock = (miniblock_t){address, size, 6, NULL};
			dll_add_nth_node(block->mb_list, 0, miniblock);
			block->size += size;
			block->start_address = address;
			return 1;
		}
		node = node->next;
	}

	// could not add the miniblock at the beginning of any block
	return 0;
}

// function that checks and adds a new miniblock at
// the end of a block's corresponding list
int add_mb_to_right(arena_t *arena, const uint64_t address,
					const uint64_t size)
{
	node_t *node;
	node = arena->alloc_list->head;
	for (int i = 0; i < arena->alloc_list->size; i++) {
		block_t *block = (block_t *)node->data;

		// if the start address of the miniblock is equal to the
		// end address of the block, add a new node with the
		// miniblock's data at end and exit the function
		if (block->start_address + block->size == address) {
			miniblock_t *miniblock = malloc(sizeof(miniblock_t));
			*miniblock = (miniblock_t){address, size, 6, NULL};
			int final = block->mb_list->size;
			dll_add_nth_node(block->mb_list, final, miniblock);
			block->size += size;
			return 1;
		}
		node = node->next;
	}
	// could not add the miniblock at the end of any block
	return 0;
}

int no_minib(const arena_t *arena)
{
	int no = 0;
	node_t *node = arena->alloc_list->head;
	for (int i = 0; i < arena->alloc_list->size; i++) {
		block_t *block = (block_t *)node->data;
		no += ((list_t *)block->mb_list)->size;
		node = node->next;
	}
	return no;
}

uint64_t memory_used(const arena_t *arena)
{
	uint64_t mem = 0;
	node_t *node = arena->alloc_list->head;
	for (int i = 0; i < arena->alloc_list->size; i++) {
		block_t *block = (block_t *)node->data;
		mem += block->size;
		node = node->next;
	}
	return mem;
}

int verif_command(char *command)
{
	if (strncmp(command, "ALLOC_ARENA", 11) == 0) {
		if (strlen(command) != 11)
			return 1;
		return 0;
	}

	if (strncmp(command, "ALLOC_BLOCK", 11) == 0) {
		if (strlen(command) != 11)
			return 1;
		return 0;
	}

	if (strncmp(command, "PMAP", 4) == 0) {
		if (strlen(command) != 4)
			return 1;
		return 0;
	}
	if (strncmp(command, "FREE_BLOCK", 10) == 0) {
		if (strlen(command) != 10)
			return 1;
		return 0;
	}

	if (strncmp(command, "DEALLOC_ARENA", 13) == 0) {
		if (strlen(command) != 13)
			return 1;
		return 0;
	}
	if (strncmp(command, "READ", 4) == 0) {
		if (strlen(command) != 4)
			return 1;
		return 0;
	}
	if (strncmp(command, "WRITE", 5) == 0) {
		if (strlen(command) != 5)
			return 1;
		return 0;
	}
	if (strncmp(command, "MPROTECT", 8) == 0) {
		if (strlen(command) != 8)
			return 1;
		return 0;
	}
	return 1;
}
