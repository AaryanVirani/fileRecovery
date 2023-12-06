CC=gcc
CFLAGS=-g -pedantic -std=gnu17 -Wall -Werror -Wextra
LDFLAGS=-lcrypto

.PHONY: all
all: nyufile

nyufile: nyufile.o implement.o

nyufile.o: nyufile.c implement.h

implement.o: implement.c implement.h

.PHONY: clean
clean:
	rm -f *.o nyufile
