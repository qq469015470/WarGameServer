#include <unordered_map>
#include <vector>
#include <stdexcept>
#include <chrono>

class RoomService
{
public:
	struct RoomInfo 
	{
		int64_t roomId;
		std::string roomName;
		std::string ipaddr;
	};

private:
	std::unordered_map<int64_t, RoomInfo> rooms;
	std::vector<RoomInfo*> roomsVec;

public:
	RoomInfo AddRoom(std::string _name, std::string _ip)
	{
		std::chrono::system_clock::duration d = std::chrono::system_clock::now().time_since_epoch();
		std::chrono::nanoseconds nan = std::chrono::duration_cast<std::chrono::nanoseconds>(d);

		std::hash<int64_t> idHash;

		RoomInfo temp;

		temp.roomId = idHash(nan.count());
		temp.roomName = std::move(_name);
		temp.ipaddr = _ip; 
	
		auto res = this->rooms.insert(std::pair<int64_t, RoomInfo>(temp.roomId, std::move(temp)));
		if(res.second == false)
			throw std::runtime_error("roomId Repeat");

		this->roomsVec.push_back(&(res.first->second));

		return res.first->second;
	}

	std::vector<RoomInfo*> GetRooms()
	{
		return this->roomsVec;
	}

	void RemoveRoom(uint64_t _roomId)
	{
		auto roomIter = this->rooms.find(_roomId);

		if(roomIter == this->rooms.end())
			throw std::logic_error("roomId not exists");

		for(auto iter = this->roomsVec.begin(); iter != this->roomsVec.end(); iter++)
		{
			if(*iter == &roomIter->second)
			{
				this->roomsVec.erase(iter);
				this->rooms.erase(roomIter);
				return;
			}	
		}

		throw std::logic_error("roomVec not compare");
	}
};
