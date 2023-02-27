#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>

class TCPServer
{
	public:
		int m_listenfd;
		int m_clientfd;

		TCPServer();

		bool NewTCPServer(const int port);

		bool Accept();

		int Send(const void *buf, const int buflen);

		int Recv(void *buf, const int buflen);

		void CloseClient();

		void CloseListen();

		~TCPServer();
};

TCPServer::TCPServer()
{
	m_listenfd = m_clientfd = 0;
}

TCPServer::~TCPServer()
{
	if (m_listenfd != 0)
	{
		close(m_listenfd);
	}

	if (m_clientfd != 0)
	{
		close(m_clientfd);
	}
}

//  实例化一个TCP server
bool TCPServer::NewTCPServer(const int port)
{
	if (m_listenfd != 0)
	{
		close(m_listenfd);
		m_listenfd = 0;
	}

	m_listenfd = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_port = htons(port);
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_family = AF_INET;

	if (bind(m_listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) != 0)
	{
		close(m_listenfd);
		m_listenfd = 0;
		return false;
	}

	// 将socket设置为监听模式
	if (listen(m_listenfd, 5) != 0)
	{
		close(m_listenfd);
		m_listenfd = 0;
		return false;
	}

	return true;
}

bool TCPServer::Accept()
{
	if ((m_clientfd = accept(m_listenfd, 0, 0)) <= 0)
	{
		return false;
	}

	return true;
}

int TCPServer::Send(const void *buf, const int buflen)
{
	return send(m_clientfd, buf, buflen, 0);
}

int TCPServer::Recv(void *buf, const int buflen)
{
	return recv(m_clientfd, buf, buflen, 0);
}

void TCPServer::CloseClient()
{
	if (m_clientfd != 0)
	{
		close(m_clientfd);
		m_clientfd = 0;
	}
}

void TCPServer::CloseListen()
{
	if (m_listenfd != 0)
	{
		close(m_listenfd);
		m_listenfd = 0;
	}
}

TCPServer TServer;

int main()
{
	signal(SIGCHLD, SIG_IGN); // 忽略子进程退出信号，避免产生僵尸进程

	if (TServer.NewTCPServer(5051) == false)
	{
		printf("new tcp server error\n");
		return -1;
	}

	while(1)
	{
		if (TServer.Accept() == false)
		{
			continue;
		}

		if (fork() > 0)
		{
			TServer.CloseClient(); // 父进程回到while循环，继续处理新的Accept
			continue;
		}

		TServer.CloseListen(); // 子进程负责与客户端进行通信，直到客户端断开连接
		printf("client is disconnetion...\n");

		char buffer[1024];

		while(1)
		{
			// 服务端先进行数据接收
			memset(buffer, 0, sizeof(buffer));
			if (TServer.Recv(buffer, strlen(buffer)) <= 0)
			{
				break;
			}

			printf("recv : %s\n", buffer);

			// 服务端开始回复消息给客户端

			strcpy(buffer, "Ok");
			if (TServer.Send(buffer, strlen(buffer)) <= 0)
			{
				break;
			}

			printf("send : %s\n", buffer);
		}

		printf("client is disconnetion...\n");

		return 0;
	}
}
