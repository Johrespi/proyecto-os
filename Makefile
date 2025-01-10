CC = gcc
CFLAGS = -Wall -pthread
LDFLAGS = -lrt

FILES = publicador desenfocador realzador combinador
SRC = bmp.c

all: $(FILES)

publicador: publicador.c bmp.c common.h bmp.h
	$(CC) $(CFLAGS) -o publicador publicador.c bmp.c $(LDFLAGS)

desenfocador: desenfocador.c bmp.c common.h bmp.h
	$(CC) $(CFLAGS) -o desenfocador desenfocador.c bmp.c $(LDFLAGS)

realzador: realzador.c bmp.c common.h bmp.h
	$(CC) $(CFLAGS) -o realzador realzador.c bmp.c $(LDFLAGS)

combinador: combinador.c bmp.c common.h bmp.h
	$(CC) $(CFLAGS) -o combinador combinador.c bmp.c $(LDFLAGS)

clean:
	 rm -f $(FILES)
