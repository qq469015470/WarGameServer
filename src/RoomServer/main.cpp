#include "web.h"

#include "../Service/RoomService.h"

class Controller
{
private:
	RoomService roomService;

public:
	web::HttpResponse AddRoom(const web::UrlParam& _params, const web::HttpHeader& _header)
	{
		const std::string roomName = _params["name"].ToString();
		
		web::HttpClient client;

		//唤醒服务器地址
		const char* callGameServerAddress = "192.168.1.105";
		const int port = 7500;

		client.Connect(callGameServerAddress, port);

		web::HttpResponse response = client.SendRequest("POST", "/ExecGameServer");

		if(response.GetStateCode() != 200)
		{
			return web::Json("error:request GameCallServer failed!");
		}

		const std::string ipaddress = std::string(response.GetBody(),  response.GetBodySize());
	
		this->roomService.AddRoom(roomName, ipaddress);
		
		return web::Json("ok");
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

			res.Push(temp);
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
	router->RegisterUrl("POST", "/RemoveRoom", &Controller::RemoveRoom, &controller);

	web::HttpServer server(std::move(router));

	server.Listen(_args[1], std::stoi(_args[2]));

	return 0;
}
