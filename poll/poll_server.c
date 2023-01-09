#include <signal.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <stdarg.h>
#include <syslog.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <poll.h>

#define LISTENQ   15
#define MAXLINE   1024
#define SERV_PORT 43211
#define INI_SIZE  128

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

int tcp_server_listener(int port)
{
	int listenfd;
	listenfd = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in server_addr;
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(port);

	int on = -1;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

	int rt1 = bind(listenfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
	if (rt1 < 0)
	{
		error(1, errno, "bind failed");
	}

	int rt2 = listen(listenfd, LISTENQ);
	if (rt2 < 0)
	{
		error(1, errno, "listen failed");
	}

	signal(SIGPIPE, SIG_IGN);

	return listenfd;
}

int main(int argc, char **argv)
{
	int listenfd, clientfd;
	int ready_number;
	ssize_t n;
	char buffer[MAXLINE];
	struct sockaddr_in client_addr;

	listenfd = tcp_server_listener(SERV_PORT);
	struct pollfd event_set[INI_SIZE];

	event_set[0].fd = listenfd;
	event_set[0].events = POLLRDNORM;

	int i;
	for(i = 1; i < INI_SIZE; i++)
	{
		event_set[i].fd = -1;
	}

	for(;;)
	{
		if((ready_number = poll(event_set, INI_SIZE, -1)) < 0)
		{
			error(1, errno, "poll failed");
		}

		if(event_set[0].revents & POLLRDNORM) {
			socklen_t client_len = sizeof(client_addr);
			clientfd = accept(listenfd, (struct sockaddr *)&client_addr, &client_len);

			for (i = 1; i < INI_SIZE; i++)
			{
				if (event_set[i].fd < 0)
				{
					event_set[i].fd = clientfd;
					event_set[i].events = POLLRDNORM;
					break;
				}
			}

			if (i == INI_SIZE)
			{
				error(1, errno, "can not hold so many clients");
			}

			if (--ready_number <= 0)
			{
				continue;
			}
		}

		for (i = 1; i < INI_SIZE; i++)
		{
			int socket_fd;
			if ((socket_fd = event_set[i].fd) < 0)
				continue;
			if (event_set[i].revents & (POLLRDNORM | POLLERR))
			{
				if ((n = read(socket_fd, buffer, MAXLINE)) > 0)
				{
					if (write(socket_fd, buffer, n) < 0)
					{
						error(1, errno, "write error");
					}
				} else if (n == 0 || errno == ECONNRESET)
				{
					close(socket_fd);
					event_set[i].fd = -1;
				} else {
					error(1, errno, "read error");
				}

				if (--ready_number <= 0)
					break;
			}
		}
	}

	return 0;
}
