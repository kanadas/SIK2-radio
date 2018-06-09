#ifndef _STATION_LIST_
#define _STATION_LIST_

#include <stdint.h>
#include <netinet/in.h>
#include <pthread.h>

#include "config.h"

#define TICK_TO_EXPIRE 4
#define LOOKUP_TIME_MS 5000

//Thread safe list for managing stations
typedef struct {
	char name[MAX_NAME_LENGTH];
	uint16_t port;
	uint32_t addr;
	uint8_t last_resp;
} station;

extern station actual_station;
extern int station_changed;
extern int is_station;

void init_station_list();
void statl_found(const station * s);
void statl_time();
void destroy_station_list();
char * print_station_list();
void change_station(int num);
void wait_station();
void end_wait_station();

#endif //_STATION_LIST_

