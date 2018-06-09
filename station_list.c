#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include <stdio.h>

#include "station_list.h"
#include "err.h"

station actual_station;
int station_changed;
static pthread_mutex_t mut;
static station * station_list;
static int slsize;
uint64_t changed = 0;
int station_num;
int actual_station_num;
int is_station = 0;
pthread_mutex_t no_stat_m = PTHREAD_MUTEX_INITIALIZER;

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
	pthread_mutex_lock(&no_stat_m);
}

void statl_found(const station * s)
{
	for(int i = 0; i < station_num; ++i)
		if(station_list[i].port == s->port && station_list[i].addr == s->addr) {
			station_list[i].last_resp = 0;
			return;
		}
	int l = 0;
	while(l < station_num && strcmp(s->name, station_list[l].name) > 0) ++l;
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
	++station_num;
	if(actual_station_num < 0 && (strcmp(NAZWA, "") == 0 || strcmp(NAZWA, station_list[l].name))) {
		actual_station_num = l;
		station_changed = 1;
		actual_station = station_list[l];
		pthread_mutex_unlock(&no_stat_m);
	}
	++changed;
	pthread_mutex_unlock(&mut);
	//printf("Number of stations %d actual_station_num %d station_changed %d\n", station_num, actual_station_num, station_changed);
}

void statl_time()
{
	int shft[station_num + 1];
	int chg = 0;
	shft[0] = 0;
	pthread_mutex_lock(&mut);
	for(int i = 0; i < station_num; ++i) {
		++station_list[i].last_resp;
		shft[i + 1] = shft[i];
		if(station_list[i].last_resp == TICK_TO_EXPIRE) {
			shft[i + 1]++;
			if(i == actual_station_num) {
				chg= 1;
			}
		}
	}
	for(int i = 0; i < station_num; ++i)
		station_list[i - shft[i]]  = station_list[i];
	station_num -= shft[station_num];
	if(chg) {
		if(station_num > 0) {
			actual_station_num = 0;
			actual_station = station_list[actual_station_num];
		}
		else {
			actual_station_num = -1;
			pthread_mutex_lock(&no_stat_m);
		}
		station_changed = 1;
	}
	if(shft[station_num]) changed++;
	pthread_mutex_unlock(&mut);
}

void destroy_station_list()
{
	free(station_list);
	pthread_mutex_destroy(&mut);
	pthread_mutex_destroy(&no_stat_m);
}

//#define CLRSCR "\033[H\033[]"
#define CLRSCR "\033[H\033[2J"

#define LIST_BEGIN "------------------------------------------------------------------------\015\n  SIK Radio\015\n------------------------------------------------------------------------\015\n"

#define LIST_END "------------------------------------------------------------------------\015\n"

char * print_station_list() {
	pthread_mutex_lock(&mut);
	int len = 3 + strlen(CLRSCR) + strlen(LIST_BEGIN) + strlen(LIST_END);
	for(int i = 0; i < station_num; ++i) len += strlen(station_list[i].name) + 6;
	char * lst = (char*)calloc(len, sizeof(char));
	strcpy(lst, CLRSCR);
	strcat(lst, LIST_BEGIN);
	for(int i = 0; i < station_num; ++i) {
		if(i == actual_station_num) strcat(lst, "  > ");
		else strcat(lst, "    ");
		strcat(lst, station_list[i].name);
		strcat(lst, "\015\n");
	}
	strcat(lst, LIST_END);
	pthread_mutex_unlock(&mut);
	return lst;
}

void change_station(int num) {
	if(actual_station_num + num < 0 || actual_station_num + num >= station_num) return;
	actual_station_num += num;
	actual_station = station_list[actual_station_num];
	station_changed = 1;
	++changed;
}

void wait_station()
{
	pthread_mutex_lock(&no_stat_m);
}

void end_wait_station()
{
	pthread_mutex_unlock(&no_stat_m);
}

uint64_t list_changed()
{
	return changed;
}

