#ifndef _RADIO_CONFIG_
#define _RADIO_CONFIG_

#include <stdint.h>
#include <netinet/in.h>

//-------------PARAMETERS--------------

const uint16_t MAX_NAME_LENGTH = 64;

//Flag settable parameters
int32_t MCAST_ADDR;
int32_t DISCOVER_ADDR = ~0; //Broadcast
uint16_t DATA_PORT = 25674;
uint16_t CTRL_PORT = 35674;
uint16_t UI_PORT = 15674;
uint32_t PSIZE = 512;
uint32_t BSIZE = 64 << 10;
uint32_t FSIZE = 128 << 10;
uint32_t RTIME = 250;
char NAZWA[MAX_NAME_LENGTH] = "Nienazwany Nadajnik";

//-----------PROTOCOL---------------

//Format of audio data transmitions
struct audio_package {
	uint64_t session_id;
	uint64_t first_byte_num;
	int8_t audio_data[];
};

//Messages
#define LOOKUP "ZERO_SEVEN_COME_IN"
#define REPLY "BOREWICZ_HERE"
#define REXMIT "LOUDER_PLEASE"

#endif //_RADIO_CONFIG_

