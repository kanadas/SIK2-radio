#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "odbiornik.h"
#include "io_buffer.h"

int parse_flags(int argc, char *argv[])
{
	strcpy(NAZWA, "");
	struct sockaddr_in addr;
	int l = 1;
	while (l < argc) {
		if (strcmp(argv[l], "-d") == 0) {
			if (inet_aton(argv[l + 1], &addr.sin_addr) == 0) {
				printf("Wrong address format\n");
				return 1;
			}
			DISCOVER_ADDR = addr.sin_addr.s_addr;
		} 
		else if (strcmp(argv[l], "-U") == 0) {
			UI_PORT = atoi(argv[l + 1]);
			if(UI_PORT > 65535) {
			       printf("Wrong port number\n");
			       return 1;
			}
		} 
		else if (strcmp(argv[l], "-C") == 0) {
			CTRL_PORT = atoi(argv[l + 1]);
			if(CTRL_PORT > 65535) {
			       printf("Wrong port number\n");
		       	       return 1;
			}
		} 
		else if (strcmp(argv[l], "-b") == 0) {
			BSIZE = atoi(argv[l + 1]);
		} 
		else if (strcmp(argv[l], "-R") == 0) {
			RTIME = atoi(argv[l + 1]);
		}
		else if (strcmp(argv[l], "-n") == 0) {
			if(strlen(argv[l + 1]) > MAX_NAME_LENGTH) {
				printf("Too long name");
				return 1;
			}
			strcpy(NAZWA, argv[l + 1]);
		} else {
			printf("Unknown option %s\n", argv[l]);
			return 1;
		}
		l += 2;
	}	
	if(MCAST_ADDR == 0) {
		printf("You have to specify transmition address (-a flag)\n");
		return 1;
	}
	return 0;
}

io_buffer buffer;
int buffer_waiting = 1;
pthread_mutex_t buf_mut;
int is_station = 0;

int init_socket()
{
	int sock;
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		syserr("socket");
	return sock;
}

void set_socket(int * sock, uint32_t addr, uint16_t port)
{
	/* podpięcie się do grupy rozsyłania (ang. multicast) */
	struct ip_mreq ip_mreq;
	struct sockaddr_in saddr;
	ip_mreq.imr_interface.s_addr = htonl(INADDR_ANY);
	ip_mreq.imr_multiaddr.s_addr = addr;
	if(setsockopt(*sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void*)&ip_mreq, sizeof ip_mreq) < 0)
	  syserr("setsockopt");

	/* podpięcie się pod lokalny adres i port */
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = htonl(INADDR_ANY);
	saddr.sin_port = htons(port);
	if (bind(*sock, (struct sockaddr *)&saddr, sizeof saddr) < 0)
	  syserr("bind");
}

void reset_record(int * sock)
{
	pthread_mutex_lock(&buf_mut);
	buffer_waiting = 1;
	set_socket(sock, actual_station.addr, actual_station.port);
}

void listen()
{
	int sock = init_socket();
	while(1) {
		//TODO change this 2 if's to some mutex
		if(is_station) {
			if(station_changed) reset_record(&sock);
			//TODO listening
		}
	}
}


int main (int argc, char *argv[])
{
	if(parse_flags(argc, argv)) {
		printf("Usage: %s [flags]\n"
			"Supported flags: \n"
			"	-d discover address (default 255.255.255.255) \n"
			"	-U UI port (default 15674) \n"
			"	-C controll port (default 35674) \n"
			"	-b buffor size [B] (default 64kB) \n"
			"	-R retransmition time [ms] (default 250) \n"
			"	-n default station name (no default) \n", argv[0]);
		return 1;
	}
	//Potrzebuję 3 wątki: Jeden pobiera dane do bufora, drugi je wypisuje, trzeci odpowiada za UI.
	//Do tego timer odpalający się co LOOKUP_TIME sekund aktualizujący listę stacji
	//Thread safe lista stacji już jest, potrzebny jeszcze globalny bufor (bardzo prosty wektor).
	if(create_io_buffer(&buffer, BSIZE) != 0)
		syserr("create buffer");
	

	/*retb = create_retb(8);
	pthread_t rec_t;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_create(&rec_t, &attr, nadajnik_recv, NULL);
	nadajnik_send();
	pthread_cancel(rec_t);
	destroy_retb(&retb);*/
	delete_io_buffer(&buffer);
	return 0;
}
