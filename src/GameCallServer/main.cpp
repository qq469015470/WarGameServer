#include "web.h"

#include <list>
#include <stdlib.h>

class Controller
{
private:
	std::string serverIp;
	//可用端口
	std::list<int> offUse;
	//未用端口
	std::list<int> inUse;

	int ApplyPort()
	{
		if(this->offUse.size() == 0)
			throw std::runtime_error("out of port");

		int port = this->offUse.back();
		this->offUse.pop_back();
		this->inUse.insert(this->inUse.begin(), port);

		return port;
	}

public:
	Controller(std::string _serverIp):
		serverIp(_serverIp)
	{
		for(int i = 11000; i <= 12000; i++)
		{
			offUse.push_back(i);
		}
	}

	//执行gameserver程序
	web::HttpResponse ExecGameServer(const web::UrlParam& _params, const web::HttpHeader& _header)
	{
		const int port = this->ApplyPort();

		//后台执行GameServer
		std::string command("~/Project/WarGameServer/src/GameServer/build/bin/GameServer ");
		command += this->serverIp + " " + std::to_string(port) + " &";
		const int result = system(command.c_str());
		if(result != 0)
			return web::Json(std::string("error:system call return " + std::to_string(result)));

		return web::Json(this->serverIp + ":" + std::to_string(port));
	}
};

int main(int _argc, char** _args)
{
	if(_argc != 3)
	{
		std::cout << "args must be two. args[1] is ip args[2] is port." << std::endl;
		return -1;
	}

	std::unique_ptr<web::Router> router(std::make_unique<web::Router>());
	Controller controller(_args[1]);

	router->RegisterUrl("POST", "/ExecGameServer", &Controller::ExecGameServer, &controller);

	web::HttpServer server(std::move(router));

	server.Listen(_args[1], std::stoi(_args[2]));

	return 0;
}
