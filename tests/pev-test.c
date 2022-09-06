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
#include <time.h>
#include <signal.h>
#include <stdbool.h>
#include <math.h>

#include <pevulib.h>

#define NUMBER_OF_CHANNELS	12
#define ADDRESS_SET_REF_C	175
#define ADDRESS_SET_REF_Q	144
#define ADDRESS_ILOAD_C		153
#define ADDRESS_ILOAD_Q		148
#define ADDRESS_ENABLE		128
#define BASE_CHANNEL_OFFSET 0x100
#define EVENT_ID            0x40
#define EVENT_ID_BITS		0xf0
#define EVENT_CHANNEL_BITS	0x0f
#define REGISTER_OPERATION_WRITE	0x0080000000000000ULL	// b56 == "w/!r"
#define REGISTER_OPERATION_READ		0x0000000000000000ULL	// b56 == "w/!r"
#define REGISTER_LINK_PS_SHIFT		46
#define TSR_ERROR					0x8000

#define BPM_COUNT	32

#define PEV_OK				0
#define PEV_ERR_CHANNEL 	1
#define PEV_ERR_REGISTER	2

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

typedef uint64_t u64;
typedef uint32_t u32;
typedef int64_t  i64;
typedef int32_t  i32;

typedef struct
{
	uint64_t registers[NUMBER_OF_REGISTERS];
} channel_t;

static channel_t* channels;
static struct pev_ioctl_map_pg map;
static struct pev_node *node;
static struct pev_ioctl_evt *event;
static void	*base;

static void initialize_pev();
static void cleanup_pev(int code);
static int  pev_read(u32 channel, u64 address, u64 ps, float* value);
static int  pev_write(u32 channel, u64 address, u64 ps, float value);

struct psc_map_t
{
	uint32_t channel;
	uint32_t ps;
} psc_map[BPM_COUNT] = {
	{0, 1}, {0, 2}, {0, 3}, {1, 1},  {1, 2},  {1, 3},  {2, 1},  {2, 2},
	{3, 1}, {3, 2}, {3, 3}, {4, 1},  {4, 2},  {4, 3},  {5, 1},  {5, 2},
	{6, 1}, {6, 2}, {6, 3}, {7, 1},  {7, 2},  {7, 3},  {8, 1},  {8, 2},
	{9, 1}, {9, 2}, {9, 3}, {10, 1}, {10, 2}, {10, 3}, {11, 1}, {11, 2}
};

