#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <endian.h>

#include <pevulib.h>

#define BPM_COUNT			32
#define NUMBER_OF_CHANNELS	12
#define BASE_CHANNEL_OFFSET 0x100
#define EVENT_ID            0x40
#define REGISTER_OPERATION_WRITE	0x0080000000000000ULL	// b56 == "w/!r"
#define REGISTER_OPERATION_READ		0x0000000000000000ULL	// b56 == "w/!r"
#define REGISTER_LINK_PS_SHIFT		46

// Registers IDs in the memory map from PEV.
// These registeres are transferred in the memory map.
enum
{
	REGISTER_PRIORITY_WRITE	= 0,
	REGISTER_NORMAL_WRITE,
	REGISTER_NORMAL_READ,
	REGISTER_WAVEFORM_WRITE,
	REGISTER_WAVEFORM_READ,
	NUMBER_OF_IO_REGISTERS,	/*Reserved*/
	REGISTER_MSR,
	REGISTER_TSR,
	NUMBER_OF_REGISTERS
};

// A struct to define the PEV memory map transferred to the PSC.
// Each channel contains 8 unsigned 64-bit registeres.
typedef struct
{
	uint64_t registers[NUMBER_OF_REGISTERS];
} channel_t;

// This struct defines the mapping of the correctors to the PSC controllers.
// Example: Corrector #4 is on channel 1 PS 1
struct psc_map_t
{
	uint8_t channel;
	uint8_t ps;
} psc_map[BPM_COUNT] = {
	{0, 1}, {0, 2}, {0, 3}, {1, 1},  {1, 2},  {1, 3},  {2, 1},  {2, 2},	
	{3, 1}, {3, 2}, {3, 3},	{4, 1},  {4, 2},  {4, 3},  {5, 1},  {5, 2},	
	{6, 1}, {6, 2}, {6, 3},	{7, 1},  {7, 2},  {7, 3},  {8, 1},  {8, 2},
	{9, 1}, {9, 2}, {9, 3},	{10, 1}, {10, 2}, {10, 3}, {11, 1}, {11, 2}
};

// Declarations for PEV initialization.
static channel_t* channels;
static struct pev_ioctl_map_pg map;
static struct pev_node *node;
static struct pev_ioctl_evt *event;
static void	*base;

static void initialize_pev();
static void cleanup_pev(int code);

int main()
{
	int socket_fd;
	struct sockaddr_in sa;
	double buffer[BPM_COUNT];
	double psc_iloads[BPM_COUNT];
	ssize_t bytes;

	initialize_pev();

	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = htonl(INADDR_ANY);
	sa.sin_port = htons(55555);

	socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(bind(socket_fd, (struct sockaddr*) &sa, sizeof(sa)) == -1)
	{
		perror("bind");
		close(socket_fd);
		return 1;
	}

	const uint32_t address = 157;
	uint32_t ps;
	uint32_t channel;
	while(1)
	{
		printf("Waiting for buffer ...\n");
		bytes = read(socket_fd, &buffer, sizeof(buffer));
		if(bytes < 0)
		{
			perror("recv error");
			exit(99);
		}

		int i = 0;
		for(i = 0; i < BPM_COUNT; i++)
		{
			// Test the received data.
			printf("%d\n", (int) buffer[i]);

			// Perform correction on the current values we stored initially.
			psc_iloads[i] += buffer[i];

			// Write these values to the memory map.
			ps = psc_map[i].ps;
			channel = psc_map[i].channel;
			channels[channel].registers[REGISTER_OPERATION_READ] =  REGISTER_OPERATION_WRITE | 
																	((uint64_t)ps) << REGISTER_LINK_PS_SHIFT |
																	(((uint64_t)address) << 32) | 
																	((uint64_t)( *(uint64_t*) &psc_iloads[i] ));
		}
	}

	return 0;
}

void initialize_pev()
{
	int status;

	node = pev_init(0);
	if(!node) 
	{
		printf("[psc][pev] Initialization failed");
		return;
	}
	if(node->fd < 0) 
	{
		printf("[psc][pev] Can't find PEV1100 interface");
		return;
	}

	/*
	 * Map PSC registers
	 */
	map.rem_addr = 0;
	map.mode  = MAP_ENABLE | MAP_ENABLE_WR | MAP_SPACE_USR1;
	map.flag  = 0;
	map.sg_id = MAP_MASTER_32;
	map.size  = 0x400000;
	status    = pev_map_alloc(&map);

	base = pev_mmap(&map);
	channels = (channel_t*)(base + BASE_CHANNEL_OFFSET);

	/*
	 * Allocate event queue, register channel event, and enable it
	 */
	event = pev_evt_queue_alloc(0);
	if(!event)
	{ 
		printf("[psc][pev] Can't allocate event queue");
		return;
	}
	event->wait = -1;

	int c = 0;
	for(c = 0; c < NUMBER_OF_CHANNELS; c++)
	{
		status = pev_evt_register (event, EVENT_ID + c);
		if(status)
		{
			printf("[psc][pev] Event queue register failed");
			return;
		}	
		status = pev_evt_queue_enable(event);
		if(status)
		{
			printf("[psc][pev] Event queue enable failed");
			return;
		}
	}
}

void cleanup_pev(int code)
{
	uint32_t channel;

	pev_evt_read(event, 1);
	pev_evt_unmask(event, event->src_id);

	if(base)
		pev_map_free(&map);

	if(event)
	{
		pev_evt_queue_disable(event);
		for (channel = 0; channel < NUMBER_OF_CHANNELS; channel++)
			pev_evt_unregister (event, EVENT_ID+channel);

		pev_evt_queue_free(event);
	}

	pev_exit(node);
	exit(code);
}

