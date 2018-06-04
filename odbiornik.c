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

	/*retb = create_retb(8);
	pthread_t rec_t;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_create(&rec_t, &attr, nadajnik_recv, NULL);
	nadajnik_send();
	pthread_cancel(rec_t);
	destroy_retb(&retb);*/
	return 0;
}
