#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "ret_buf.h"
#include "err.h"

retransmit_buf retb;

retransmit_buf create_retb(int size) 
{
	retransmit_buf rb;
	rb.end = 0;
	rb.size = size;
	if((rb.buf = (uint64_t *)calloc(size, sizeof(uint64_t))) == NULL)
		syserr("calloc");
	if(pthread_mutex_init(&rb.mut, NULL) != 0)
		syserr("mutex init");
	return rb;
}

void retb_append(retransmit_buf * retb, uint64_t elem)
{
	pthread_mutex_lock(&retb->mut);
	if(retb->end == retb->size) {
		if((retb->buf = (uint64_t *)realloc(retb->buf, sizeof(uint64_t) * retb->size * 2)) == NULL)
			syserr("realloc");
		retb->size *= 2;
	}
	retb->buf[retb->end++] = elem;
	pthread_mutex_unlock(&retb->mut);
}

int get_elem_to_ret(retransmit_buf * retb, uint64_t ** buf)
{
	pthread_mutex_lock(&retb->mut);
	*buf = retb->buf;
	int size = retb->end;
	retb->end = 0;
	retb->size = 8;
	if((retb->buf = (uint64_t *)calloc(retb->size, sizeof(uint64_t))) == NULL)
		syserr("calloc");
	pthread_mutex_unlock(&retb->mut);
	return size;
}

void destroy_retb(retransmit_buf * retb)
{
	free(retb->buf);
	pthread_mutex_destroy(&retb->mut);
}

