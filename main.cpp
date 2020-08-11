#include "UserService.h"
#include "ChatService.h"

#include "web.h"

#include <stdlib.h>
#include <thread>
#include <chrono>

web::HttpResponse HomePage(const web::UrlParam& _params, const web::HttpHeader& _header)
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
		return web::Json(_ex.what());
	}
}

web::HttpResponse Register(const web::UrlParam& _params, const web::HttpHeader& _header)
{
	try
	{
		UserService userService;

		userService.Register(_params["username"].ToString(), _params["password"].ToString());

		return web::Json("ok");
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

	std::unique_ptr<web::Router> router(new web::Router());
	router->RegisterUrl("GET", "/", &HomePage);
	router->RegisterUrl("POST", "/Register", &Register);
	router->RegisterUrl("POST", "/Login", &Login);
	router->RegisterWebsocket("/chat", &Chat::ChatConnect, &Chat::ChatMessage, &Chat::ChatDisconnect, &chat); 

	web::HttpServer server(std::move(router));
	server.Listen(_args[1], std::atoi(_args[2]));

	while(true)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

	return 0;
}
