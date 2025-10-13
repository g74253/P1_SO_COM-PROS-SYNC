CC=gcc
CFLAGS=`pkg-config --cflags glib-2.0` -O2 -Wall -Wextra
LDLIBS=`pkg-config --libs glib-2.0`

all: inicializador

inicializador: inicializador.c

clean:
	rm -f inicializador
