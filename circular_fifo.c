#include <stdlib.h>
#include "circular_fifo.h"

#include <stdio.h>

int create_circ_fifo(circular_fifo * fifo, uint32_t size)
{
	fifo->begin = fifo->end = 0;
	fifo->size = size;
	if((fifo->buffer = (uint8_t *)calloc(size, sizeof(uint8_t))) == NULL)
		return 1;
	return 0;
}

void delete_circ_fifo(circular_fifo * fifo)
{
	free(fifo->buffer);
}

void get_bytes(const circular_fifo * fifo, uint8_t * buffer, uint32_t offset, uint32_t num)
{
	for(uint32_t i = 0; i < num; ++i)
		buffer[i] = fifo->buffer[(fifo->begin + offset + i) % fifo->size];
}

void push_byte(circular_fifo * fifo, uint8_t byte)
{
	fifo->buffer[fifo->end] = byte;
	fifo->end = (fifo->end + 1) % fifo->size;
	if(fifo->begin == fifo->end) ++fifo->begin;
}

