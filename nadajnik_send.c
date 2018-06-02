#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "nadajnik.h"
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

void nadajnik_send()
{
	int sock = set_socket();
	if (create_circ_fifo(&fifo, FSIZE) != 0)
		syserr("create fifo");
	//TODO setup timer
	
	uint8_t z;
	uint16_t cnt = 0;
	uint8_t buf[PSIZE + 2 * sizeof(uint64_t)];
	audio_package pac;
        pac.audio_data = NULL;
	bytetopac(&pac, NULL, PSIZE);
	pac.session_id = htobe64(time(NULL));
	pac.first_byte_num = htobe64(0);
	while (scanf("%c", (char*)&z) > 0) {
		push_byte(&fifo, z);
		pac.audio_data[cnt++] = z;
		if (cnt == PSIZE) {
			pactobyte(&pac, buf, PSIZE);
			
			//printf("Sending package of size %lu:\n session_id = %lu \n first_byte_num = %lu \n bytes: 0x", sizeof(buf), pac.session_id, pac.first_byte_num);
			//for(size_t i = 0; i < sizeof(buf); ++i) printf("%x", buf[i]); 
			//printf("\n");

			if (write(sock, (void*)buf, sizeof(buf)) != (ssize_t)sizeof(buf))
				syserr("write");
			cnt = 0;
			pac.first_byte_num = htobe64(be64toh(pac.first_byte_num) + 64);
		}
	}
	delete_pac(&pac);
}

