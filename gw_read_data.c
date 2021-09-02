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

#define COUNT 32

int main()
{
	int socket_fd;
	struct sockaddr_in sa;
	double buffer[COUNT];
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
		for(i = 0; i < COUNT; i++)
			printf("%d\n", (int) buffer[i]);
	}

	return 0;
}
