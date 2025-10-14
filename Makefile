CC=gcc
CFLAGS=`pkg-config --cflags glib-2.0` -O2 -Wall -Wextra
LDLIBS=`pkg-config --libs glib-2.0`

all: finalizador

finalizador: finalizador.c

clean:
	rm -f finalizador
