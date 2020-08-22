#include "../GameCore/GameCore.h"
#include "../UserService.h"

#include "../web.h"

#include <chrono>
#include <unordered_map>
#include <mutex>

class Chat
{
private:
	GameScene scene;
	UserService userService;

	std::unordered_map<std::string, ISnake*> snakeMap;
	std::unordered_map<web::Websocket*, std::string> tokenMap;
	std::unordered_map<ISnake*, User> userMap;

	//线程锁
	std::mutex sceneMtx;

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
				std::lock_guard lg(this->sceneMtx);

				ISnake* snake(this->scene.AddSnake(0));
				this->snakeMap.insert(std::pair<std::string, ISnake*>(_token.data(), snake));
				this->tokenMap.insert(std::pair<web::Websocket*, std::string>(_websocket, _token.data()));
				this->userMap.insert(std::pair<ISnake*, User>(snake, *user));
			}
		}
	}


	inline void Logout(web::Websocket* _websocket)
	{
		std::lock_guard lg(this->sceneMtx);

		auto tokenIter = this->tokenMap.find(_websocket);
		if(tokenIter == this->tokenMap.end())
		{
			return;
		}

		ISnake* snakePtr = this->snakeMap.at(tokenIter->second);

		this->userMap.erase(snakePtr);
		this->snakeMap.erase(tokenIter->second);
		this->tokenMap.erase(_websocket);

		this->scene.RemoveSnake(snakePtr);
	}

	inline void PlayerControl(web::Websocket* _websocket, glm::vec2 _pos)
	{
		const std::string& token = this->tokenMap.at(_websocket);
		ISnake* const snake = this->snakeMap.at(token);

		snake->Move(_pos);
	}

	std::vector<char> PackGameInfo()
	{
		std::vector<char> info;
	
		//发送游戏信息结构
		//char\0结尾
		//uint16位为全部蛇(玩家)数量
		//之后的为蛇身信息
		//
		//蛇身信息为
		//char字符串遇到\0则结束
		//uint16 body数量(非字节数)
		//每个body
		//含有两个float32 分别为x,y坐标
	
		//添加蛇的信息	

		const auto snakes = this->scene.GetSnakes();
		uint16_t temp(snakes.size());
		const char cmd[] = "update";

		info.insert(info.end(), cmd, cmd + sizeof(cmd));	
		info.insert(info.end(), reinterpret_cast<const char*>(&temp), reinterpret_cast<const char*>(&temp) + sizeof(temp));
	
		for(const auto& item: snakes)
		{
			const User& user = this->userMap.at(item);
			
			info.insert(info.end(), user.name.c_str(), user.name.c_str() + user.name.size() + 1);

			const auto& bodys = item->GetBodys();
	
			temp = bodys.size();
	
			info.insert(info.end(), reinterpret_cast<const char*>(&temp), reinterpret_cast<const char*>(&temp) + sizeof(temp));
			for(const auto& elem: bodys)
			{
				const glm::vec2 pos = elem->GetPosition();
	
				info.insert(info.end(), reinterpret_cast<const char*>(&pos.x), reinterpret_cast<const char*>(&pos.x) + sizeof(pos.x));
				info.insert(info.end(), reinterpret_cast<const char*>(&pos.y), reinterpret_cast<const char*>(&pos.y) + sizeof(pos.y));
			}
		}

		return info;
	}

	//发送信息到客户端
	void SendGameInfo()
	{
		std::vector<char> temp(this->PackGameInfo());

		for(const auto& item: this->tokenMap)
		{
        		item.first->SendByte(temp.data(), temp.size());
		}

	}

