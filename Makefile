CC = gcc
CFLAGS = -std=c99 -c -g -D_GNU_SOURCE
LIBS = -lpthread
GCCWARN = -Wall -Wextra
CLANGWARN = -Weverything

cut.o: cut.c cut.h
ifeq ($(CC), gcc)
	$(CC) cut.c $(CFLAGS) $(GCCWARN) -o cut.o
else ifeq ($(CC), clang)
	$(CC) cut.c $(CFLAGS) $(CLANGWARN) -o cut.o
endif

cut: cut.o
	$(CC) cut.o $(LIBS) -o cut

clean:
	rm -f cut.o
	rm -f cut
	rm -f log.txt

