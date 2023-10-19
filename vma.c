#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "vma.h"

arena_t *alloc_arena(const uint64_t size)
{
	arena_t *arena = malloc(sizeof(arena_t));
	DIE(!arena, "Malloc failed\n");

	arena->arena_size = size;
	arena->alloc_list = dll_create(sizeof(block_t));

	return arena;
}

void dealloc_arena(arena_t *arena)
{
	list_t *block_list = arena->alloc_list;
	node_t *it = block_list->head, *aux;
	// we go through each node in the arena block list
	for (unsigned int i = 0; i < (block_list)->size; ++i) {
		aux = it->prev;
		block_t *block = (block_t *)it->data;
		// first, free the memory allocated to the miniblock list
		dll_free_mini(&((block_t *)it->data)->mb_list);
		// free the whole block
		free(it->data);
		free(it);
		it = aux;
	}
	// dealloc the memory of the arena
	free(block_list);
	free(arena);
}

void alloc_block(arena_t *arena, const uint64_t address, const uint64_t size)
{
	// check the address date and size of the new block can be allocated
	if (verif_address(arena, address, size) == 1)
		return;

	if (add_mb_to_left(arena, address, size) == 1) {
		node_t *node_block = arena->alloc_list->head;

		// when we add a miniblock to an already existing block on
		// the first position, we check if the new miniblock joins 2
		// already existing blocks
		for (int i = 0; i < arena->alloc_list->size; i++) {
			if (((block_t *)node_block->data)->start_address +
					((block_t *)node_block->data)->size ==
				address) {
				// add all the elements in the block to be attached
				// and update the new dimensions
				block_t *bl_to_move = (block_t *)node_block->data;
				block_t *bl_where_to_mv = (block_t *)node_block->next->data;

				bl_where_to_mv->size += bl_to_move->size;
				bl_where_to_mv->start_address = bl_to_move->start_address;
				node_t *mini = ((list_t *)bl_to_move->mb_list)->head;
				list_t *lmini = (list_t *)bl_to_move->mb_list;
				for (int i = 0; i < lmini->size; i++) {
					uint64_t new_address =
						((miniblock_t *)mini->data)->start_address;
					uint64_t new_size = ((miniblock_t *)mini->data)->size;
					miniblock_t *miniblock = malloc(sizeof(miniblock_t));
					*miniblock = (miniblock_t){new_address, new_size, 6, NULL};
					dll_add_nth_node((list_t *)bl_where_to_mv
							->mb_list, i, miniblock);
					mini = mini->next;
				}
				// free the old block
				free_block(arena, ((block_t *)node_block->data)->start_address);
				break;
			}
			node_block = node_block->next;
		}
		return;
	}
	// the previous check is no longer necessary
	if (add_mb_to_right(arena, address, size) == 1)
		return;

	//if the block has not been assigned to another existing block, det.
	//the position to be placed in according to the address and add it
	int poz = 0;
	node_t *it = arena->alloc_list->head;
	for (int i = 0; i < arena->alloc_list->size; i++) {
		if (((block_t *)it->data)->start_address < address)
			poz++;
		it = it->next;
	}

	block_t *block = malloc(sizeof(block_t));
	DIE(!block, "Malloc failed\n");
	list_t *list_mb = dll_create(sizeof(miniblock_t));
	*block = (block_t){address, size, list_mb};
	miniblock_t *miniblock = malloc(sizeof(miniblock_t));
	*miniblock = (miniblock_t){address, size, 6, NULL};
	dll_add_nth_node(block->mb_list, 0, miniblock);
	dll_add_nth_node(arena->alloc_list, poz, block);
}

void free_block(arena_t *arena, const uint64_t address)
{
	list_t *block_list = arena->alloc_list;
	node_t *it = block_list->head, *aux;
	int poz = 0;

	for (unsigned int i = 0; i < (block_list)->size; ++i) {
		// if the block to be freed is on the first position,
		// change the head of the list from the arena
		if (((block_t *)it->data)->start_address == address) {
			if (poz == 0)
				arena->alloc_list->head = arena->alloc_list->head->next;

			it->prev->next = it->next;
			it->next->prev = it->prev;

			// free the block and its components
			dll_free_mini(&((block_t *)it->data)->mb_list);
			free(it->data);
			free(it);

			// update the new arena size
			arena->alloc_list->size--;
			return;
		}
		poz++;
		it = it->next;
	}
}

