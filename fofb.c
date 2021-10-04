#define _GNU_SOURCE

#include <stdio.h>
#include <time.h>
#include <string.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <poll.h>
#include <sched.h>

#include "mkl.h"

#define PORT			2048 
#define BUFFER_SIZE		1040
#define TOTAL_BPMS		65
#define BPM_COUNT		32
#define NIC_FA_DATA		"eno2"
#define NIC_GATEWAY		"eno1"
#define GW_X_ADDRESS	"10.2.2.18"
#define GW_Y_ADDRESS	"10.2.2.20"
#define GW_PORT			55555

struct flags
{
	uint16_t lock_status:1;
	uint16_t :1;
	uint16_t libera_id:8;
	uint16_t :1;
	uint16_t valid:1;
	uint16_t :1;
	uint16_t overflow:1;
	uint16_t interlock:1;
};

struct libera_payload
{
	int32_t  sum;
	int32_t  x;
	int32_t  y;
	struct   flags flags;
	uint16_t counter;
};

int bpm_index[BPM_COUNT] = { 1,  2,  3,  5,  6,  7,  9, 10,
							23, 25, 26, 27, 29, 30, 31, 32,
							33, 34, 35, 36, 38, 39, 41, 42,
							50, 51, 52, 54, 61, 62, 63, 64 };

