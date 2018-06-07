#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "config.h"
#include "err.h"

//Setting default values
uint32_t MCAST_ADDR = 0;
uint32_t DISCOVER_ADDR = ~0; //Broadcast
uint16_t DATA_PORT = 25674;
uint16_t CTRL_PORT = 35674;
uint16_t UI_PORT = 15674;
uint32_t PSIZE = 512;
uint32_t BSIZE = 64 << 10;
uint32_t FSIZE = 128 << 10;
uint32_t RTIME = 250;
char NAZWA[MAX_NAME_LENGTH] = "Nienazwany Nadajnik";

void bytetopac(audio_package * pac, const uint8_t * buf, uint32_t size) 
{
	if(pac->audio_data == NULL) {
		if((pac->audio_data = (uint8_t *)calloc(size, sizeof(uint8_t))) == NULL)
			syserr("calloc");
	}
	if(buf != NULL) {
		memcpy(&pac->session_id, buf, sizeof(uint64_t));
		memcpy(&pac->first_byte_num, buf + sizeof(uint64_t), sizeof(uint64_t));
		memcpy(pac->audio_data, buf + 2 * sizeof(uint64_t), size);
	}
}

void pactobyte(const audio_package *pac, uint8_t * buf, uint32_t size) 
{
	memcpy(buf, &pac->session_id, sizeof(uint64_t));
	memcpy(buf + sizeof(uint64_t), &pac->first_byte_num, sizeof(uint64_t));
	memcpy(buf + 2 * sizeof(uint64_t), pac->audio_data, size);
}

void delete_pac(audio_package * pac) 
{
	free(pac->audio_data);
}