void free_miniblock(arena_t *arena, const uint64_t address)
{
	list_t *block_list = arena->alloc_list;
	if (block_list->size == 0) {
		printf("Invalid address for free.\n");
		return;
	}
	node_t *it = block_list->head, *aux; int poz = 0;
	for (unsigned int i = 0; i < (block_list)->size; ++i) {
		block_t *bit = (block_t *)it->data;
		uint64_t start = bit->start_address;
		//if the mb belongs to a block with 1 el., free the block
		if (start == address && ((list_t *)bit->mb_list)->size == 1) {
			free_block(arena, address);
			return;
		}
		if (start <= address && start + bit->size > address) {
			// if the miniblock is inside a multi-component block,
			// determine if the address is valid and its position
			if (((list_t *)bit->mb_list)->size == 1) {
				printf("Invalid address for free.\n");
				return;
			}
			int poz = 0;
			for (int j = 0; j < ((list_t *)bit->mb_list)->size; j++) {
				node_t *mverif = dll_get_nth_node((list_t *)bit->mb_list, j);
				if (((miniblock_t *)mverif->data)->start_address == address) {
					poz = j; break;
				}
			}
			// if the mb is at the beginning or at the end,
			// free and update the dimensions
			if (poz == 0 ||
				poz == ((list_t *)bit->mb_list)->size - 1) {
				node_t *mini = dll_rm_nth_node((list_t *)bit->mb_list, poz);
				if (poz == 0)
					bit->start_address += ((miniblock_t *)mini->data)->size;

				bit->size -= ((miniblock_t *)mini->data)->size;
				if (((miniblock_t *)mini->data)->rw_buffer)
					free(((miniblock_t *)mini->data)->rw_buffer);

				free(mini->data);
				free(mini);
				return;
			}
			// if mb is in the middle, divide the remaining block into 2 blocks
			node_t *mini = dll_rm_nth_node((list_t *)
							((block_t *)it->data)->mb_list, poz);
			block_t *block = malloc(sizeof(block_t));
			DIE(!block, "Malloc failed\n");
			list_t *list_mb = dll_create(sizeof(miniblock_t));
			*block = (block_t){0, 0, list_mb};

			block->start_address = ((block_t *)it->data)->start_address;
			block->size = 0;
			((block_t *)it->data)->size -= ((miniblock_t *)mini->data)->size;
			((block_t *)it->data)->start_address =
				((miniblock_t *)mini->data)->size +
				((miniblock_t *)mini->data)->start_address;

			free(mini->data);
			free(mini);
			for (int j = 0; j < poz; j++) {
				node_t *newm = dll_rm_nth_node((list_t *)
								((block_t *)it->data)->mb_list, 0);
				((block_t *)it->data)->size -=
					((miniblock_t *)newm->data)->size;
				miniblock_t *newmini = (miniblock_t *)newm->data;

				dll_add_nth_node(block->mb_list, j, newmini);
				block->size += ((miniblock_t *)newm->data)->size;
			}
			dll_add_nth_node(arena->alloc_list, i, block);
			return;
		}
		it = it->next;
	}
	printf("Invalid address for free.\n");
}

