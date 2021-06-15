#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <sys/uio.h>
#include <poll.h>
#include <net/if.h>
#include <pthread.h>
#include <sys/time.h>
#include <sched.h>

#include "mkl.h"

#define PORT		2048 
#define BUFFER_SIZE	1040
#define TOTAL_BPMS	65
#define BPM_COUNT	32
#define NIC			"eno2"

#define LOCK   pthread_mutex_lock(&orbit_data_lock)
#define UNLOCK pthread_mutex_unlock(&orbit_data_lock)

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
int    bpm_index[BPM_COUNT] = { 1,  2,  3,  5,  6,  7,  9, 10, 
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
	int     m = 32, n = 1, k = 32, i, j;
	int     sockfd;
	int     status;
	int     bytes;
	struct  timespec start, end;
	struct  sockaddr_in servaddr;
	struct  libera_payload payload[65];
	struct  iovec payload_vector[65];
	struct  pollfd events[1];

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
	if(orm == NULL || orbit_x == NULL || orbit_y == NULL ||
			delta_x == NULL || delta_y == NULL || gold_orbit_x == NULL || gold_orbit_y == NULL)
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
	
	sockfd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(sockfd < 0)
	{
		perror("socket"); 
		exit(EXIT_FAILURE); 
	}
	if(setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0)
	{
		printf("Socket opt #1\n");
		perror("setsocketopt");
		exit(1);
	}
	if(setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, NIC, strlen(NIC)) < 0)
	{
		printf("Socket opt #2\n");
		perror("setsocketopt");
		exit(1);
	}
	
	servaddr.sin_family = AF_INET; 
	servaddr.sin_port = htons(PORT); 
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(sockfd, (struct sockaddr*) &servaddr, sizeof(servaddr)) < 0)
	{
		perror("bind");
		exit(1);
	}

	while(1)
	{
		clock_gettime(id, &start);
		events[0].fd = sockfd;
		events[0].events = POLLIN;
		events[0].revents = 0;
		int status = poll(events, 1, 1000);
		if(status < 0)
		{
			perror("poll");
			exit(1);
		}

		int bytes = readv(sockfd, payload_vector, iov_count);
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
		clock_gettime(id, &end);

		long int d = 1E9 * (end.tv_sec - start.tv_sec) + end.tv_nsec - start.tv_nsec;
		printf("Calc: %8.3f us | %d\n", d / 1000.0, (int) orbit_x[0]);
	}

	return 0;
}

