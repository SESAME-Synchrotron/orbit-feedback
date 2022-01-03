#include <stdio.h> 
#include <stdlib.h> 
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
#include <time.h>

#define PORT			2048 
#define BUFFER_SIZE		1040
#define TOTAL_BPMS		65
#define BPM_COUNT		32
#define NET_INTERFACE	"eno2"

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
	int32_t sum;
	int32_t x;
	int32_t y;
	struct  flags flags;
	uint16_t counter;
};

int    bpm_index[BPM_COUNT] = { 1,  2,  3,  5,  6,  7,  9, 10, 
	                           23, 25, 26, 27, 29, 30, 31, 32,
							   33, 34, 35, 36, 38, 39, 41, 42,
							   50, 51, 52, 54, 61, 62, 63, 64 };

int main()
{
	int sockfd;
	struct sockaddr_in servaddr;
	struct libera_payload payload[65];
	struct iovec payload_vector[65];
	struct pollfd events[1];
	struct timespec start, end;
	clockid_t id = CLOCK_REALTIME;
	
	int i = 0;
	for(i = 0; i < TOTAL_BPMS; i++)
	{
		payload_vector[i].iov_base = &payload[i];
		payload_vector[i].iov_len  = sizeof(payload);
	}

	// Creating socket file descriptor 
	if ( (sockfd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0 ) { 
		perror("socket"); 
		exit(EXIT_FAILURE); 
	} 
	
	int broadcast = 1;
	if(setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0)
	{
		printf("Socket opt #1\n");
		perror("setsocketopt");
		exit(1);
	}
	if(setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, NET_INTERFACE, strlen(NET_INTERFACE)) < 0)
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

	int iov_count = sizeof(payload_vector) / sizeof(struct iovec);
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

		int i = 0;
		// for(i = 0; i < BPM_COUNT; i += 4)
		//	printf("I: %02d | X: %010d | Y: %010d\n", i, payload[bpm_index[i]].x, payload[bpm_index[i]].y);
		clock_gettime(id, &end);
		long int d = (end.tv_sec - start.tv_sec)*1000000000 + end.tv_nsec - start.tv_nsec;
		printf("Time: %011.3f us | %d\n", d / 1000.0, payload[bpm_index[0]].x);
	}

	close(sockfd);
	return 0;
}
