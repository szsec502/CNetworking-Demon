#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

class TCPClient
{
	public:
		int m_sockfd;

		TCPClient();

		~TCPClient();

		bool ConnectTCP(const char *host, const int port);

		int Send(const void *buf, const int buflen);

		int Recv(void *buf, const int buflen);
};

TCPClient::TCPClient()
{
	m_sockfd = 0;
}

TCPClient::~TCPClient()
{
	if (m_sockfd != 0)
	{
		close(m_sockfd);
	}
}

bool TCPClient::ConnectTCP(const char *host, const int port)
{
	m_sockfd = socket(AF_INET, SOCK_STREAM, 0);

	struct hostent *h;
	if ((h = gethostbyname(host)) == 0)
	{
		close(m_sockfd);
		m_sockfd = 0;
		return false;
	}

	struct sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	memcpy(&serv_addr.sin_addr, h->h_addr_list, h->h_length);

	if (connect(m_sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) != 0)
	{
		close(m_sockfd);
		m_sockfd = 0;
		return false;
	}

	return true;
}

int TCPClient::Send(const void *buf, const int buflen)
{
	return send(m_sockfd, buf, buflen, 0);
}

int TCPClient::Recv(void *buf, const int buflen)
{
	return recv(m_sockfd, buf, buflen, 0);
}

int main()
{
	TCPClient TClient;
	if (TClient.ConnectTCP("192.168.2.207", 5051) == false)
	{
		printf("connect to server error\n");
		return -1;
	}

	char buffer[1024];

	for (int i = 0; i < 50; i++)
	{
		memset(buffer, 0, sizeof(buffer));
		sprintf(buffer, "this is %d message", i);
		if (TClient.Send(buffer, strlen(buffer)) <= 0)
			break;

		printf("send : %s\n", buffer);

		memset(buffer, 0, sizeof(buffer));
		if(TClient.Recv(buffer, strlen(buffer)) <= 0)
			break;

		printf("recv : %s \n", buffer);

		sleep(1);
	}
}
