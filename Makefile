TARGETS = nadajnik
DEPS = nadajnik.h config.h circular_fifo.h err.h
OBJ = nadajnik.o circular_fifo.o nadajnik_send.o err.o config.o

CC      = gcc
CFLAGS  = -Wall -O2
LFLAGS  = -Wall

all: $(TARGETS)

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

nadajnik: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean TARGET

clean: 
	rm -f $(TARGETS) *.o *~ *.bak