void read(arena_t *arena, uint64_t address, uint64_t size)
{
	int index = -1;
	node_t *it = arena->alloc_list->head;
	for (int i = 0; i < arena->alloc_list->size; i++) {
		if (((block_t *)it->data)->start_address <= address &&
			((block_t *)it->data)->start_address + ((block_t *)it->data)->size >
				address) {
			index = i; break;
		}
		it = it->next;
	}

	if (index == -1) {
		printf("Invalid address for read.\n");
		return;
	}
	// if the index has been determined, check permissions and start reading
	node_t *read = dll_get_nth_node(arena->alloc_list, index);
	node_t *mini =
		(node_t *)((list_t *)((block_t *)read->data)->mb_list)->head;

	for (int i = 0; i < ((list_t *)
			((block_t *)read->data)->mb_list)->size; i++) {
		if (((miniblock_t *)mini->data)->start_address <= address &&
			((miniblock_t *)mini->data)->start_address + ((miniblock_t *)
			mini->data)->size > address) {
			miniblock_t *curr = (miniblock_t *)mini->data;
			uint8_t perm = curr->perm;
			if (perm == 0 || perm == 2 || perm == 1 || perm == 3) {
				printf("Invalid permissions for read.\n");
				return;
			}
			uint64_t copy = size;
			int poz = address - curr->start_address;
			char print[size + 1]; int j = 0;
			// reads from the first miniblock, maximum number of characters
			for (int k = poz; k < strlen(curr->rw_buffer); k++) {
				if (k > size)
					break;
				print[j] = ((char *)curr->rw_buffer)[k];
				j++;
			}
			copy -= j;
			// if the no. read is equal to the no. requested, the reading ends
			if (copy <= 0) {
				print[j] = '\0';
				printf("%s\n", print);
				return;
			}
			// if the reading is not finished, continue with the next miniblocks
			while (i < ((list_t *)((block_t *)read->data)
					->mb_list)->size - 1 && copy > 0) {
				i++;
				mini = mini->next; curr = (miniblock_t *)mini->data;
				perm = curr->perm;
				if (perm == 0 || perm == 2 || perm == 1 || perm == 3) {
					printf("Invalid permissions for read.\n");
					return;
				}

				int nr = 0;
				for (int k = 0; k < strlen(curr->rw_buffer); k++) {
					if (k >= copy)
						break;
					print[j] = ((char *)curr->rw_buffer)[k];
					j++; nr++;
				}
				copy -= nr;
			}
			// display characters that have been read
			print[j] = '\0';
			if (strlen(print) < size)
				printf("Warning: size was bigger than the block size. Reading "
					   "%ld characters.\n", strlen(print));
			printf("%s\n", print);
		}
		mini = mini->next;
	}
}

void write(arena_t *arena, const uint64_t address, const uint64_t size,
		   char *data)
{
	int index = -1; int total = 0;
	node_t *it = arena->alloc_list->head;
	for (int i = 0; i < arena->alloc_list->size; i++) {
		if (((block_t *)it->data)->start_address <= address && ((block_t *)
		it->data)->start_address + ((block_t *)it->data)->size > address) {
			index = i; break;
		}
		it = it->next;
	}
	if (index == -1) {
		printf("Invalid address for write.\n");
		return;
	}
	// if the index has been determined, start writing
	node_t *write = dll_get_nth_node(arena->alloc_list, index);
	list_t *wlist = (list_t *)((block_t *)write->data)->mb_list;
	node_t *mini = (node_t *)wlist->head;
	miniblock_t *mb = (miniblock_t *)mini->data;
	for (int i = 0; i < wlist->size; i++) {
		if (mb->start_address <= address &&
			mb->start_address + mb->size > address) {
			miniblock_t *curr = (miniblock_t *)mini->data;
			uint8_t perm = curr->perm;
			if (perm == 0 || perm == 4 || perm == 1 || perm == 5) {
				printf("Invalid permissions for write.\n");
				return;
			}
			uint64_t ctotal = curr->size + curr->start_address;
			// if the first minib has room for the whole buffer, write and exit
			if (ctotal - address >= size) {
				curr->rw_buffer = malloc((size + 1) * sizeof(char));
				for (int i = 0; i < size; i++)
					((char *)curr->rw_buffer)[i] = data[i];
				((char *)curr->rw_buffer)[size] = '\0';
				return;
			}
			//complete the firt mb and then move to the next
			curr->rw_buffer = malloc((ctotal - address + 1) *
										sizeof(char));
			for (int j = 0; j < ctotal - address; j++) {
				((char *)curr->rw_buffer)[j] = data[total];
				total++;
			}

			((char *)curr->rw_buffer)[ctotal - address] = '\0';
			while (total < size && i < wlist->size - 1) {
				i++; mini = mini->next;
				curr = (miniblock_t *)mini->data;
				perm = curr->perm;
				if (perm == 0 || perm == 4 || perm == 1 || perm == 5) {
					printf("Invalid permissions for write.\n");
					return;
				}
				curr->rw_buffer =
						malloc((curr->size + 1) * sizeof(char));
				if (size - total > curr->size) {
					for (int j = 0; j < curr->size; j++) {
						((char *)curr->rw_buffer)[j] = data[total];
						total++;
					}
					((char *)curr->rw_buffer)[curr->size] = '\0';
				} else {
					int j = 0;
					while (total < size) {
						((char *)curr->rw_buffer)[j] = data[total];
						total++; j++;
					}
					((char *)curr->rw_buffer)[j] = '\0';
				}
			}
			// if the given size is greater than the written number, display
			if (total < size)
				printf("Warning: size was bigger than the block size. Writing "
					   "%d characters.\n", total);
		}
		mini = mini->next;
	}
}

