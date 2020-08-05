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
	UserService userService;

	auto token = userService.Login(_params["username"].ToString(), _params["password"].ToString());
		
		if(token.has_value())
		return web::Json(*token);
	else
		return web::Json("");
}

web::HttpResponse Register(const web::UrlParam& _params, const web::HttpHeader& _header)
{
	UserService userService;

	userService.Register(_params["username"].ToString(), _params["password"].ToString());

	return web::Json("ok");
}

void ChatConnect(web::Websocket* _websocket)
{
	std::cout << "chat connect" << std::endl;
	_websocket->SendText("hello client");
}

void ChatMessage(web::Websocket* _websocket, const char* _data, size_t _len)
{

}

void ChatDisconnect(web::Websocket* _websocket)
{
	std::cout << "chat disconnect" << std::endl;
}

int main(int _argc, char** _args)
{
	db::Database().UseDb("WarGameServer");

	if(_argc != 3)
	{
		std::cout << "args must be two. args[1] is ip args[2] is port." << std::endl;
		return -1;
	}

	std::unique_ptr<web::Router> router(new web::Router());
	router->RegisterUrl("GET", "/", &HomePage);
	router->RegisterUrl("POST", "/Register", &Register);
	router->RegisterUrl("POST", "/Login", &Login);
	router->RegisterWebsocket("/chat", &ChatConnect, &ChatMessage, &ChatDisconnect); 

	web::HttpServer server(std::move(router));
	server.Listen(_args[1], std::atoi(_args[2]));

	while(true)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

	return 0;
}