int main(int argc, char** argv)
{
	u32 status;
	u32 channel = 0;
	u32 raw;
	u64 address;
	u64 ps = (u64)(1) << REGISTER_LINK_PS_SHIFT;

	float f = 1.0;
	float sampling = 10;
	float cycles = 1.0;
	float amp = 1.0;
	float v1, v2;
	int ps_count = 1;

	struct timespec start, end;
	clockid_t id = CLOCK_REALTIME;
	
	int socket_fd;
	struct sockaddr_in sa;
	uint64_t buffer[BPM_COUNT];
	ssize_t bytes;
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

	if(argc == 6)
	{
		if(argv[1])
			sampling = atof(argv[1]);
		if(argv[2])
			f = atof(argv[2]);
		if(argv[3])
			cycles = atof(argv[3]);
		if(argv[4])
			amp = atof(argv[4]);
		if(argv[5])
			ps_count = atoi(argv[5]);
	}

	signal(SIGINT, cleanup_pev);

	initialize_pev();

	float cycles_count = cycles / f;
	float x = 0.0;
	float _2pi_f = 2 * M_PI * f;
	float inc = sampling / 1000.0;
	float delay_us = sampling * 1000 - 185;
	float delay_us2 = sampling * 1000 - 185;

	int index = 0;
	int i = 0;
	float size = (cycles / f) * (1000.0 / sampling);
	float* buffer1 = (float*) malloc(size * sizeof(float));
	float* buffer2 = (float*) malloc(size * sizeof(float));
	//u32* raw_buffer = 
	
	for(; i < size; i++)
	{
		buffer1[i] = amp * sin(_2pi_f * x);
		x += inc;
	}

	for(i = 0; i < ps_count; i++)
	{
		channels[psc_map[i].channel].registers[REGISTER_NORMAL_WRITE] = 
			REGISTER_OPERATION_WRITE | (u64) psc_map[i].ps << REGISTER_LINK_PS_SHIFT | (u64) ADDRESS_ENABLE << 32 | (uint64_t) (1);
	}

	pev_evt_read(event, -1);
	pev_evt_unmask(event, event->src_id);
	for(i = 0; i < ps_count; i++)
	{
		status = (uint32_t)channels[psc_map[i].channel].registers[REGISTER_TSR];
	}

	usleep(delay_us2);

	printf("PS: %d\n", ps_count);
	while(true)
	{
		// printf("%f\n", *(buffer1 + index));
		// clock_gettime(id, &start);
		bytes = read(socket_fd, &buffer, sizeof(buffer));
		if(bytes < 0)
		{
			perror("recv error");
			continue;
		}

		raw = *(u32*) (buffer1 + index);
		for(i = 0; i < ps_count; i++)
		{
			channels[psc_map[i].channel].registers[REGISTER_NORMAL_WRITE] = 
				REGISTER_OPERATION_WRITE | (u64) psc_map[i].ps << REGISTER_LINK_PS_SHIFT | (u64) ADDRESS_SET_REF_C << 32 | (uint64_t) raw;
		}
 
		pev_evt_read(event, -1);
		pev_evt_unmask(event, event->src_id);
		for(i = 0; i < ps_count; i++)
		{
			status = (uint32_t)channels[psc_map[i].channel].registers[REGISTER_TSR];
		}
 
		// usleep(delay_us2);

		// pev_write(channel, ADDRESS_SET_REF_C, ps, buffer1[index]);
		// usleep(delay_us);

		// clock_gettime(id, &end);
		// int duration = (end.tv_sec - start.tv_sec) * 1E9 + (end.tv_nsec - start.tv_nsec);
		// printf("Duration: %f ms\n", duration / 1E6);

		index++;
		if(index >= size)
			index = 0;	
	}

	// for(; i < index; i++)
	//	printf("%f %f\n", buffer1[i], buffer2[i]);

	cleanup_pev(0);
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
	int status;
	int i;

	printf("SIGINT captured, cleanup in progress ... \n");
	
	pev_evt_read(event, 3);
	pev_evt_unmask(event, event->src_id);
	for(i = 0; i < NUMBER_OF_CHANNELS; i++)
	{
		status = (uint32_t)channels[psc_map[i].channel].registers[REGISTER_TSR];
	}

	if(base)
	{
		pev_map_free(&map);
	}

	if(event)
	{
		pev_evt_queue_disable(event);
		for (channel = 0; channel < NUMBER_OF_CHANNELS; channel++)
			pev_evt_unregister (event, EVENT_ID+channel);

		pev_evt_queue_free(event);
	}

	status = pev_exit(node);
	// printf("Status: %d\n", status);
	// printf("Done.\n");
	exit(0);
}

int pev_read(u32 channel, u64 address, u64 ps, float* value)
{
	u32 raw, status;

	channels[channel].registers[REGISTER_NORMAL_READ]  = REGISTER_OPERATION_READ  | ps | address << 32 | address;
	pev_evt_read(event, -1);
	pev_evt_unmask(event, event->src_id);
	if ((event->src_id & EVENT_ID_BITS) != EVENT_ID)
		return PEV_ERR_CHANNEL;

	channel = event->src_id & EVENT_CHANNEL_BITS;
	status = (uint32_t)channels[channel].registers[REGISTER_TSR];
	if (status & TSR_ERROR)
		return PEV_ERR_REGISTER;

	raw = (uint32_t)(channels[channel].registers[REGISTER_NORMAL_READ] & 0x00000000ffffffffULL);
	*value = *(float*) &raw;
	return PEV_OK;
}

int pev_write(u32 channel, u64 address, u64 ps, float value)
{
	u32 raw = *(u32*) &value;
	u32 status;

	channels[channel].registers[REGISTER_NORMAL_WRITE] = REGISTER_OPERATION_WRITE | ps | address << 32 | (uint64_t) raw;
	pev_evt_read(event, -1);
	pev_evt_unmask(event, event->src_id);
	if ((event->src_id & EVENT_ID_BITS) != EVENT_ID)
		return PEV_ERR_CHANNEL;

	channel = event->src_id & EVENT_CHANNEL_BITS;
	status = (uint32_t)channels[channel].registers[REGISTER_TSR];
	if (status & TSR_ERROR)
		return PEV_ERR_REGISTER;

	return PEV_OK;
}
