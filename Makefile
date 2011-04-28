CC=gcc
CFLAGS=-g -Wall

all: rut

rut: main.c rut.o nb0.o mlf.o util.o
	$(CC) $(CFLAGS) main.c rut.o nb0.o mlf.o util.o  -lusb-1.0 -o rut

rut.o : rut.c
	$(CC) $(CFLAGS) -c rut.c -I/usr/local/include

nb0.o : nb0.c nb0.h
	$(CC) $(CFLAGS) -c nb0.c

mlf.o : mlf.c mlf.h
	$(CC) $(CFLAGS) -c mlf.c

util.o : util.c util.h
	$(CC) $(CFLAGS) -c util.c



