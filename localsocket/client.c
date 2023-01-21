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
		error(1, 0, "usage : client <local path>\n");
	}

	int sockfd;
	struct sockaddr_un client_addr, server_addr;

	sockfd = socket(AF_LOCAL, SOCK_STREAM, 0);
	if (sockfd < 0)
	{
		error(1, 0, "create socket error\n");
	}

	bzero(&client_addr, sizeof(client_addr));
	client_addr.sun_family = AF_LOCAL;
	strcpy(client_addr.sun_path, tmpnam(NULL));

	if (bind(sockfd, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0)
	{
		error(1, 0, "bind failed\n");
	}

	bzero(&server_addr, sizeof(server_addr));
	server_addr.sun_family = AF_LOCAL;
	strcpy(server_addr.sun_path, argv[1]);

	char send_line[MAXLINE];
	bzero(send_line, sizeof(send_line));
	char recv_line[MAXLINE];

	while(fgets(send_line, MAXLINE, stdin) != NULL)
	{
		int len = strlen(send_line);
		if(send_line[len - 1] == '\n')
		{
			send_line[len - 1] = 0;
		}

		size_t nbytes = strlen(send_line);
		printf("now sending %s\n", send_line);

		if (sendto(sockfd, send_line, nbytes, 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) != nbytes)
		{
			error(1, 0, "sendto failed");
		}

		int n = recvfrom(sockfd, recv_line, MAXLINE, 0, NULL, NULL);
		recv_line[n] = 0;

		fputs(recv_line, stdout);
		fputs("\n", stdout);
	}

	return 0;
}
