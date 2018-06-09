#ifndef _IO_BUFFER_
#define _IO_BUFFER_

#include <stdint.h>

typedef struct {
	uint32_t begin, end, size, first_byte;
	uint8_t * buffer;
	uint32_t * holes;
	uint32_t n_holes, holes_size;
} io_buffer;

//Carefull - it alocates space, don't forget to delete
int create_io_buffer(io_buffer * buffer, uint32_t size);

uint32_t get_bytes(io_buffer * buffer, uint8_t * buf, uint32_t num);

//Automatically overrides old data when lacking space
void push_bytes(io_buffer * buffer, const uint8_t * buf, uint32_t size, uint32_t fbyte_num);

uint32_t buffer_length(const io_buffer * buffer);

uint32_t first_byte_num(const io_buffer * buffer);

void clear_buffer(io_buffer * buffer);

char * print_holes(io_buffer * buffer);

void delete_io_buffer(io_buffer * buffer);

#endif //_IO_BUFFER_
