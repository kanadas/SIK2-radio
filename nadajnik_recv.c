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

static int set_socket()
{
        int sock;
        struct sockaddr_in addr;
        if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
                syserr("socket");

        //int optval = 1;
	//if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (void*)&optval, sizeof optval) < 0)
	//  syserr("setsockopt broadcast");

	memset(&addr, 0, sizeof addr);
        addr.sin_family = AF_INET;
        addr.sin_port = htons(CTRL_PORT);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        if (bind(sock, (struct sockaddr *)&addr, sizeof addr) < 0)
                syserr("bind");
        return sock;
}

void cleanup(void * sock)
{
	close(*(int*)sock);
}

void* nadajnik_recv(void* arg)
{
	(void)arg;
	int sock = set_socket();
	pthread_cleanup_push(cleanup, (void*)&sock);
	struct sockaddr_in caddr;
	socklen_t rcva_len = sizeof(struct sockaddr_in);
	int32_t len;
	char msg[MAX_UDP_PACKET_SIZE];
	struct in_addr tmp_mc_addr;
	tmp_mc_addr.s_addr = MCAST_ADDR;
	char mc_addr[16];
	strcpy(mc_addr, inet_ntoa(tmp_mc_addr));
	while(1) {
		len = recvfrom(sock, msg, sizeof(msg), 0,
				(struct sockaddr *) &caddr, &rcva_len);

		//printf("Got message %s of length %d, from %d : %d of length %d\n", msg, len, caddr.sin_addr.s_addr, caddr.sin_port, rcva_len);

		if (len < 0) 
			syserr("error on datagram from client socket");
		msg[len] = '\0';
		if (strncmp(msg, LOOKUP, strlen(LOOKUP)) == 0) {
			sprintf(msg, "%s %s %d %s", REPLY, mc_addr, DATA_PORT, NAZWA);

		//	printf("Sending %s to %s on port %d\n", msg, inet_ntoa(caddr.sin_addr), caddr.sin_port);

			if (sendto(sock, msg, strlen(msg), 0, (struct sockaddr *) &caddr, 
						rcva_len) != (ssize_t)strlen(msg))
				syserr("sendto"); 
		}
		else if	(strncmp(msg, REXMIT, strlen(REXMIT)) == 0) {
			char *tmp = msg + strlen(REXMIT) + 1;
			char *tmp2;
			uint64_t num;

			//printf("Got rexmit %s\n", msg);

			while(*tmp != '\0') {
				num = strtoul(tmp, &tmp2, 10);
				if (tmp2 == tmp)
					break;
				retb_append(&retb, num);

				//printf("Appended package %lu to retransmit\n", num);

				if(*tmp2 == '\0')
					break;
				tmp = tmp2 + 1;
			}
		}
	}
	pthread_cleanup_pop(1);
	return NULL;
}