public:
	Chat()
	{
		using TYPE = std::remove_pointer<decltype(this)>::type; 

		std::function<void(const IItem*)> itemAdd = std::bind(&TYPE::SendAddItem, this, std::placeholders::_1);
		std::function<void(const IItem*)> itemRemove = std::bind(&TYPE::SendRemoveItem, this, std::placeholders::_1);

		this->scene.SetItemNotifity(itemAdd, itemRemove);
	}

	inline std::vector<char> PackAddItem(const IItem* _item)
	{
		//发送additem命令
		//格式解析:
		//char\0结尾字符串
		//uint64_t 道具uid
		//uint8_t 道具类型
		//float32 x位置
		//float32 y位置
		std::vector<char> info;
		const char cmd[] = "additem";
		const uint8_t type = _item->GetId();
		const uint64_t id = reinterpret_cast<uint64_t>(_item);

		info.insert(info.end(), cmd, cmd + sizeof(cmd));
		info.insert(info.end(), reinterpret_cast<const char*>(&type), reinterpret_cast<const char*>(&type) + sizeof(type));
		info.insert(info.end(), reinterpret_cast<const char*>(&id), reinterpret_cast<const char*>(&id) + sizeof(id));

		const glm::vec2 pos = _item->GetPosition();

		info.insert(info.end(), reinterpret_cast<const char*>(&pos.x), reinterpret_cast<const char*>(&pos.x) + sizeof(pos.x));
		info.insert(info.end(), reinterpret_cast<const char*>(&pos.y), reinterpret_cast<const char*>(&pos.y) + sizeof(pos.y));
		
		return info;
	}

	void SendAddItem(const IItem* _item)
	{
		std::vector<char> info(this->PackAddItem(_item));

		{
			std::lock_guard lg(this->sceneMtx);
			for(const auto& item: this->tokenMap)
			{
				const auto& websocket = item.first;

				websocket->SendByte(info.data(), info.size());
			}
		}	
	}

	void SendRemoveItem(const IItem* _item)
	{
		//发送rmitem命令
		//char\0结尾
		//uint64_t 道具uid

		std::vector<char> info;
		const char cmd[] = "rmitem";
		const uint64_t id = reinterpret_cast<uint64_t>(_item);

		info.insert(info.end(), cmd, cmd + sizeof(cmd));
		info.insert(info.end(), reinterpret_cast<const char*>(&id), reinterpret_cast<const char*>(&id) + sizeof(id));

		{
			std::lock_guard lg(this->sceneMtx);
			for(const auto& item: this->tokenMap)
			{
				const auto& websocket = item.first;


				websocket->SendByte(info.data(), info.size());
			}
		}	
	}

	void OnConnect(web::Websocket* _websocket, const web::HttpHeader& _header)
	{
		std::lock_guard lg(this->sceneMtx);
		for(const auto& item: this->scene.GetItems())
		{
			std::vector<char> temp(this->PackAddItem(item));

			_websocket->SendByte(temp.data(), temp.size());
		}	
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

		if(args[0] == "control")
		{
			if(args.size() == 3)
			{
				try
				{
					this->PlayerControl(_websocket, glm::vec2{std::stof(args[1]), std::stof(args[2])});
				}
				catch(...)
				{
					std::cout << "PlayerConrol failed" << std::endl;
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

	void GameLoop()
	{
		std::chrono::steady_clock::time_point past(std::chrono::steady_clock::now());
	
		while(true)
		{
			auto now = std::chrono::steady_clock::now();
			auto timespan = std::chrono::duration_cast<std::chrono::milliseconds>(now - past);
	
			constexpr float deltaTime = 1.0f / 32.0f;
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
	
			if(updateCount > 0)
			{
				this->SendGameInfo();
			}
		}
	}
};

void ListenProc(Chat* _chat)
{
	std::unique_ptr<web::Router> router(new web::Router());

	router->RegisterWebsocket("/chat", &Chat::OnConnect, &Chat::OnMessage, &Chat::OnDisconnect, _chat);

	web::HttpServer server(std::move(router));

	server.Listen("192.168.1.105", 9001);
}

int main(int _argc, char** _args)
{
	db::Database().UseDb("WarGameServer");

	Chat chat;

	std::thread serverProc(ListenProc, &chat);

	chat.GameLoop();

	return 0;
}
