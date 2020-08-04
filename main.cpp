#include "UserService.h"
#include "ChatService.h"

#include "web.h"

#include <stdlib.h>
#include <thread>
#include <chrono>

web::HttpResponse HomePage(const web::UrlParam& _params)
{
	return web::View("home/index.html");
}

web::HttpResponse Register(const web::UrlParam& _params)
{
	UserService userService;

	userService.Register(_params["username"].ToString(), _params["password"].ToString());

	userService.Login(_params["username"].ToString(), _params["password"].ToString());

	return web::Json("ok");
}

int main(int _argc, char** _args)
{
	db::Database().UseDb("wargameServer");

	if(_argc != 3)
	{
		std::cout << "args must be two. args[1] is ip args[2] is port." << std::endl;
		return -1;
	}

	std::unique_ptr<web::Router> router(new web::Router());
	router->RegisterUrl("GET", "/", &HomePage);
	router->RegisterUrl("POST", "/Register", &Register);

	web::HttpServer server(std::move(router));
	server.Listen(_args[1], std::atoi(_args[2]));

	while(true)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

	return 0;
}
