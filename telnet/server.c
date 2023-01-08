#include <signal.h>
#include <strings.h>
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


#define SERV_PORT 43211
#define LISTENQ 15

static int count;

char *run_cmd(char *cmd)
{
	char *data = malloc(16384);
	bzero(data, sizeof(data));
	FILE *fdp;
	const int max_buffer = 256;
	char buffer[max_buffer];
	fdp = popen(cmd, "r");
	char *data_index = data;
	if (fdp)
	{
		while(!feof(fdp))
		{
			if (fgets(buffer, max_buffer, fdp) != NULL) 
			{
				int len = strlen(buffer);
				memcpy(data_index, buffer, len);
				data_index += len;
			}
		}
		pclose(fdp);
	}
	return data;
}

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
	int listenfd;
	int on = 1;

	listenfd = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in server_addr;
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(SERV_PORT);

	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

	int rt1 = bind(listenfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
	if (rt1 < 0)
	{
		error(1, errno, "bind failed ");
	}

	int rt2 = listen(listenfd, LISTENQ);
	if (rt2 < 0)
	{
		error(1, errno, "listen failed ");
	}

	signal(SIGPIPE, SIG_IGN);

	int confd;
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);

	char buffer[256];
	count = 0;

	while(1)
	{
		if ((confd = accept(listenfd, (struct sockaddr *)&client_addr, &client_len)) < 0)
		{
			error(1, errno, "accept failed ");
		}

		while(1)
		{
			bzero(buffer, sizeof(buffer));
			int n = read(confd, buffer, sizeof(buffer));
			if (n < 0)
			{
				error(1, errno, "error read message ");
			} else if (n == 0)
			{
				printf("client close\n");
				close(confd);
				break;
			}

			count++;
			buffer[n] = 0;

			if (strncmp(buffer, "ls", n) == 0)
			{
				char *result = run_cmd("ls");
				if (send(confd, result, strlen(result), 0) < 0)
				{
					return 1;
				}
				free(result);
			} else if (strncmp(buffer, "pwd", n) == 0)
			{
				char buffer[256];
				char *result = getcwd(buffer, 256);
				if (send(confd, result, strlen(result), 0) < 0)
				{
					return 1;
				}
			} else if (strncmp(buffer, "cd", 3) == 0)
			{
				char target[256];
				bzero(target, sizeof(target));
				memcpy(target, buffer + 3, strlen(buffer) - 3);
				if (chdir(target) == -1)
				{
					printf("change dir failed , %s\n", target);
				}
			} else {
				char *error = "error unkown input type";
				if (send(confd, error, strlen(error), 0) < 0)
				{
					return 1;
				}
			}
		}
	}
	exit(0);
	return 0;
}
