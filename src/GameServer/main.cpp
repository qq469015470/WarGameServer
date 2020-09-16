#include "../GameCore/GameCore.h"
#include "../Service/UserService.h"

#include "web.h"

#include <chrono>
#include <unordered_map>
#include <mutex>
#include <condition_variable>

class Chat
{
private:
	struct UserData
	{
		User user;
		std::string token;
		web::Websocket* websocket;
		ISnake* snake;
	};

	//token关联数据
	std::unordered_map<std::string, UserData> userMap;
	std::unordered_map<web::Websocket*, UserData*> websocketMap;
	std::unordered_map<ISnake*, UserData*> snakeMap;

	//等待初始化的websocket
	std::queue<UserData*> loginQueue;
	std::queue<ISnake*> logoutQueue;
	//线程锁
	std::mutex loginMtx;
	std::condition_variable loginCV;
	bool isLoging;

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

	inline void Login(std::string _token, web::Websocket* _websocket)
	{
		std::lock_guard<std::mutex> lg(this->loginMtx);
		this->isLoging = true;

		web::HttpClient userServerClient;
		
		userServerClient.Connect("0.0.0.0", 9999);

		const web::HttpResponse response = userServerClient.SendRequest("GET", std::string("/UserInfo?token=" + _token));
		const std::string body = std::string(response.GetBody(), response.GetBodySize());

		if(body.find("error") != std::string::npos)
		{
			_websocket->SendText("error:token failed!");
		}
		else
		{
			web::JsonObj jsonObj(web::JsonObj::ParseJson(body));

			if(this->userMap.find(_token.data()) != this->userMap.end())
			{
				_websocket->SendText("error:repeat login!");
			}
			else
			{
				UserData temp;

				User user;

				user.name = jsonObj["name"].ToString();

				temp.user = std::move(user);
				temp.token = std::string(_token);
				temp.websocket = _websocket;
				temp.snake = nullptr;

				const auto result = this->userMap.insert(std::pair<std::string, UserData>(temp.token, std::move(temp)));
				this->websocketMap.insert(std::pair<web::Websocket*, UserData*>(_websocket, &result.first->second));
				this->loginQueue.push(&result.first->second);
			}
		}

		this->isLoging = false;
		this->loginCV.notify_all();
	}

	inline void Logout(web::Websocket* _websocket)
	{
		std::lock_guard<std::mutex> lg(this->loginMtx);
		this->isLoging = true;

		const auto websocketIter = this->websocketMap.find(_websocket);
		if(websocketIter == this->websocketMap.end())
		{
			return;
		}
		
		if(websocketIter->second->snake != nullptr)
			this->logoutQueue.push(websocketIter->second->snake);

		this->userMap.erase(websocketIter->second->token);
		const auto snakeIter = this->snakeMap.find(websocketIter->second->snake);

		this->snakeMap.erase(snakeIter);
		this->websocketMap.erase(websocketIter);

		this->isLoging = false;
		this->loginCV.notify_all();
	}

	inline void PlayerControl(web::Websocket* _websocket, glm::vec2 _pos)
	{
		const auto websocketIter = this->websocketMap.at(_websocket);
		ISnake* const snake = websocketIter->snake;

		snake->Move(_pos);
	}

	std::vector<char> PackGameInfo(const std::vector<ISnake*>& _snakes)
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

		uint16_t temp(_snakes.size());
		const char cmd[] = "update";

		info.insert(info.end(), cmd, cmd + sizeof(cmd));	
		info.insert(info.end(), reinterpret_cast<const char*>(&temp), reinterpret_cast<const char*>(&temp) + sizeof(temp));
	
