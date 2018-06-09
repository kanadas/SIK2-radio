#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "config.h"
#include "io_buffer.h"
#include "err.h"

int create_io_buffer(io_buffer * buffer, uint32_t size)
{
	buffer->begin = buffer->end = 0;
	buffer->size = size;
	buffer->n_holes = 0;
	buffer->holes_size = 4;
	if((buffer->buffer = (uint8_t *)calloc(size, sizeof(uint8_t))) == NULL)
		return 1;
	if((buffer->holes = (uint32_t *)calloc(buffer->holes_size, sizeof(uint32_t))) == NULL)
		return 1;
	return 0;
}

void delete_io_buffer(io_buffer * buffer)
{
	free(buffer->buffer);
	free(buffer->holes);
}

uint32_t get_bytes(io_buffer * buffer, uint8_t * buf, uint32_t num)
{
	if(buffer->n_holes && buffer->first_byte + num > buffer->holes[0]) num = buffer->holes[0] - buffer->first_byte;
	if(num == 0) return 0;
	uint32_t len = buffer->begin + num > buffer->size ? buffer->size - buffer->begin : num;
	memcpy(buf, buffer->buffer + buffer->begin, len);
	if(len < num) memcpy(buf + len, buffer->buffer, num - len);
	buffer->begin += num;
	buffer->first_byte += num;
	/*if(buffer->n_holes && buffer->first_byte == buffer->holes[0]) {
		buffer->first_byte += buffer->buffer[buffer->begin];
		buffer->begin += buffer->buffer[buffer->begin];
		for(uint32_t i = 1; i < buffer->n_holes; ++i)
			buffer->holes[i - 1] = buffer->holes[i];
		--buffer->n_holes;
	}*/
	return num;
}

void push_bytes(io_buffer * buffer, const uint8_t * buf, uint32_t size, uint32_t fbyte_num)
{
	if(fbyte_num < buffer->first_byte) return;
	uint32_t lbyte = buffer->first_byte + buffer_length(buffer);
	uint32_t start = (buffer->begin + fbyte_num - buffer->first_byte) % buffer->size;
	int l = -1;
	if(lbyte > fbyte_num) {
		for(uint32_t i = 0; i < buffer->n_holes; ++i)
			if(buffer->holes[i] == fbyte_num) {
				l = i;
				break;
			}
		if(l < 0) return;
		if(size == buffer->buffer[start]) {
			for(uint32_t i = l + 1; i < buffer->n_holes; ++i)
				buffer->holes[i - 1] = buffer->holes[i];
			--buffer->n_holes;
		} else if(size < buffer->buffer[start]) {
			buffer->holes[l] += size;
			buffer->buffer[start+size] = buffer->buffer[start] - size;
		}
	} else {
		if(lbyte < fbyte_num) {
			if(buffer->n_holes == buffer->holes_size) {
				buffer->holes_size *= 2;
				if((buffer->holes = (uint32_t*)realloc(buffer->holes, buffer->holes_size * sizeof(uint32_t))) == NULL)
					syserr("realloc");
			}
			buffer->holes[buffer->n_holes++] = lbyte;
			buffer->buffer[lbyte] = fbyte_num - lbyte;
		}
		l = 0;
		uint32_t nfb = fbyte_num + size - buffer->size > buffer->first_byte ? fbyte_num + size - buffer->size : buffer->first_byte;
		while((int)buffer->n_holes > l && buffer->holes[l] <= nfb) ++l;
		uint32_t lhp;
		if(l > 0) {
			lhp = (buffer->begin + buffer->holes[l - 1] - buffer->first_byte) % buffer->size;
			if(buffer->buffer[lhp] + buffer->holes[l - 1] > nfb) nfb = buffer->buffer[lhp] + buffer->holes[l - 1];
			for(uint32_t i = l; i < buffer->n_holes; ++i)
				buffer->holes[i - l] = buffer->holes[i];
			buffer->n_holes -= l;
		}
		buffer->begin += nfb - buffer->first_byte;
		buffer->first_byte = nfb;
	}
	uint32_t len = start + size > buffer->size ? buffer->size - start : size;
	memcpy(buffer->buffer + start, buf, len);
	if(len < size) memcpy(buffer->buffer, buf + len, size - len);
	if((buffer->end - start) % buffer->size < size)  
		buffer->end = (start + size) % buffer->size;
}

inline uint32_t buffer_length(const io_buffer * buffer)
{
	return (buffer->end - buffer->begin) % buffer->size;
}

inline uint32_t first_byte_num(const io_buffer * buffer)
{
	return buffer->first_byte;
}

void clear_buffer(io_buffer * buffer)
{
	buffer->begin = buffer->end = buffer->n_holes = 0;
}

uint16_t logg(int64_t x)
{
	int r = 0;
	while(x / 10) ++r;
	return r;
}

char * print_holes(io_buffer * buffer)
{
	if(buffer->n_holes == 0) return 0;
	int len = strlen(REXMIT);
	for(uint32_t i = 0; i < buffer->n_holes; ++i) len += logg(buffer->holes[i]) + 1;
	char * msg;
	if((msg = (char*)calloc(len, sizeof(char))) == NULL)
		syserr("calloc");
	strcpy(msg, REXMIT" ");
	int n = strlen(msg);
	for(uint32_t i = 0; i < buffer->n_holes; ++i) {
		if(i != 0) strcat(msg, ",");
		sprintf(msg + n, "%u", buffer->holes[i]);
		n += logg(buffer->holes[i]);
	}
	return msg;
}
