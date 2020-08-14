#pragma once

#include <vector>
#include <string>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdexcept>
#include <iostream>

//rpc框架
//服务器间调用函数的方式
//类似协议的存在
namespace rpc 
{
	class Server
	{
	private:
		int serverSock;
		int epfd;
		bool listenSignal;
		static constexpr int maxListen = 20; 

		inline void AcceptSocket()
		{
			sockaddr_in clntAddr = {};
			socklen_t size = sizeof(clntAddr);
			
			const int connfd = accept(this->serverSock, reinterpret_cast<sockaddr*>(&clntAddr), &size);
			if(connfd == -1)
			{
				std::cout << "accept failed!" << std::endl;
				return;
			}
			
			std::cout << "accept client_addr" << inet_ntoa(clntAddr.sin_addr) << std::endl;

			epoll_event ev;

			ev.data.fd = connfd;
			ev.events = EPOLLIN;
			epoll_ctl(this->epfd, EPOLL_CTL_ADD, connfd, &ev);

			//设置超时时间
        		struct timeval timeout={3,0};//3s
        		if(setsockopt(connfd, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout)) == -1)
        			std::cout << "setsoockopt failed!" << std::endl;
        		if(setsockopt(connfd, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout)) == -1)
        			std::cout << "setsoockopt failed!" << std::endl;
		}

		inline void HandleClient(const int& _clntfd)
		{
			char buffer[1024];

			memset(buffer, 0, sizeof(buffer));
			const int recvLen = recv(_clntfd, buffer, sizeof(buffer), 0);

			if(recvLen <= 0)
			{
				std::cout << "client close" << std::endl;

				epoll_event ev;

				epoll_ctl(this->epfd, EPOLL_CTL_DEL, _clntfd, &ev);
				close(_clntfd);
			}
			else
			{
				std::cout << "client:" << buffer << std::endl;
			}
		}

	public:
		Server():
			serverSock(-1),
			epfd(-1),
			listenSignal(false)
		{
			this->epfd = epoll_create(this->maxListen);
		}

		~Server()
		{
			this->StopListen();
			close(this->epfd);
		}
		
		void Listen(std::string_view _address, int _port = 3737)
		{
			if(this->listenSignal == true)
				throw std::logic_error("server had listening");

			this->listenSignal = true;

			sockaddr_in serverAddr = {};
			                                                         
			serverAddr.sin_family = AF_INET;
			serverAddr.sin_addr.s_addr = inet_addr(_address.data());
			serverAddr.sin_port = htons(_port);


			this->serverSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

			if(this->serverSock == -1)
				throw std::runtime_error("create socket failed!");

			if(bind(this->serverSock, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == -1)
			{
				perror("bind");
				throw std::runtime_error("bind socket failed!");
			}
			
			if(listen(this->serverSock, this->maxListen))
			{
				throw std::runtime_error("listen failed!");
			}

			epoll_event events[this->maxListen];
			

			if(epfd == -1)
			{
				throw std::runtime_error("create epoll failed!");
			}

			epoll_event ev;

			ev.data.fd = serverSock;
			ev.events = EPOLLIN;
			if(epoll_ctl(epfd, EPOLL_CTL_ADD, this->serverSock, &ev) == -1)
			{
				throw std::runtime_error("epoll control failed!");
			}
			
			std::cout << "rpc listening..." << std::endl;

			//用listening作为信号中断循环
			while(this->listenSignal == true)
			{
				const int nfds = epoll_wait(epfd, events, this->maxListen, -1);

				if(nfds == -1)
				{
					std::cout << "epoll wait failed!" << std::endl;
				}

				for(int i = 0; i < nfds; i++)
				{
					if(events[i].data.fd == this->serverSock)
					{
						this->AcceptSocket();
					}
					else if(events[i].events & EPOLLIN)
					{
						//处理客户端
						this->HandleClient(events[i].data.fd);	
					}
					else
					{
						std::cout << "something else happen" << std::endl;
					}
				}
			}

			close(this->serverSock);
		}

		void StopListen()
		{
			this->listenSignal = false;
		}
		
		template<typename _FUNC>
		void Register(std::string_view _funcName, _FUNC _funcPointer)
		{

		}

		//根据序列化后的参数调用
		void Call(std::string_view _name, std::vector<char*> _params)
		{

		}
	};

	class Client
	{
	private:
		int clntfd;

	public:
		Client():
			clntfd(-1)
		{
			this->clntfd = socket(AF_INET, SOCK_STREAM, 0);
		}

		~Client()
		{
			close(this->clntfd);
		}

		void Connect(std::string_view _address, int _port = 3737)
		{
			sockaddr_in serverAddr = {};

			serverAddr.sin_family = AF_INET;
			serverAddr.sin_addr.s_addr = inet_addr(_address.data());
			serverAddr.sin_port = htons(_port);
			
			const int connRes = connect(this->clntfd, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr));
			if(connRes != 0)
			{
				throw std::runtime_error("connect failed!");
			}
		}

		void CallFunc(std::string_view _funcName, std::vector<char*> _params)
		{
			char temp[] = "hello rpc server";

			if(send(this->clntfd, temp, sizeof(temp), 0) == -1)
			{
				throw std::runtime_error("send failed!");
			}
		}
	};
}
