
# compiler setup
CC=gcc
CFLAGS=-Wall -Wextra -std=c99 -lm

# define targets
TARGETS=main

build: $(TARGETS)

main: main.c utils.c vma.c
	$(CC) $(CFLAGS) main.c utils.c vma.c -o vma -lm

pack:
	zip -FSr 314CA_BaluSandra_Tema1.zip README Makefile *.c *.h

clean:
	rm -f $(TARGETS)

run_vma: 
	./vma

.PHONY: pack clean