int main()
{
	cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET(19, &mask);
	int result = sched_setaffinity(0, sizeof(mask), &mask);

	double* orbit_x;
	double* orbit_y;
	double* gold_orbit_x;
	double* gold_orbit_y;
	double* delta_orbit_x;
	double* delta_orbit_y;
	double* orm;
	double* delta_x;
	double* delta_y;
	double  alpha = 1, beta = 0;
	double  x_buffer[32];
	double  y_buffer[32];
	int     m = 32, n = 1, k = 32, i, j;
	int     libera_socket;
	int     gw_x_socket;
	int     gw_y_socket;
	int     status;
	int     status_x;
	int     status_y;
	int     bytes;
	struct  timespec start, end;
	struct  sockaddr_in libera_address;
	struct  sockaddr_in gw_x_address;
	struct  sockaddr_in gw_y_address;
	struct  libera_payload payload[65];
	struct  iovec payload_vector[65];
	struct  pollfd events[1];
	uint64_t data;
	long int duration;
	lapack_int pivot[BPM_COUNT];

	int broadcast = 1;
	int iov_count = sizeof(payload_vector) / sizeof(struct iovec);
	clockid_t id = CLOCK_REALTIME;

	orm          = (double*) mkl_malloc(m*k*sizeof(double), 64);
	orbit_x      = (double*) mkl_malloc(k*n*sizeof(double), 64);
	orbit_y      = (double*) mkl_malloc(k*n*sizeof(double), 64);
	delta_x      = (double*) mkl_malloc(k*n*sizeof(double), 64);
	delta_y      = (double*) mkl_malloc(k*n*sizeof(double), 64);
	gold_orbit_x = (double*) mkl_malloc(k*n*sizeof(double), 64);
	gold_orbit_y = (double*) mkl_malloc(k*n*sizeof(double), 64);
	if(orm == NULL || orbit_x == NULL || orbit_y == NULL || delta_x == NULL || delta_y == NULL || gold_orbit_x == NULL || gold_orbit_y == NULL)
	{
		printf("Null mkl_alloc\n");
		return 1;
	}

	srand(time(0));
	for(i = 0; i < m*k; i++)
	{
		orm[i] = rand() % 10 + 1;
	}
	for(i = 0; i < k*n; i++)
	{
		orbit_x[i] = 0;
		orbit_y[i] = 0;
		delta_x[i] = 0;
		delta_y[i] = 0;
	}
	for(i = 0; i < TOTAL_BPMS; i++)
	{
		payload_vector[i].iov_base = &payload[i];
		payload_vector[i].iov_len  = sizeof(payload);
	}
	
	libera_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(libera_socket < 0)
	{
		perror("socket"); 
		exit(EXIT_FAILURE); 
	}
	if(setsockopt(libera_socket, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0)
	{
		perror("setsocketopt broadcast");
		exit(1);
	}
	if(setsockopt(libera_socket, SOL_SOCKET, SO_BINDTODEVICE, NIC_FA_DATA, strlen(NIC_FA_DATA)) < 0)
	{
		perror("setsocketopt bind");
		exit(1);
	}
	
	libera_address.sin_family = AF_INET; 
	libera_address.sin_port = htons(PORT); 
	libera_address.sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(libera_socket, (struct sockaddr*) &libera_address, sizeof(libera_address)) < 0)
	{
		perror("bind");
		exit(1);
	}

	gw_x_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(gw_x_socket < 0)
	{
		perror("x gw socket");
		exit(1);
	}

	gw_x_address.sin_family = AF_INET;
	gw_x_address.sin_port = htons(GW_PORT);
	gw_x_address.sin_addr.s_addr = inet_addr(GW_X_ADDRESS);
	if(connect(gw_x_socket, (struct sockaddr*) &gw_x_address, sizeof(gw_x_address)) < 0)
	{
		perror("x gw connect");
		exit(1);
	}
	if(setsockopt(gw_x_socket, SOL_SOCKET, SO_BINDTODEVICE, NIC_GATEWAY, strlen(NIC_GATEWAY)) < 0)
	{
		perror("x gw setsocketopt");
		exit(1);
	}

	gw_y_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(gw_y_socket < 0)
	{
		perror("y gw socket");
		exit(1);
	}

	gw_y_address.sin_family = AF_INET;
	gw_y_address.sin_port = htons(GW_PORT);
	gw_y_address.sin_addr.s_addr = inet_addr(GW_Y_ADDRESS);
	if(connect(gw_y_socket, (struct sockaddr*) &gw_y_address, sizeof(gw_y_address)) < 0)
	{
		perror("y gw connect");
		exit(1);
	}
	if(setsockopt(gw_y_socket, SOL_SOCKET, SO_BINDTODEVICE, NIC_GATEWAY, strlen(NIC_GATEWAY)) < 0)
	{
		perror("y gw setsocketopt");
		exit(1);
	}

	LAPACKE_dgetrf(LAPACK_ROW_MAJOR, BPM_COUNT, BPM_COUNT, orm, BPM_COUNT, pivot);
	LAPACKE_dgetri(LAPACK_ROW_MAJOR, BPM_COUNT, orm, BPM_COUNT, pivot);
	while(1)
	{
		clock_gettime(id, &start);
		events[0].fd = libera_socket;
		events[0].events = POLLIN;
		events[0].revents = 0;
		int status = poll(events, 1, 1000);
		if(status < 0)
		{
			perror("poll");
			exit(1);
		}

		int bytes = readv(libera_socket, payload_vector, iov_count);
		if(bytes != BUFFER_SIZE)
			continue;
		
		for(i = 0; i < BPM_COUNT; i++)
		{
			orbit_x[i] = payload[bpm_index[i]].x;
			orbit_y[i] = payload[bpm_index[i]].y;
			gold_orbit_x[i] = orbit_x[i] / 2;
			gold_orbit_y[i] = orbit_y[i] / 2;
		}

		cblas_daxpy(BPM_COUNT, -1, gold_orbit_x, 0, orbit_x, 0);
		cblas_daxpy(BPM_COUNT, -1, gold_orbit_y, 0, orbit_y, 0);
		
		cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, m, n, k, alpha, orm, k, orbit_x, n, beta, delta_x, n);
		cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, m, n, k, alpha, orm, k, orbit_y, n, beta, delta_y, n);

		for(i = 0; i < BPM_COUNT; i++)
		{
			memcpy(&data, &delta_x[i], sizeof(double));
			data = htobe64(data);
			memcpy(x_buffer + i, &data, sizeof(uint64_t));

			memcpy(&data, &delta_y[i], sizeof(double));
			data = htobe64(data);
			memcpy(y_buffer + i, &data, sizeof(uint64_t));
		}
		
		status_x = write(gw_x_socket, x_buffer, sizeof(x_buffer));
		status_y = write(gw_y_socket, y_buffer, sizeof(y_buffer));
		if(status_x < 0)
		{
			perror("x gw write");
		}
		if(status_y < 0)
		{
			perror("y gw write");
		}

		clock_gettime(id, &end);

		duration = 1E9 * (end.tv_sec - start.tv_sec) + end.tv_nsec - start.tv_nsec;
		printf("Calc: %8.3f us | X: %10d | Y: %10d\n", duration / 1000.0, (int) delta_x[0], (int) delta_y[0]);
	}

	close(libera_socket);
	close(gw_x_socket);
	close(gw_y_socket);

	return 0;
}