		for(const auto& item: _snakes)
		{
			const User& user = this->snakeMap.at(item)->user;
			
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

public:
	Chat():
		isLoging(false)
	{
	}

	void SendAddItem(const IItem* _item)
	{
		std::vector<char> info(this->PackAddItem(_item));

		for(const auto& item: this->userMap)
		{
			web::Websocket* const websocket = item.second.websocket;

			websocket->SendByte(info.data(), info.size());
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

		for(const auto& item: this->userMap)
		{
			web::Websocket* const websocket = item.second.websocket;

			websocket->SendByte(info.data(), info.size());
		}
	}

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

		if(this->websocketMap.find(_websocket) == this->websocketMap.end())
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

	bool NeedInitClient()
	{
		return !this->loginQueue.empty();
	}

	void InitClient(ISnake* _newSnake, const std::vector<IItem*>& _items)
	{
		UserData* const temp = this->loginQueue.front();
		this->loginQueue.pop();

		//用户已经退出则返回
		if(this->userMap.find(temp->token) == this->userMap.end())
		{
			return;
		}

		temp->snake = _newSnake;


		this->snakeMap.insert(std::pair<ISnake*, UserData*>(temp->snake, temp));

		for(const auto& item: _items)
		{
			std::vector<char> itemData = this->PackAddItem(item);
			temp->websocket->SendByte(itemData.data(), itemData.size());
		}
	}

	bool ClientLogout()
	{
		return !this->logoutQueue.empty();
	}

	ISnake* ClearClient()
	{
		const auto result = this->logoutQueue.front();
		this->logoutQueue.pop(); 

		return result;
	}

	//发送信息到客户端
	void SendGameInfo(const std::vector<ISnake*>& _snakes)
	{
		std::unique_lock<std::mutex> ug(this->loginMtx);
		this->loginCV.wait(ug, [this] () {return !this->isLoging;});

		std::vector<char> temp(this->PackGameInfo(_snakes));

		for(const auto& item: this->userMap)
		{
        		item.second.websocket->SendByte(temp.data(), temp.size());
		}
	}
};

void GameLoop(GameScene& _scene, Chat& _chat, web::HttpServer& _server)
{
	using TYPE = std::remove_reference<decltype(_chat)>::type; 
	                                                                                                              
	std::function<void(const IItem*)> itemAdd = std::bind(&TYPE::SendAddItem, &_chat, std::placeholders::_1);
	std::function<void(const IItem*)> itemRemove = std::bind(&TYPE::SendRemoveItem, &_chat, std::placeholders::_1);	
	                                                                                                              
	_scene.SetItemNotifity(itemAdd, itemRemove);

	std::chrono::steady_clock::time_point past(std::chrono::steady_clock::now());
	//没有人3s后房间退出
	float timeout(3.0f);
	while(true)
	{
		auto now = std::chrono::steady_clock::now();
		auto timespan = std::chrono::duration_cast<std::chrono::milliseconds>(now - past);

		constexpr float deltaTime = 1.0f / 32.0f;
		constexpr float deltaTimeMills = deltaTime * 1000;
		
		while(_chat.NeedInitClient())
		{
			_chat.InitClient(_scene.AddSnake(0), _scene.GetItems());
		}
		while(_chat.ClientLogout())
		{
			_scene.RemoveSnake(_chat.ClearClient());
		}

		if(timespan.count() < deltaTimeMills || timespan.count() == 0)
			continue;

		//没有蛇则累计超时时间
		//否则重置
		if(_scene.GetSnakes().size() != 0)
			timeout = 2.0f;
		else
		{	
			timeout -= timespan.count() / 1000.0f;
			//超时则退出程序
			if(timeout <= 0)
				break;
		}

		past = now;

		int updateCount(timespan.count() / deltaTimeMills);

		past += std::chrono::milliseconds(static_cast<int>(updateCount * deltaTimeMills));

		for(int i = 0; i < updateCount; i++)
		{
			_scene.FixedUpdate(deltaTime);	
		}


		if(updateCount > 0)
		{
			_chat.SendGameInfo(_scene.GetSnakes());	
		}
	}
	_server.Stop();
}

void ListenProc(web::HttpServer& _server, const char* _ip, const int _port)
{
	_server.Listen(_ip, _port);
}

int main(int _argc, char** _args)
{
	if(_argc != 3)
	{
		std::cout << "args must be two. args[1] is ip args[2] is port." << std::endl;
		return -1;
	}

	db::Database().UseDb("WarGameServer");

	Chat chat;
	GameScene scene;

	std::unique_ptr<web::Router> router(new web::Router());

	router->RegisterWebsocket("/chat", &Chat::OnConnect, &Chat::OnMessage, &Chat::OnDisconnect, &chat);

	web::HttpServer server(std::move(router));

	std::thread serverProc(::ListenProc, std::ref(server), _args[1], std::atoi(_args[2]));

	::GameLoop(scene, chat, server);

	serverProc.join();

	return 0;
}
