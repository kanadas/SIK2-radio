TARGETS = sikradio-sender sikradio-receiver
DEPS = nadajnik.h config.h circular_fifo.h err.h io_buffer.h odbiornik.h ret_buf.h station_list.h ret_buf.h
SENDEROBJ = nadajnik.o circular_fifo.o nadajnik_send.o nadajnik_recv.o err.o config.o ret_buf.o
RECEIVEROBJ = odbiornik.o config.o io_buffer.o err.o ret_buf.o station_list.o ui.o

CC      = gcc
CFLAGS  = -Wall -O2 -pthread -lrt -g
LFLAGS  = -Wall -pthread -lrt

all: $(TARGETS)

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

sikradio-sender: $(SENDEROBJ)
	$(CC) -o $@ $^ $(CFLAGS)

sikradio-receiver: $(RECEIVEROBJ)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean TARGET

clean: 
	rm -f $(TARGETS) *.o *~ *.bak