void pmap(const arena_t *arena)
{
	node_t *node;
	node = arena->alloc_list->head;
	printf("Total memory: 0x%lX bytes\n", arena->arena_size);
	uint64_t mem_used = memory_used(arena);
	printf("Free memory: 0x%lX bytes\n", arena->arena_size - mem_used);
	printf("Number of allocated blocks: %d\n", arena->alloc_list->size);
	int no_mb = no_minib(arena);
	printf("Number of allocated miniblocks: %d\n", no_mb);

	// Iterate through all the blocks and display the miniblocks
	for (int i = 0; i < arena->alloc_list->size; i++) {
		printf("\nBlock %d begin\n", i + 1);
		block_t *block = (block_t *)node->data;
		uint64_t final = block->start_address + block->size;
		printf("Zone: 0x%lX - 0x%lX\n", block->start_address, final);

		// for each miniblock display its details
		node_t *node_mini = ((list_t *)block->mb_list)->head;
		for (int i = 0; i < ((list_t *)block->mb_list)->size; i++) {
			miniblock_t *miniblock = (miniblock_t *)node_mini->data;
			printf("Miniblock %d:\t\t0x%lX\t\t-\t\t0x%lX\t\t", i + 1,
				   miniblock->start_address,
				   miniblock->start_address + miniblock->size);
			// display the coresponding permission
			if (miniblock->perm == 6)
				printf("| RW-\n");
			if (miniblock->perm == 7)
				printf("| RWX\n");
			if (miniblock->perm == 0)
				printf("| ---\n");
			if (miniblock->perm == 2)
				printf("| R--\n");
			if (miniblock->perm == 4)
				printf("| -W-\n");
			if (miniblock->perm == 1)
				printf("| --X\n");
			if (miniblock->perm == 3)
				printf("| R-X\n");
			if (miniblock->perm == 5)
				printf("| -WX\n");
			node_mini = node_mini->next;
		}
		printf("Block %d end\n", i + 1);
		node = node->next;
	}
}

void mprotect(arena_t *arena, uint64_t address, int8_t *permission)
{
	node_t *it = arena->alloc_list->head;
	int index = -1;
	for (int i = 0; i < arena->alloc_list->size; i++) {
		if (((block_t *)it->data)->start_address <= address &&
			((block_t *)it->data)->start_address + ((block_t *)it->data)->size >
				address) {
			index = i;
			break;
		}
		it = it->next;
	}

	if (index == -1) {
		printf("Invalid address for mprotect.\n");
		return;
	}

	node_t *block = dll_get_nth_node(arena->alloc_list, index);
	node_t *mini =
		(node_t *)((list_t *)((block_t *)block->data)->mb_list)->head;

	for (int i = 0;
		 i < ((list_t *)((block_t *)block->data)->mb_list)->size; i++) {
		miniblock_t *curr = (miniblock_t *)mini->data;
		if (curr->start_address == address) {
			curr->perm = (uint8_t)*permission;
			return;
		}
		mini = mini->next;
	}
	printf("Invalid address for mprotect.\n");
}
