#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <errno.h>
#include <sys/time.h>

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
	return 0;
}

io_buffer buffer;
int buffer_waiting = 0;
pthread_mutex_t buf_mut = PTHREAD_MUTEX_INITIALIZER;

int init_socket()
{
	int sock;
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		syserr("socket");
	return sock;
}

void set_listen_socket(int * sock, uint32_t addr, uint16_t port, uint64_t timeout)
{
	/* podpięcie się do grupy rozsyłania (ang. multicast) */
	struct ip_mreq ip_mreq;
	struct sockaddr_in saddr;
	ip_mreq.imr_interface.s_addr = htonl(INADDR_ANY);
	ip_mreq.imr_multiaddr.s_addr = addr;
	if(setsockopt(*sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void*)&ip_mreq, sizeof ip_mreq) < 0)
		if(errno != 22) syserr("setsockopt ip add membership");

	struct timeval tv;
	tv.tv_sec = timeout / 1000;
	tv.tv_usec = (timeout % 1000) * 1000;
	if(setsockopt(*sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv)) < 0)
		syserr("setsockopt so rcvtimeo");

	/* podpięcie się pod lokalny adres i port */
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = htonl(INADDR_ANY);
	saddr.sin_port = htons(port);
	if (bind(*sock, (struct sockaddr *)&saddr, sizeof saddr) < 0)
	  syserr("bind");
}

void set_control_socket(int * sock, uint64_t timeout)
{
	int optval = 1;
	if (setsockopt(*sock, SOL_SOCKET, SO_BROADCAST, (void*)&optval, sizeof optval) < 0)
	  syserr("setsockopt broadcast");

	/* ustawienie TTL dla datagramów rozsyłanych do grupy */ 
	optval = TTL_VALUE;
	if (setsockopt(*sock, IPPROTO_IP, IP_MULTICAST_TTL, (void*)&optval, sizeof optval) < 0)
	  syserr("setsockopt multicast ttl");

	optval = 1;
	if (setsockopt(*sock, SOL_IP, IP_MULTICAST_LOOP, (void*)&optval, sizeof optval) < 0)
	  syserr("setsockopt loop");
	
	struct timeval tv;
	tv.tv_sec = timeout / 1000;
	tv.tv_usec = (timeout % 1000) * 1000;
	if(setsockopt(*sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv)) < 0)
		syserr("setsockopt so rcvtimeo");
	
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	//Not sure of this
	addr.sin_port = htons(0);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(*sock, (struct sockaddr *)&addr, sizeof addr) < 0)
		syserr("bind");
}

void reset_record(int * sock, uint64_t * session_id)
{
	if(buffer_waiting == 0) pthread_mutex_lock(&buf_mut);
	buffer_waiting = 1;
	*session_id = 0;
	clear_buffer(&buffer);
	wait_station();
	close(*sock);
	*sock = init_socket();
	station_changed = 0;
	set_listen_socket(sock, actual_station.addr, actual_station.port, RECEIVER_TIMEOUT_MS);
	end_wait_station();

	//printf("Now listening on %d : %d\n", actual_station.addr, actual_station.port);
}

void radio_listen()
{
	int sock = init_socket();
	int32_t len;
	uint8_t buf[MAX_UDP_PACKET_SIZE];
	uint64_t session_id = 0;
	audio_package pac;
	pac.audio_data = NULL;
	bytetopac(&pac, NULL, MAX_UDP_PACKET_SIZE);
	reset_record(&sock, &session_id);

	//printf("listening\n");

	while(1) {
		if(station_changed) {
		       	reset_record(&sock, &session_id);
		}
		if((len = read(sock, buf, sizeof buf)) < 0) {
			if(errno != EAGAIN && errno != EWOULDBLOCK)
			       syserr("read");
		} else {
			bytetopac(&pac, buf, len);
			pac.first_byte_num = be64toh(pac.first_byte_num);
			pac.session_id = be64toh(pac.session_id);
			//printf("got package session %lu number %lu\n", pac.session_id, pac.first_byte_num);
			if(session_id == 0) session_id = pac.session_id;
			if(session_id < pac.session_id) reset_record(&sock, &session_id);
			else if(session_id == pac.session_id) {
				if(buffer_waiting == 0) pthread_mutex_lock(&buf_mut);
				push_bytes(&buffer, pac.audio_data, len - 2 * sizeof(uint64_t), pac.first_byte_num);

				//printf("Buffer size: %u full %u%%\n", buffer_length(&buffer), buffer_length(&buffer) * 100 / BSIZE);
				if(buffer_waiting == 1 && buffer_length(&buffer) >= BSIZE*3/4) {
					buffer_waiting = 0;
				}
				if(buffer_waiting == 0) pthread_mutex_unlock(&buf_mut);
			}
		}
	}
	delete_pac(&pac);
	close(sock);
}

