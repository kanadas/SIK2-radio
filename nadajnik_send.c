#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>

#include "nadajnik.h"
#include "circular_fifo.h"

circular_fifo fifo;
pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
uint64_t last_byte = 0;
uint64_t session_id;
int sock;

static int set_socket()
{
	int sock, optval;
	struct sockaddr_in addr;
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		syserr("socket");
	optval = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &optval, sizeof optval) < 0)
		syserr("setsockopt broadcast");
	optval = TTL_VALUE;
	if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, &optval, sizeof optval) < 0)
		syserr("setsockopt multicast ttl");
	optval = 0;
	if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, &optval, sizeof optval) < 0)
		syserr("setsockopt multicast loop");
	addr.sin_family = AF_INET;
	addr.sin_port = htons(DATA_PORT);
	addr.sin_addr.s_addr = MCAST_ADDR;
	if (connect(sock, (struct sockaddr *)&addr, sizeof addr) < 0)
		syserr("connect");
	return sock;
}

int comp(const void * a, const void * b)
{
	uint64_t _a = *(uint64_t *) a;
	uint64_t _b = *(uint64_t *) b;
	if(_a < _b) return -1;
	if(_a > _b) return 1;
	return 0;
}

void retransmit(union sigval arg)
{
	(void)arg;
	uint64_t * buf;
	uint32_t n = get_elem_to_ret(&retb, &buf);
	uint32_t l = 0;
	uint8_t buffer[PSIZE + 2 * sizeof(uint64_t)];
	audio_package pac;
	pac.audio_data = NULL;
	bytetopac(&pac, NULL, PSIZE);
	pac.session_id = session_id;
	qsort(buf, n, sizeof(uint64_t), comp);
	pthread_mutex_lock(&mut);
	uint64_t fb = last_byte > FSIZE ? last_byte - FSIZE : 0;
	while(buf[l] < fb) ++l;
	while(l < n) {
		while(l < n - 1 && buf[l] == buf[l + 1]) ++l;
		if(buf[l] % PSIZE == 0) {
			pac.first_byte_num = buf[l];
			get_bytes(&fifo, pac.audio_data, buf[l] - fb, PSIZE);
			pactobyte(&pac, buffer, PSIZE);
			if (write(sock, (void*)buffer, sizeof(buffer)) != (ssize_t)sizeof(buffer))
				syserr("write");
		}
		++l;
	}
	pthread_mutex_unlock(&mut);
	delete_pac(&pac);
	free(buf);
}
void nadajnik_send()
{
	sock = set_socket();
	if (create_circ_fifo(&fifo, FSIZE) != 0)
		syserr("create fifo");
	uint8_t z;
	uint16_t cnt = 0;
	uint8_t buf[PSIZE + 2 * sizeof(uint64_t)];
	struct sigevent sigev;
	sigev.sigev_notify = SIGEV_THREAD;
	sigev.sigev_value.sival_int = 0;
	sigev.sigev_notify_function = retransmit;
	sigev.sigev_notify_attributes = NULL;
	timer_t timerid;
	if(timer_create(CLOCK_REALTIME, &sigev, &timerid) != 0)
		syserr("timer create");
	struct itimerspec its;
	its.it_value.tv_sec = 1; //Starting in 1 second
	its.it_value.tv_nsec = 0;
	its.it_interval.tv_sec = RTIME / 1000;
	its.it_interval.tv_nsec = RTIME % 1000 * 1000000;
	if(timer_settime(timerid, 0, &its, NULL) != 0)
		syserr("timer settime");
	audio_package pac;
        pac.audio_data = NULL;
	bytetopac(&pac, NULL, PSIZE);
	session_id = htobe64(time(NULL));
	pac.session_id = session_id;
	pac.first_byte_num = htobe64(last_byte);
	while (scanf("%c", (char*)&z) > 0) {
		pthread_mutex_lock(&mut);
		push_byte(&fifo, z);
		++last_byte;
		pthread_mutex_unlock(&mut);
		pac.audio_data[cnt++] = z;
		if (cnt == PSIZE) {
			pactobyte(&pac, buf, PSIZE);
			
			//printf("Sending package of size %lu:\n session_id = %lu \n first_byte_num = %lu \n bytes: 0x", sizeof(buf), pac.session_id, pac.first_byte_num);
			//for(size_t i = 0; i < sizeof(buf); ++i) printf("%x", buf[i]); 
			//printf("\n");

			if (write(sock, (void*)buf, sizeof(buf)) != (ssize_t)sizeof(buf))
				syserr("write");
			cnt = 0;
			pac.first_byte_num = htobe64(last_byte);
		}
	}
	timer_delete(timerid);
	pthread_mutex_destroy(&mut);
	delete_pac(&pac);
	close(sock);
}


