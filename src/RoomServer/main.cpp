#include "web.h"

#include "../Service/RoomService.h"

class Controller
{
private:
	struct PingInfo
	{
		std::unique_ptr<web::ISocket> sock;
		int64_t roomId;
	};


	RoomService roomService;
	std::unordered_map<int, PingInfo> pingSock;

	//epoll
	int epfd;
	static constexpr int maxClient = 50;
	std::thread checkProc;
	bool check;

	static void GameServerCheck(Controller* _controller)
	{
		std::cout << "start check" << std::endl;

		if(_controller->epfd == -1)
			throw std::runtime_error("creat epoll failed!");

		epoll_event events[Controller::maxClient];
		while(_controller->check == true)
		{
			const int nfds = epoll_wait(_controller->epfd, events, Controller::maxClient, -1);

			for(int i = 0; i < nfds; i++)
			{
				if(events[i].events & EPOLLIN)
				{
					char buffer[1024];
					const int recvLen = recv(events[i].data.fd, buffer, sizeof(buffer), 0);
					if(recvLen == 0 || recvLen == -1)
					{
						epoll_ctl(_controller->epfd, EPOLL_CTL_DEL, events[i].data.fd, &events[i]);
						_controller->roomService.RemoveRoom(_controller->pingSock.at(events[i].data.fd).roomId);
						_controller->pingSock.erase(events[i].data.fd);
					}
				}
				else
				{
					std::cout << "something else happend" << std::endl;
				}	
			}

			std::this_thread::sleep_for(std::chrono::seconds(1));
		}

		std::cout << "end check" << std::endl;
	}

public:
	Controller():
		epfd(epoll_create(this->maxClient)),
		check(true)
	{
		this->checkProc = std::thread(Controller::GameServerCheck, this);
	}

	~Controller()
	{
		this->check = false;
		this->checkProc.join();	
	}

	web::HttpResponse AddRoom(const web::UrlParam& _params, const web::HttpHeader& _header)
	{
		const std::string roomName = _params["name"].ToString();
		
		web::HttpClient client;

		//唤醒服务器地址
		const char* callGameServerAddress = "127.0.0.1";
		const int port = 7500;

		client.Connect(callGameServerAddress, port);

		web::HttpResponse response = client.SendRequest("POST", "/ExecGameServer");

		if(response.GetStateCode() != 200)
		{
			return web::Json("error:request GameCallServer failed!");
		}

		const std::string ipaddress = std::string(response.GetBody(),  response.GetBodySize());
		auto info = this->roomService.AddRoom(roomName, ipaddress);

		std::unique_ptr<web::ISocket> tempSock(std::make_unique<web::Socket>(AF_INET, SOCK_STREAM, IPPROTO_TCP));

		//约定必须有':'号分割端口 如：127.0.0.1:9999
		std::string::size_type pos(ipaddress.find(":"));
		if(pos == std::string::npos)
			throw std::logic_error("ipadress invaild!");

		const std::string tempIp(ipaddress.substr(0, pos).data());
		const int tempPort(std::stoi(ipaddress.substr(pos + 1)));

		std::cout << "tempIp:" << tempIp << std::endl;
		std::cout << "tempPort" << tempPort << std::endl;

		sockaddr_in serverAddr = {};
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(tempPort);
		serverAddr.sin_addr.s_addr = inet_addr(tempIp.data());

		int result = tempSock->Connect(reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr));
		int tryTime = 5;
		while (result != 0 && tryTime-- > 0)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			result = tempSock->Connect(reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr));
		}

		if(result != 0)
		{
			this->roomService.RemoveRoom(info.roomId);
			perror("connect");
			return web::Json("error:check failed!");
		}

		epoll_event ev;
		ev.data.fd = tempSock->Get();
		ev.events = EPOLLIN;
		if(epoll_ctl(this->epfd, EPOLL_CTL_ADD, tempSock->Get(), &ev) != 0)
			throw std::runtime_error("epoll ctl failed!");
		
		PingInfo pingInfo = {};

		pingInfo.roomId = info.roomId;
		pingInfo.sock = std::move(tempSock);

		this->pingSock.insert(std::pair<int, PingInfo>(pingInfo.sock->Get(), std::move(pingInfo)));
		
		return web::Json(ipaddress);
	}
	
	web::HttpResponse GetList(const web::UrlParam& _params, const web::HttpHeader& _header)
	{
		web::JsonObj res;
		for(const auto& item: this->roomService.GetRooms())
		{
			web::JsonObj temp;

			temp["roomId"] = item->roomId;
			temp["name"] = item->roomName;
			temp["ip"] = item->ipaddr;

			res.Push(std::move(temp));
		}

		return web::Json(res);	
	}
	
	web::HttpResponse RemoveRoom(const web::UrlParam& _params, const web::HttpHeader& _header)
	{
		this->roomService.RemoveRoom(std::stoll(_params["id"].ToString()));

		return web::Json("ok");
	}
};

int main(int _argc, char** _args)
{
	if(_argc != 3)
	{
		std::cout << "args must be two. args[1] is ip args[2] is port." << std::endl;
		return -1;
	}

	std::unique_ptr<web::Router> router(new web::Router());
	Controller controller;

	router->RegisterUrl("POST", "/AddRoom", &Controller::AddRoom, &controller);
	router->RegisterUrl("GET", "/List", &Controller::GetList, &controller);
	//router->RegisterUrl("POST", "/RemoveRoom", &Controller::RemoveRoom, &controller);

	web::HttpServer server(std::move(router));

	server.Listen(_args[1], std::stoi(_args[2]));

	return 0;
}
