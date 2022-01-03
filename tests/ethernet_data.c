#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>

#pragma pack(2)

#define PSC_OK		0
#define PSC_SOCKET	1
#define PSC_READ	2
#define PSC_WRITE	3
#define PSC_CONNECT	4
#define PSC_POLL	5

#define UDP_PACKET_LENGTH	10
#define PACKET_STATUS_RAW  	0x0000
#define COMMAND_READ		0x0001
#define COMMAND_WRITE		0x0002
#define PS_ADDRESS_SHIFT	14
#define ADDRESS_PRIORITY	0x2 | (1 << PS_ADDRESS_SHIFT)
#define ADDRESS_ILOAD		153 | (1 << PS_ADDRESS_SHIFT)
#define ADDRESS_SET_REF		175 | (1 << PS_ADDRESS_SHIFT)
#define ETHERNET_ENABLE		0x1000000
#define POLL_TIMEOUT		1000
#define IP_ADDRESS			"10.2.2.33"
#define PORT				9322

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;
typedef int32_t  i32;
typedef struct pollfd pollfd;

typedef struct
{
	u16 status;
	u16 command;
	u16 address;
	u32 data;
} packet_t;

typedef struct 
{
	char ip[16];
	int  port;
	int  fd;

	pollfd   poller[1];
	packet_t rx;
	packet_t tx;
	packet_t eth;
} psc_t;

psc_t* psc_init(char* ip, u16 port);
int psc_read(psc_t* device, float* value);
int psc_write(psc_t* device);

int main(int argc, char** argv)
{
	float  sampling = 10;
	float  v1, v2;
	float  t;
	float  amplitude = 1;
	float  frequency = 1;
	float  increment;
	float  _2pi_f;
	u32    delay_us;
	u32    duration;
	i32    cycles = 1;
	struct timespec start, end;
	clockid_t id = CLOCK_MONOTONIC;

	psc_t* psc;
	psc = (psc_t*) psc_init(IP_ADDRESS, PORT);
	if(!psc)
	{
		printf("Null PSC\n");
		return 0;
	}

	psc->tx.address = ADDRESS_PRIORITY;
	psc->tx.data = (u32) ETHERNET_ENABLE;
	psc_write(psc);
	psc->tx.address = ADDRESS_SET_REF;
	psc->tx.data = 0;

	if(argv[1])
		sampling = atof(argv[1]);
	if(argv[2])
		frequency = atof(argv[2]);
	if(argv[3])
		cycles = atoi(argv[3]);
	if(argv[4])
		amplitude = atof(argv[4]);

	delay_us = sampling * 1000 - 760;
	increment = sampling / 1000.0;
	_2pi_f = 2 * M_PI * frequency;
	t = 0;
	while( t <= (cycles / frequency) + sampling / 1000.0 )
	{
		clock_gettime(id, &start);
		v1 = amplitude * sin(_2pi_f * t);
		psc->tx.data = *(u32*)&v1;
		psc_write(psc);
		usleep(delay_us);
		psc_read(psc, &v2);
		t += increment;
		clock_gettime(id, &end);
		duration = (end.tv_sec - start.tv_sec)*1E9 + (end.tv_nsec - start.tv_nsec);
		/* printf("Duration: %010.3f ms\n", duration / 1E6); */
		printf("%f %f\n", v1, v2);
	}

	psc->tx.address = ADDRESS_PRIORITY;
	psc->tx.data = 0;
	psc_write(psc);
	close(psc->fd);
	return 0;
}

psc_t* psc_init(char* ip, u16 port)
{
	int status;
	struct sockaddr_in socket_address;
	psc_t* device = (psc_t*) malloc(sizeof(psc_t));

	device->fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(device->fd < 0)
		return NULL;

	strncpy(device->ip, ip, sizeof(device->ip));
	device->ip[strlen(device->ip)] = '\0';
	device->port = port;
	memset(&socket_address, 0, sizeof(socket_address));
	socket_address.sin_family = AF_INET;
	socket_address.sin_addr.s_addr = inet_addr(device->ip);
	socket_address.sin_port = htons(device->port);
	status = connect(device->fd, (struct sockaddr*) &socket_address, sizeof(socket_address));
	if(status < 0)
		return NULL;

	device->rx.status  = (u16) PACKET_STATUS_RAW;
	device->rx.command = (u16) COMMAND_READ;
	device->rx.address = (u16) ADDRESS_ILOAD;
	device->rx.data    = (u32) 0;

	device->tx.status  = (u16) PACKET_STATUS_RAW;
	device->tx.command = (u16) COMMAND_WRITE;
	device->tx.address = (u16) ADDRESS_SET_REF;
	device->tx.data    = (u32) 0;

	device->poller[0].fd = device->fd;
    device->poller[0].events = POLLIN;
    device->poller[0].revents = 0;
	return device;
}

int psc_read(psc_t* device, float* value)
{
	int status;

	status = write(device->fd, &device->rx, sizeof(device->rx));
	if(status < 0)
		return PSC_WRITE;
	
    status = poll(device->poller, 1, POLL_TIMEOUT);
    if(status <= 0)
    {
        return PSC_POLL;
    }

	status = read(device->fd, &device->rx, sizeof(device->rx));
	if(status < 0)
		return PSC_READ;

	*value = *(float*)(&device->rx.data);
	return PSC_OK;
}

int psc_write(psc_t* device)
{
	int status;

	status = write(device->fd, &device->tx, sizeof(device->tx));
	if(status < 0)
		return PSC_WRITE;

    status = poll(device->poller, 1, POLL_TIMEOUT);
    if(status <= 0)
    {
        return PSC_POLL;
    }

	status = read(device->fd, &device->tx, sizeof(device->tx));
	if(status < 0)
		return PSC_READ;

	return PSC_OK;
}

