#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "vma.h"

int main(void)
{
	arena_t *arena;
	while (1) {
		char command[30];
		scanf("%s", command);

		if (verif_command(command) == 1)
			printf("Invalid command. Please try again.\n");

		if (strncmp(command, "ALLOC_ARENA", 11) == 0) {
			uint64_t DIM;
			scanf("%ld", &DIM);
			arena = alloc_arena(DIM);
		}

		if (strncmp(command, "ALLOC_BLOCK", 11) == 0) {
			uint64_t address;
			size_t size;
			scanf("%ld", &address);
			scanf("%zd", &size);
			alloc_block(arena, address, size);
		}

		if (strncmp(command, "PMAP", 4) == 0)
			pmap(arena);

		if (strncmp(command, "FREE_BLOCK", 10) == 0) {
			uint64_t address;
			scanf("%ld", &address);
			free_miniblock(arena, address);
		}

		if (strcmp(command, "WRITE") == 0) {
			uint64_t address, size;
			scanf("%ld %ld", &address, &size);

			char spatiu;
			scanf("%c", &spatiu);
			char string[size + 1];
			for (int i = 0; i <= size; i++)
				scanf("%c", &string[i]);

			string[size + 1] = '\0';
			write(arena, address, size, string);
		}

		if (strncmp(command, "READ", 4) == 0) {
			uint64_t address, size;
			scanf("%ld %ld", &address, &size);
			read(arena, address, size);
		}

		if (strcmp(command, "MPROTECT") == 0) {
			uint64_t addressp;
			char permissions[60];
			scanf("%ld", &addressp);
			fgets(permissions, 60, stdin);
			int8_t newp = 0;
			char *p = strtok(permissions, " |");
			while (p) {
				if (strncmp(p, "PROT_NONE", 9) == 0)
					newp += 0;
				if (strncmp(p, "PROT_READ", 9) == 0)
					newp += 4;
				if (strncmp(p, "PROT_WRITE", 10) == 0)
					newp += 2;
				if (strncmp(p, "PROT_EXEC", 9) == 0)
					newp += 1;
				p = strtok(NULL, " |");
			}
			mprotect(arena, addressp, &newp);
		}
		if (strncmp(command, "DEALLOC_ARENA", 13) == 0) {
			dealloc_arena(arena);
			break;
		}
	}
	return 0;
}
