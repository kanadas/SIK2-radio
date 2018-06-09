#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>

#include "err.h"
#include "config.h"
#include "station_list.h"

#define BUFFER_SIZE   100
#define QUEUE_LENGTH     5

const uint8_t NOP = 241; //No OPeration
const uint8_t AO = 245; //Abort Output
const uint8_t EL = 248; //Erase Line
const uint8_t GA = 249; //Go Ahead
const uint8_t WILL = 251;
const uint8_t WONT = 252;
const uint8_t DO = 253;
const uint8_t DONT = 254;
const uint8_t IAC = 255;

const uint8_t BS = 8; //Back Space
const uint8_t HT = 9; //Horizontal Tab
const uint8_t VT = 11; //Vertical Tab
const uint8_t FF = 12; //Form Feed
const uint8_t LF = 10; //Line Forward
const uint8_t CR = 13; //Carriage Return
const uint8_t ECHO = 1;
const uint8_t LINEMODE = 34;

const uint8_t ESC = 27; //Escape

void nbytestr(char * out, int argc, ...)
{
	va_list arg;
	va_start(arg, argc);
	for(int i = 0; i < argc; ++i){
		*(out + i) = (uint8_t)va_arg(arg, int);
	}
	out[argc] = '\0';
	va_end(arg);
}

void* ui(void* arg)
{
	(void)arg;
	int sock, msg_sock;
	struct sockaddr_in server_address;
	struct sockaddr_in client_address;
	socklen_t client_address_len;

	char buffer[BUFFER_SIZE];
	char * menu;
	ssize_t len, snd_len;

	int port_num = UI_PORT;
	sock = socket(PF_INET, SOCK_STREAM, 0); // creating IPv4 TCP socket
	if (sock < 0)
		syserr("socket");
	// after socket() call; we should close(sock) on any execution path;
	// since all execution paths exit immediately, sock would be closed when program terminates
	server_address.sin_family = AF_INET; // IPv4
	server_address.sin_addr.s_addr = htonl(INADDR_ANY); // listening on all interfaces
	server_address.sin_port = htons(port_num); // listening on port PORT_NUM

	// bind the socket to a concrete address
	if (bind(sock, (struct sockaddr *) &server_address, sizeof(server_address)) < 0)
		syserr("bind");

	// switch to listening (passive open)
	if (listen(sock, QUEUE_LENGTH) < 0)
		syserr("listen");

	//printf("accepting client connections on port %hu\n", ntohs(server_address.sin_port));
	for (;;) {
		client_address_len = sizeof(client_address);
		// get client connection from the socket
		msg_sock = accept(sock, (struct sockaddr *) &client_address, &client_address_len);
		if (msg_sock < 0)
			syserr("accept");
		printf("New Client connected from port no %d and IP %s\n", ntohs(client_address.sin_port), inet_ntoa(client_address.sin_addr));
		//Parameter negotiation
		nbytestr(buffer, 6, IAC, DO, LINEMODE, IAC, WILL, ECHO);
		write(msg_sock, buffer, 6);
		//Hide cursor
		nbytestr(buffer, 6, ESC, '[', '?', '2', '5', 'l');
		write(msg_sock, buffer, 6);
		//print menu
		menu = print_station_list();
		write(msg_sock, menu, strlen(menu));
		free(menu);
		do {
			//read character
			len = read(msg_sock, buffer, sizeof(buffer));
			if (len < 0)
				syserr("reading from client socket");
			else {

					//printf("read from socket: %zd bytes: ", len);
					//for(int asdf = 0; asdf < len; ++asdf) printf("%d ", buffer[asdf]);
					//printf("\n");
				
				//Arrows
				if(buffer[0] == '\033' && buffer[1] == '[') {
					if(buffer[2] == 'A') change_station(-1);
					else if(buffer[2] == 'B') change_station(1);
				}
			}
			//print menu
			menu = print_station_list();
			snd_len = write(msg_sock, menu, strlen(menu));
			if (snd_len != (ssize_t)strlen(menu))
				syserr("writing to client socket");
			free(menu);
		} while (len > 0);
		//Show cursor
		nbytestr(buffer, 6, ESC, '[', '?', '2', '5', 'h');
		write(msg_sock, buffer, 6);
		printf("ending connection\n");
		if (close(msg_sock) < 0)
		  syserr("close");
	}
	return 0;
}
