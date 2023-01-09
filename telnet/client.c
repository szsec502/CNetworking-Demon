#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <syslog.h>
#include <stdarg.h>


#define MAXLINE 1024

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

int tcp_client(char *addr, int port)
{
	int sock_fd;
	sock_fd = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in server_addr;
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);

	inet_pton(AF_INET, addr, &server_addr.sin_addr.s_addr);
	socklen_t server_len = sizeof(server_addr);
	int rt = connect(sock_fd, (struct sockaddr *)&server_addr, server_len);
	if (rt < 0)
	{
		error(1, errno, "connect failed ");
	}

	return sock_fd;
}

int main(int argc, char **argv)
{
	if (argc != 3)
	{
		error(1, 0, "Usage : client <ip address> <port>");
	}

	int port = atoi(argv[2]);
	int sock_fd = tcp_client(argv[1], port);
	char recv_len[MAXLINE], send_len[MAXLINE];
	int n;

	fd_set readmask;
	fd_set allreads;
	FD_ZERO(&allreads);
	FD_SET(0, &allreads);
	FD_SET(sock_fd, &allreads);

	for(;;)
	{
		readmask = allreads;
		int rc = select(sock_fd + 1, &readmask, NULL, NULL, NULL);

		if (rc <= 0)
		{
			error(1, errno, "select failed ");
		}

		if(FD_ISSET(sock_fd, &readmask))
		{
			n = read(sock_fd, recv_len, MAXLINE);
			if (n < 0)
			{
				error(1, errno, "read error");
			}
			else if (n == 0)
			{
				printf("server closed\n");
				break;
			}
			recv_len[n] = 0;
			fputs(recv_len, stdout);
			fputs("\n", stdout);
		}

		if (FD_ISSET(STDIN_FILENO, &readmask))
		{
			if(fgets(send_len, MAXLINE, stdin) != NULL)
			{
				int length = strlen(send_len);
				if (send_len[length - 1] == '\n')
				{
					send_len[length - 1] = 0;
				}
				
				if (strncmp(send_len, "quit", strlen(send_len)) == 0)
				{
					if (shutdown(sock_fd, 1)) {
						error(1, errno, "shutdown failed");
					}
				}

				size_t rt = write(sock_fd, send_len, strlen(send_len));
				if (rt < 0)
				{
					error(1, errno, "write failed");
				}
			}
		}
	}

	exit(0);
	return 0;
}
