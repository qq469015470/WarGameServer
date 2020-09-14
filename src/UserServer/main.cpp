#include "../Service/UserService.h"
#include "../Service/ChatService.h"

#include "web.h"

#include <stdlib.h>
#include <thread>
#include <chrono>

web::HttpResponse HomePage(const web::UrlParam& _params, const web::HttpHeader& _header)
{
	try
	{
		UserService userService;
		const char* token = _header.GetCookie("token");

		if(token == "")
			return web::View("home/index.html");

		std::optional<User> user(userService.GetUser(token));

		if(user.has_value())
			return web::View("home/chat.html");
		else
			return web::View("home/index.html");
	}
	catch(...)
	{
		return web::View("home/index.html");
	}
}

web::HttpResponse Login(const web::UrlParam& _params, const web::HttpHeader& _header)
{
	try
	{
		UserService userService;

		auto token = userService.Login(_params["username"].ToString(), _params["password"].ToString());
			
		if(token.has_value())
			return web::Json(*token);
		else
			return web::Json("");
	}
	catch(std::logic_error _ex)
	{
		std::cout << _ex.what() << std::endl;
		return web::Json("");
	}
}

web::HttpResponse UserInfo(const web::UrlParam& _params, const web::HttpHeader& _header)
{
	try
	{
		UserService userService;

		auto user = userService.GetUser(_params["token"].ToString());
		if(user.has_value())
		{
			web::JsonObj temp;

			temp["name"] = user->name;
			return web::Json(temp);
		}
		else
		{
			return web::Json("error:token not exists");
		}
	}
	catch(std::logic_error _ex)
	{
		return web::Json(_ex.what());
	}
}

web::HttpResponse Register(const web::UrlParam& _params, const web::HttpHeader& _header)
{
	try
	{
		UserService userService;

		userService.Register(_params["username"].ToString(), _params["password"].ToString());

		return web::Json("注册成功!");
	}
	catch(std::logic_error _ex)
	{
		return web::Json(_ex.what());
	}
}

class Chat
{
private:
	std::unordered_map<web::Websocket*, std::string> tokenMap;
	ChatService chatService;
	UserService userService;

public:
	void ChatConnect(web::Websocket* _websocket,const web::HttpHeader& _header)
	{
		this->tokenMap.insert(std::pair<web::Websocket*, std::string>(_websocket, _header.GetCookie("token")));

		for(const auto& item: this->chatService.GetMessageHistory(this->chatService.GetWorldSession(), 10))
		{
			_websocket->SendText(std::string("msg ") + item.date + " " + item.name + " " + item.message);
		}
	}
	
	void ChatMessage(web::Websocket* _websocket, const char* _data, size_t _len)
	{
		if(_len == 0)
			return;

		auto user = this->userService.GetUser(this->tokenMap.at(_websocket));

		if(!user.has_value())
		{
			_websocket->SendText("token failure");
			return;
		}

		auto result = this->chatService.SendMessage(this->chatService.GetWorldSession(), user->name, std::string(_data, _len));
		for(auto& item: this->tokenMap)
		{
			item.first->SendText(std::string("msg ") + result.date + " " + result.name + " " + result.message);
		}
	}
	
	void ChatDisconnect(web::Websocket* _websocket)
	{
		this->tokenMap.erase(_websocket);
		std::cout << "chat disconnect" << std::endl;
	}
};

int main(int _argc, char** _args)
{


	db::Database().UseDb("WarGameServer");
	Chat chat;

	if(_argc != 3)
	{
		std::cout << "args must be two. args[1] is ip args[2] is port." << std::endl;
		return -1;
	}

	std::unique_ptr<web::Router> router(std::make_unique<web::Router>());
	router->RegisterUrl("GET", "/", &::HomePage);
	router->RegisterUrl("GET", "/UserInfo", &::UserInfo);

	router->RegisterUrl("POST", "/Register", &::Register);
	router->RegisterUrl("POST", "/Login", &::Login);
	router->RegisterWebsocket("/chat", &Chat::ChatConnect, &Chat::ChatMessage, &Chat::ChatDisconnect, &chat);

	//daemon用于后台运行
	//第一个参数非0则不改变当前目录路径
	//第二个参数非0则不改变当前的输出流指向
	//if(daemon(1, 1) != 0)
	//{
	//	std::cout << "daemon failed" << std::endl;
	//	return -1;
	//}

	web::HttpServer server(std::move(router));
	server.UseSSL(false)
		.Listen(_args[1], std::atoi(_args[2]));

	//while(true)
	//{
	//	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	//}

	return 0;
}
