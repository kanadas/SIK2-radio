#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "err.h"
#include "config.h"

static int set_socket()
{
	int sock, optval;
	struct sockaddr_in addr;
	if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		syserr("socket");
}

void send()
{

}

