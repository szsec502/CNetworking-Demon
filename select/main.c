#include "../lib/common.h"

#define MAXLINE  1024

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		exit(EXIT_FAILURE);
	}

	int socket_fd = tcp_client(argv[1], SERV_PORT);
	char recv_line[MAXLINE], send_line[MAXLINE];
	int n;

	fd_set readmask;
	fd_set allreads;
	FD_ZERO(&allreads);
	FD_SET(0, &allreads);
	FD_SET(socket_fd, &allreads);

	for(;;)
	{
		readmask =allreads;
		int rc = select(socket_fd + 1, &readmask, NULL, NULL, NULL);
		if (rc <= 0)
		{
			exit(EXIT_FAILURE);
		}

		if (FD_ISSET(socket_fd, &readmask))
		{
			n = read(socket_fd, recv_line, MAXLINE);
			if (n < 0)
			{
				exit(EXIT_FAILURE);
			}
			else if (n == 0)
			{
				exit(EXIT_FAILURE);
			}
			recv_line[n] = 0;
			fputs(recv_line, stdout);
			fputs("\n", stdout);
		}
	}

	if (FD_ISSET(STDIN_FILENO, &readmask))
	{
		if (fgets(send_line, MAXLINE, stdin) != NULL)
		{
			int i = strlen(send_line);
			if (send_line[i - 1] == '\n')
			{
				send_line[i - 1] = 0;
			}

			printf("now sending %s\n", send_line);
			ssize_t rt = write(socket_fd, send_line, strlen(send_line));
			if (rt < 0)
			{
				exit(EXIT_FAILURE);
			}
			printf("send bytes : %zu \n", rt);
		}
	}
	return 0;
}
