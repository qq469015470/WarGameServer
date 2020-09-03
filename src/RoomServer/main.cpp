#include "web.h"

#include "../Service/RoomService.h"

class Controller
{
private:
	RoomService roomService;	

public:
	Controller()
	{
		for(const auto& item: this->roomService.GetRooms())
		{
			std::cout << item->roomId << std::endl;
		}
	}

	web::HttpResponse AddRoom(const web::UrlParam& _params, const web::HttpHeader& _header)
	{
		this->roomService.AddRoom(_params["name"].ToString(), _params["ip"].ToString());
		
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
//	web::JsonObj temp;
//
//	std::string asd("newbee");
//
//	temp["a"]["wtf"] = "123";
//	temp["a"]["haha"] = asd;
//	temp["c"] = 123;
//	temp["f"] = 123;
//	temp["f"] = 123.34;
//	temp["asd"]["attr"] = false;
//	temp["asd"]["attr2"].SetNull();
//
//	temp["asd"]["arrtest"].Push(true);
//	temp["asd"]["arrtest"].Push(false);
//	temp["asd"]["arrtest"].Push("测试");
//
//	web::JsonObj haha;
//
//	haha.Push(temp);
//	haha.Push(false);
//
//	std::cout << temp.ToJson() << std::endl;
//	std::cout << haha.ToJson() << std::endl;
//
//	return 0;
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
