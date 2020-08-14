#include "rpc.h"

#include <thread>

void ListenProc(int _port)
{
	try
	{
		rpc::Server server;

		server.Listen("127.0.0.1", _port);
	}
	catch(std::runtime_error _ex)
	{
		std::cout << _ex.what() << std::endl;
	}
	catch(std::logic_error _ex)
	{
		std::cout << _ex.what() << std::endl;
	}
}

void ClientProc(int _port)
{
	rpc::Client client;

	client.Connect("127.0.0.1", _port);
	client.CallFunc("asd", {});
}

int main(int _argc, char** _args)
{
	const int port = 3333;

	std::thread proc(ListenProc, port);

	for(int i = 0; i < 10; i++)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		try
		{
			ClientProc(port);
		}
		catch(std::runtime_error _ex)
		{
			std::cout << _ex.what() << std::endl;

		}
	}

	proc.join();

	return 0;
}
