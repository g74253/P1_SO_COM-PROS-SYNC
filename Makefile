# Makefile
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2
LDFLAGS = -lpthread -lrt

# Nombres de los ejecutables
TARGETS = inicializador emisor receptor finalizador

all: $(TARGETS)

inicializador: inicializador.c shared.h
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

emisor: emisor.c shared.h ipc_utils.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

receptor: receptor.c shared.h ipc_utils.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

finalizador: finalizador.c shared.h ipc_utils.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(TARGETS)

.PHONY: all clean