#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "err.h"
#include "config.h"
#include "circular_fifo.h"

circular_fifo fifo;

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
	addr.sin_family = AF_INET;
	addr.sin_port = htons(DATA_PORT);
	addr.sin_addr.s_addr = MCAST_ADDR;
	if (connect(sock, (struct sockaddr *)&addr, sizeof addr) < 0)
		syserr("connect");
	return sock;
}

void send()
{
	int sock = set_socket();
	if (create_circ_fifo(&fifo, FSIZE) != 0)
		syserr("create fifo");
	//TODO setup timer
	
	uint8_t z;
	uint16_t cnt = 0;
	struct audio_package pac;
	pac.session_id = htobe64(time(NULL));
	pac.first_byte_num = htobe64(0);
	while (scanf("%c", (char*)&z) == 0) {
		push_byte(&fifo, z);
		pac.audio_data[cnt++] = z;
		if (cnt == PSIZE) {
			if (write(sock, (void*)&pac, sizeof(pac)) != sizeof(pac))
				syserr("write");
			cnt = 0;
			pac.first_byte_num = htobe64(be64toh(pac.first_byte_num) + 64);
		}
	}
}

