#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "station_list.h"
#include "err.h"

station actual_station;
int station_changed;
static pthread_mutex_t mut;
static station * station_list;
static int slsize;
int station_num;
int actual_station_num;

void init_station_list() 
{
	station_num = 0;
	slsize = 2;
	actual_station_num = -1;
	station_changed = 0;
	if((station_list = (station *)calloc(slsize, sizeof(station))) == NULL)
		syserr("calloc");
	if(pthread_mutex_init(&mut, NULL) != 0)
		syserr("mutex init");
}

void statl_found(const station * s)
{
	for(int i = 0; i < station_num; ++i)
		if(station_list[i].port == s->port && station_list[i].addr == s->addr) {
			station_list[i].last_resp = 0;
			return;
		}
	int l = 0;
	while(strcmp(s->name, station_list[l].name) > 0) ++l;
	pthread_mutex_lock(&mut);
	if(l <= actual_station_num) ++actual_station_num;
	if(slsize == station_num) {
		slsize *= 2;
		if((station_list = (station *)realloc(station_list, sizeof(station) * slsize)) == NULL)
			syserr("realloc");
	}
	for(int i = station_num - 1; i >= l; --i)
		station_list[i + 1] = station_list[i];
	memcpy(&station_list[l], s, sizeof(station));
	station_list[l].last_resp = 0;
	pthread_mutex_unlock(&mut);
	if(actual_station_num < 0 && (strcmp(NAZWA, "") == 0 || strcmp(NAZWA, station_list[l].name))) {
		actual_station_num = l;
		station_changed = 1;
		actual_station = station_list[l];
	}
}

void statl_time()
{
	int shft[station_num];
	shft[0] = 0;
	pthread_mutex_lock(&mut);
	for(int i = 0; i < station_num; ++i) {
		++station_list[i].last_resp;
		shft[i + 1] = shft[i];
		if(station_list[i].last_resp == TICK_TO_EXPIRE) {
			shft[i + 1]++;
			if(i == actual_station_num) {
				actual_station_num = 0;
				station_changed = 1;
			}
		}
	}
	for(int i = 0; i < station_num; ++i)
		station_list[i - shft[i]]  = station_list[i];
	if(station_changed) actual_station = station_list[actual_station_num];
	pthread_mutex_unlock(&mut);
}

void destroy_station_list()
{
	free(station_list);
	pthread_mutex_destroy(&mut);
}

#define LIST_BEGIN "------------------------------------------------------------------------\n  SIK Radio\n------------------------------------------------------------------------\n"

#define LIST_END "------------------------------------------------------------------------\n"

char * print_station_list() {
	pthread_mutex_lock(&mut);
	int len = strlen(LIST_BEGIN) + strlen(LIST_END);
	for(int i = 0; i < station_num; ++i) len += strlen(station_list[i].name) + 5;
	char * lst = (char*)calloc(len, sizeof(char));
	strcpy(lst, LIST_BEGIN);
	for(int i = 0; i < station_num; ++i) {
		if(i == actual_station_num) strcat(lst, "  > ");
		strcat(lst, "    ");
		strcat(lst, station_list[i].name);
		strcat(lst, "\n");
	}
	strcat(lst, LIST_END);
	pthread_mutex_unlock(&mut);
	return lst;
}

void change_station(int num) {
	actual_station_num += num;
	actual_station = station_list[actual_station_num];
	station_changed = 1;
}