void* print_data(void * arg)
{
	(void)arg;
	uint8_t buf[WRITE_CHUNK_SIZE];
	uint32_t len;
	while(1) {
		pthread_mutex_lock(&buf_mut);
		if((len = get_bytes(&buffer, buf, WRITE_CHUNK_SIZE)) > 0) {

			//printf("About to write %u bytes\n", len);

			if(write(1, buf, len) != len) 
				syserr("write");
		} else {
			//No data to write - reseting writing
			clear_buffer(&buffer);
			station_changed = 1;
		}
		pthread_mutex_unlock(&buf_mut);
	}
	return NULL;
}

//in miliseconds
int64_t delta_time(struct timeval from)
{
	struct timeval to;
	gettimeofday(&to, NULL);
	//TODO change 1e9 to 1e6 and 1e6 to 1e3
	return ((to.tv_sec - from.tv_sec)*1e9 + to.tv_usec - from.tv_usec)/1e6;
}

void * refresh_stations(void * arg)
{
	int * sock = (int *) arg;
	struct timeval time;
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_addr.s_addr = DISCOVER_ADDR;
	addr.sin_port = htons(CTRL_PORT);
	char msg[REPLY_MSG_SIZE];
	char ipaddr[16];
	memset(ipaddr, 0, sizeof(ipaddr));
	station s;
	memset(&s, 0, sizeof(s));
	int len;
	struct in_addr inaddr;
	while(1) {
		gettimeofday(&time, NULL);
		if(sendto(*sock, LOOKUP, strlen(LOOKUP), 0, (struct sockaddr *) &addr, sizeof(struct sockaddr_in)) != strlen(LOOKUP))
			syserr("sendto");
		while(delta_time(time) < LOOKUP_TIME_MS) {
			if((len = read(*sock, msg, sizeof(msg))) < 0) {
				if(errno == EAGAIN || errno == EWOULDBLOCK) 
					continue;
				else syserr("read");
			}
			msg[len] = '\0';

			//printf("Got response %s\n", msg);
			memset(s.name, 0, sizeof(s.name));
			sscanf(msg, REPLY" %s %hu %64c", ipaddr, &s.port, s.name);

			//printf("addr = %s port = %hu name =  %s\n", ipaddr, s.port, s.name);
			
			if(inet_aton(ipaddr, &inaddr) != 0) {
				s.addr = inaddr.s_addr;
				statl_found(&s);
			}
		}
		statl_time();
	}
	return NULL;
}

void request_retransmit(union sigval arg)
{
	int * sock = (int *) arg.sival_ptr;
	char * holes = print_holes(&buffer);
	uint32_t n;
	if(holes != NULL) {
		n = strlen(holes);
		struct sockaddr_in addr;
		addr.sin_addr.s_addr = DISCOVER_ADDR;
		addr.sin_port = CTRL_PORT;
		if(sendto(*sock, holes, n, 0, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) != n)
			syserr("sendto");
		free(holes);
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
	init_station_list();
	int ctrl_sock = init_socket();
	set_control_socket(&ctrl_sock, RECEIVER_TIMEOUT_MS);

	pthread_t print_t, refresh_t, ui_t;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_create(&print_t, &attr, print_data, NULL);
	pthread_create(&refresh_t, &attr, refresh_stations, (void*)&ctrl_sock);
	pthread_create(&ui_t, &attr, ui, NULL);
        struct sigevent sigev;
	sigev.sigev_notify = SIGEV_THREAD;
	sigev.sigev_value.sival_int = 0;
	sigev.sigev_notify_function = request_retransmit;
	sigev.sigev_notify_attributes = NULL;
	sigev.sigev_value.sival_ptr = (void*)&ctrl_sock;
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
	radio_listen();
	//pthread_cancel(print_t);
	//pthread_cancel(refresh_t);
	//pthread_cancel(ui_t);
	//timer_delete(timerid);
	//pthread_attr_destroy(&attr);
	delete_io_buffer(&buffer);
	destroy_station_list();
	pthread_mutex_destroy(&buf_mut);
	close(ctrl_sock);
	return 0;
}
