#include <stdlib.h>
#include <string.h>
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
/*	for(uint32_t i = 0; i < num; ++i)
		buffer[i] = fifo->buffer[(fifo->begin + offset + i) % fifo->size]; */
	uint32_t start = (fifo->begin + offset) % fifo->size;
	uint32_t len = start + num > fifo->size ? fifo->size - start : num;
	memcpy(buffer, fifo->buffer + start, len);
	if(len < num) memcpy(buffer + len, fifo->buffer, num - len);
}

int pop_bytes(circular_fifo * fifo, uint8_t * buffer, uint32_t num)
{
	if(fifo_length(fifo) < num) num = fifo_length(fifo);
	uint32_t len = fifo->begin + num > fifo->size ? fifo->size - fifo->begin : num;
	memcpy(buffer, fifo->buffer + fifo->begin, len);
	if(len < num) memcpy(buffer + len, fifo->buffer, num - len);
	fifo->begin = (fifo->begin + num) % fifo->size;
	return num;
}

void push_byte(circular_fifo * fifo, uint8_t byte)
{
	fifo->buffer[fifo->end] = byte;
	fifo->end = (fifo->end + 1) % fifo->size;
	if(fifo->begin == fifo->end) ++fifo->begin;
}

void push_bytes(circular_fifo * fifo, const uint8_t * buf, uint32_t size, uint32_t pos)
{
	uint32_t start = (fifo->begin + pos) % fifo->size;
	uint32_t len = start + size > fifo->size ? fifo->size - start : size;
	memcpy(fifo->buffer + start, buf, len);
	if(len < size) memcpy(fifo->buffer, buf + len, size - len);
	if((fifo->end - start) % fifo->size < size)  
		fifo->end = (pos + size) % fifo->size;
}

inline uint32_t fifo_length(const circular_fifo * fifo)
{
	return (fifo->end - fifo->begin) % fifo->size;
}
