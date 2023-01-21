#include <signal.h>
#include <stddef.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <stdarg.h>
#include <sys/un.h>
#include <syslog.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#define MAXLINE 4096
#define BUFFER_SIZE 1024


void error(int status, int err, char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	if (err)
		fprintf(stderr, ":%s (%d)\n", strerror(err), err);
	if (status)
		exit(status);
}


int main(int argc, char **argv)
{
	if (argc != 2)
	{
		error(1, 0, "usage : server <local path>");
	}

	int sockfd;
	sockfd = socket(AF_LOCAL, SOCK_STREAM, 0);
	if (sockfd < 0)
	{
		error(1, 0, "socket create error");
	}

	struct sockaddr_un server_addr;
	char *local_path = argv[1];
	unlink(local_path);
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sun_family = AF_LOCAL;
	strcpy(server_addr.sun_path, local_path);

	if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		error(1, errno, "bind failed");
	}

	char buffer[BUFFER_SIZE];
	struct sockaddr_un client_addr;
	socklen_t client_len = sizeof(client_addr);

	while(1)
	{
		bzero(buffer, sizeof(buffer));
		if (recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, &client_len) == 0)
		{
			printf("client quit\n");
			break;
		}
		printf("Receive : %s \n", buffer);

		char send_line[MAXLINE];
		bzero(send_line, sizeof(send_line));
		sprintf(send_line, "HI , %s", buffer);

		size_t nbytes = strlen(send_line);
		printf("now sending: %s \n", send_line);

		if (sendto(sockfd, send_line, nbytes, 0, (struct sockaddr *)&client_addr, client_len) != nbytes)
		{
			error(1, errno, "sendto failed");
		}
	}

	close(sockfd);

	return 0;
}
