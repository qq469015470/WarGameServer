#include "../GameCore/GameCore.h"
#include "../UserService.h"

#include "../web.h"

#include <chrono>
#include <unordered_map>

struct SnakeBodyInfo
{
	float x;
	float y;	
};

GameScene scene;
std::vector<char> infos;

std::vector<char> GetInfo(std::vector<ISnake*> _snakes)
{
	std::vector<char> info;

	//发送游戏信息结构
	//uint16位为人数
	//之后的为蛇身信息
	
	uint16_t size(0);

	info.insert(info.end(), reinterpret_cast<char*>(&size), reinterpret_cast<char*>(&size) + sizeof(size));

	for(const auto& item: _snakes)
	{
		for(const auto& elem: item->GetBodys())
		{
			const glm::vec2 pos = elem->GetPosition();

			SnakeBodyInfo temp;

			temp.x = pos.x;
			temp.y = pos.y;

			info.insert(info.end(), reinterpret_cast<char*>(&temp), reinterpret_cast<char*>(&temp) + sizeof(temp));

			size++;
		}
	}

	std::copy(reinterpret_cast<char*>(&size), reinterpret_cast<char*>(&size) + sizeof(size), info.begin());

	return info;
}

void GameLoop()
{
	std::chrono::steady_clock::time_point past(std::chrono::steady_clock::now());

	while(true)
	{
		auto now = std::chrono::steady_clock::now();
		auto timespan = std::chrono::duration_cast<std::chrono::milliseconds>(now - past);

		constexpr float deltaTime = 1.0f / 20.0f;
		constexpr float deltaTimeMills = deltaTime * 1000;

		if(timespan.count() < deltaTimeMills || timespan.count() == 0)
			continue;

		past = now;

		int updateCount(timespan.count() / deltaTimeMills);

		past += std::chrono::milliseconds(static_cast<int>(updateCount * deltaTimeMills));

		for(int i = 0; i < updateCount; i++)
		{
			scene.FixedUpdate(deltaTime);	
		}

		::infos = GetInfo(scene.GetSnakes());
	}
}

class Chat
{
private:
	UserService userService;

	std::unordered_map<std::string, ISnake*> snakeMap;
	std::unordered_map<web::Websocket*, std::string> tokenMap;

	static std::vector<std::string> GetArgs(const std::string& _cmd)
	{
		std::string::size_type left(0);
		std::string::size_type right(0);
		std::vector<std::string> args;
		do
		{
			right = _cmd.find(" ", left);
			args.push_back(_cmd.substr(left, right - left));

			left = right + 1;

		} while(right != std::string::npos);

		return args;
	}

	inline void Login(std::string_view _token, web::Websocket* _websocket)
	{
		auto user = this->userService.GetUser(_token.data());

		if(!user.has_value())
		{
			_websocket->SendText("error:token failed!");
		}
		else
		{
			if(this->snakeMap.find("token") != this->snakeMap.end())
			{
				_websocket->SendText("error:repeat login!");
			}
			else
			{	
				this->snakeMap.insert(std::pair<std::string, ISnake*>(_token.data(), scene.AddSnake(0)));
				this->tokenMap.insert(std::pair<web::Websocket*, std::string>(_websocket, _token.data()));
			}
		}
	}

	inline void Logout(web::Websocket* _websocket)
	{
		auto tokenIter = this->tokenMap.find(_websocket);
		if(tokenIter == this->tokenMap.end())
		{
			return;
		}

		ISnake* snakePtr = this->snakeMap.at(tokenIter->second);

		scene.RemoveSnake(snakePtr);

		this->snakeMap.erase(tokenIter->second);
		this->tokenMap.erase(_websocket);
	}

	inline void PlayerControl(web::Websocket* _websocket, glm::vec2 _pos)
	{
		const std::string& token = this->tokenMap.at(_websocket);
		ISnake* const snake = this->snakeMap.at(token);

		snake->Move(_pos);
	}

public:
	void OnConnect(web::Websocket* _websocket, const web::HttpHeader& _header)
	{

	}

	void OnMessage(web::Websocket* _websocket, const char* _data, size_t _len)
	{
		std::string msg(_data, _len);
		std::vector<std::string> args(this->GetArgs(msg));

		if(args[0] == "login")
		{
			if(args.size() == 2)
				this->Login(args[1], _websocket);
		}

		if(this->tokenMap.find(_websocket) == this->tokenMap.end())
		{
			_websocket->SendText("error:token failed!");
			return;
		}

		
		if(args[0] == "update")
		{
			_websocket->SendByte(::infos.data(), ::infos.size());
		}
		else if(args[0] == "control")
		{
			if(args.size() == 3)
			{
				try
				{
					this->PlayerControl(_websocket, glm::vec2{std::stof(args[1]), std::stof(args[2])});
				}
				catch(...)
				{
				}
			}
		}
		else
		{
			_websocket->SendText("error:command not exists!");
		}
	}

	void OnDisconnect(web::Websocket* _websocket)
	{
		this->Logout(_websocket);
	}
};

void ListenProc()
{

	db::Database().UseDb("WarGameServer");

	Chat chat;
	std::unique_ptr<web::Router> router(new web::Router());

	router->RegisterWebsocket("/chat", &Chat::OnConnect, &Chat::OnMessage, &Chat::OnDisconnect, &chat);

	web::HttpServer server(std::move(router));

	server.Listen("192.168.1.105", 9001);
}

int main(int _argc, char** _args)
{
	std::thread serverProc(ListenProc);

	GameLoop();

	return 0;
}
