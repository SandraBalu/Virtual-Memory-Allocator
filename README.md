# Virtual Memory Allocaor 

### Description:
The program creates a virtual memory allocator which is represented by an
arena. The arena has several components: size - measured in bytes, and a doubly
linked list of blocks. The nodes of this list contain the lists of miniblocks 
allocated during the program. Miniblocks are arena components that contain 
addresses, sizes, buffers and permissions that we alloc, fee and modify
according to the command read from the keyboard. Regardless of the commands
executed, at the end of the program run, memory errors are null.

There are the following commands:

## ALLOC_ARENA <size>
For this command it will be built an arena that will have 2 fields: a numeric
field in which we save its size, not allocating all the memory and a doubly 
linked list currently empty.

## DEALLOC_ARENA
For this command we will go through the entire list of blocks in the arena. For
each block, we release the list of miniblocks already allocated to it using an
auxiliary function, then we release the member that holds the block information
(date) and then we release the node itself. After all nodes have been
deallocated, free the empty block list and the arena

## ALLOC_BLOCK <address> <size>
For this command, first check if the address and size given as parameter can be
allocated in the arena. If the space that the new block occupies can be 
allocated, then we first check whether the new block will stick to the input of
another block already in the arena, effectively becoming a miniblock within
another block. If so, we then check if the beginning of the new block is at the
end of another existing block (i.e. if the added miniblock joins 2 already
allocated blocks). If this is true, we add the miniblocks of the block to be
concatenated and at the end, we deallocate the whole block that was copied at 
the beginning of the modified block.
If the block has not been added to the beginning of another block, check if it
will stick to the end of an existing block. The verifications made in the 
previous case are no longer necessary because as long as we check in one of the
cases, in whatever order we add the blocks, they will still merge.
If the block has not been glued to any existing block, then we determine the 
position where it will be added (blocks should be ordered ascending by starting
address to make these verifications correct and easier) and allocate it in the 
arena.

## FREE_BLOCK <address>
For this command, we check if we have the possibility to free a miniblock 
(meaning if there is a block that contains the address where the release will
be made). If the address is at the beginning of a block that contains only one
miniblock, then the entire block will be released.If the address is at the 
beginning or end of a block, only the corresponding miniblock will be released.
If the miniblock is inside a block, first we remove the miniblock from the list
and then free it. The current block is thus transformed into 2 new adjacent
blocks - we move all the miniblocks that were positioned before the deleted
miniblock into a new block which we add to the arena before the block that was
just removed.

## WRITE <address> <size> <buffer>
To perform this command, we check if there is a block that contains the address
where the write operation starts. If so, we check if the permissions of the
miniblock in which the address is found allow writing. If the number of
characters to be written is less than the size of the miniblock, then we fill
up its buffer and the writing is completed. If this was not possible, however,
we continue writing in adjacent miniblocks as long as there is enough space and
their permissions allow it (if at least one miniblock does not have the 
permission W, the writing stops). If the miniblocks and their sizes were not
sufficient to write all the desired characters, an appropriate message
is displayed.

## READ <address> <size>
For this command we first determine the block where the read will be performed.
Then, we go through the miniblock list until we find the miniblock in which the
start address is located and if the miniblock has the permission R the reading
starts. We use an auxiliary buffer to copy the characters of the first
miniblock, starting from the desired address (this is not always at the
beginning of the minblock). As long as the desired number of characters have
not been read and as long as there are adjacent miniblocks with R permission,
the buffer is filled with their characters. At the end, if the number of
characters that have been read is less than the desired one, an appropriate
message is displayed and then the buffer.

## PMAP
This command displays detailed details about the arena and its components. 
First we display general details about the memory used and the number of 
components. Then, by going through each block and each list of corresponding 
miniblocks in turn, the arena is sectioned and all details about its components
are displayed.

## MPROTECT <address> <permissions>
For this command an address and various permissions are read and need to be 
updated. After we determine if there is a miniblock in the arena at the address
we are looking for, its permissions are replaced with the ones given as a
parameter. Permissions are encoded by a standard numeric value and miniblocks
when assigned at the beginning are given by default the permissions RW (6).

### Comments:

* The implementation leaves a little to be desired because of repeated deferences, but I feel much more in control of the pointers on this topic.
