#ifndef _CIRCULAR_FIFO_
#define _CIRCULAR_FIFO_

#include <stdint.h>

// Very basic circular fifo
typedef struct {
	uint32_t begin, end, size;
	uint8_t * buffer;
} circular_fifo;

//Carefull - it alocates space, don't forget to delete
int create_circ_fifo(circular_fifo * fifo, uint32_t size);

//Doesn't check arguments - you can get more data than it holds (it will be duplicated)
void get_bytes(const circular_fifo * fifo, uint8_t * buffer, uint32_t offset, uint32_t num);

//Returns amount of actually returned bytes <= num
int pop_bytes(const circular_fifo * fifo, uint8_t * buffer, uint32_t num);

//Automatically overrides old data when lacking space
void push_byte(circular_fifo * fifo, uint8_t byte);

void insert_bytes(circular_fifo * fifo, const uint8_t * buf, uint32_t size, uint32_t pos);

inline uint32_t fifo_length(const circular_fifo * fifo);

void delete_circ_fifo(circular_fifo * fifo);

#endif //_CIRCULAR_FIFO_
