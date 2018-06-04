#ifndef _STATION_LIST_
#define _STATION_LIST_

#include <stdint.h>
#include <netinet/in.h>
#include <pthread.h>

#include "config.h"

#define TICK_TO_EXPIRE 4
#define LOOKUP_TIME 5

//Thread safe list for managing stations
typedef struct {
	char name[MAX_NAME_LENGTH];
	uint16_t port;
	uint32_t addr;
	uint8_t last_resp;
} station;

extern station actual_station;
extern int station_changed;

void init_station_list();
void statl_found(const station * s);
void statl_time();
void destroy_station_list();
char * print_station_list();
void change_station(int num);

#endif //_STATION_LIST_

