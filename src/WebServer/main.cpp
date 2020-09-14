#include "web.h"

web::HttpResponse Home(const web::UrlParam& _params, const web::HttpHeader& _header)
{
	return web::View("home/index.html");
}

int main(int _argc, char** _args)
{
	if(_argc != 3)
	{
		std::cout << "args must be two. args[1] is ip args[2] is port." << std::endl;
		return -1;
	}

	std::unique_ptr<web::Router> router(std::make_unique<web::Router>());
	router->RegisterUrl("GET", "/", &::Home);
	
	web::HttpServer server(std::move(router));

	server.Listen(_args[1], std::atoi(_args[2]));

	return 0;
}
