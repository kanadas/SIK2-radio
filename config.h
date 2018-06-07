#ifndef _RADIO_CONFIG_
#define _RADIO_CONFIG_

#include <stdint.h>
#include <netinet/in.h>
#include <pthread.h>

//-------------PARAMETERS--------------

#define MAX_NAME_LENGTH 64
#define TTL_VALUE 4
#define MAX_UDP_PACKET_SIZE 65527
#define RECEIVER_TIMEOUT_MS 100

//Flag settable parameters
extern uint32_t MCAST_ADDR;
extern uint32_t DISCOVER_ADDR;
extern uint16_t DATA_PORT;
extern uint16_t CTRL_PORT;
extern uint16_t UI_PORT;
extern uint32_t PSIZE;
extern uint32_t BSIZE;
extern uint32_t FSIZE;
extern uint32_t RTIME;
extern char NAZWA[MAX_NAME_LENGTH];

//-----------PROTOCOL---------------

//Format of audio data transmitions
typedef struct {
	uint64_t session_id;
	uint64_t first_byte_num;
	uint8_t * audio_data;
} audio_package;

//If pac->audio_data is NULL then allocates size elements to it
void bytetopac(audio_package * pac, const uint8_t * buf, uint32_t size);
void pactobyte(const audio_package * pac, uint8_t * buf, uint32_t size);
void delete_pac(audio_package * pac);

//Messages
#define LOOKUP "ZERO_SEVEN_COME_IN"
#define REPLY "BOREWICZ_HERE"
#define REXMIT "LOUDER_PLEASE"

#endif //_RADIO_CONFIG_

