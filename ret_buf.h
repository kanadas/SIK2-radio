#ifndef _RET_BUF_
#define _RET_BUF_

#include <stdint.h>
#include <netinet/in.h>
#include <pthread.h>

//Thread safe buffor for managing retransmitions
typedef struct {
	uint64_t * buf;
	uint32_t end, size;
	pthread_mutex_t mut;
} retransmit_buf;

extern retransmit_buf retb;

retransmit_buf create_retb(int size);
void retb_append(retransmit_buf* retb, uint64_t elem);
//Allocates buffor of elements to retransmit, not less than lowbnd. Returns lenght of result, deletes returned elements from buffor.
int get_elem_to_ret(retransmit_buf * retb, uint64_t ** buf);
void destroy_retb(retransmit_buf * retb);

#endif //_RET_BUF_

