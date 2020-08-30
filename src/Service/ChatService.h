#pragma once

#include "Database.h"

#include <chrono>
#include <sstream>
#include <iomanip>

struct ChatInfo 
{
	std::string name;
	std::string message;
	std::string date;
};

class ChatService
{
private:
	static db::Database database;
	static const std::string tableName;

	static inline std::string ToDateTime(std::chrono::system_clock::time_point _tp)
	{
		std::time_t now_c = std::chrono::system_clock::to_time_t(_tp);
		std::tm now_tm = *std::localtime(&now_c);

		std::ostringstream ss;
		                                                                      
		ss << 1900 + now_tm.tm_year
			<< "/" << now_tm.tm_mon + 1
		       	<< "/" << now_tm.tm_mday
		       	<< " " << std::setfill('0') << std::setw(2) << now_tm.tm_hour
		       	<< ":" << std::setfill('0') << std::setw(2) << now_tm.tm_min
		       	<< ":" << std::setfill('0') << std::setw(2) << now_tm.tm_sec;
		
		return ss.str();
	}

public:
	static inline std::string GetWorldSession()
	{
		return "World";
	}

	//添加会话
	std::string AddSession()
	{
		bsoncxx::oid session;

		auto table = this->database.CreateTable("Chat" + session.to_string());
	
		return session.to_string();
	}

	ChatInfo SendMessage(std::string_view _session, std::string _userName, std::string _message)
	{
		auto table = this->database.Query()
					.Equal("name", this->tableName + _session.data())
					.FindOne();

		if(!table.has_value())
			throw std::logic_error("session not exists");

		bsoncxx::types::b_date b_date{std::chrono::system_clock::now()};

		table->Add()
			.Update("name", _userName)
			.Update("message", _message)
			.Update("time", b_date);

		ChatInfo result;

		result.name = std::move(_userName);
		result.message = std::move(_message);
		result.date = this->ToDateTime(b_date);

		return result;
	}

	std::vector<ChatInfo> GetMessageHistory(std::string_view _session, int _count)
	{
		auto table = this->database.Query()
						.Equal("name", this->tableName + _session.data())
						.FindOne();

		if(!table.has_value())
		{
			throw std::logic_error("session not exists");
		}

		auto result = table->Query()
					.LessThan("time", bsoncxx::types::b_date(std::chrono::system_clock::now()))
					.Sort("time", -1)
					.Limit(_count)
					.Find();

		std::vector<ChatInfo> chats;

		chats.reserve(result.size());

		for(auto iter = result.rbegin(); iter != result.rend(); iter++)
		{
			auto item = *iter;
			auto view = item.GetView();

			ChatInfo temp;

			temp.name = view["name"].get_utf8().value.to_string();
			temp.message = view["message"].get_utf8().value.to_string();

			temp.date = this->ToDateTime(view["time"].get_date());

			chats.emplace_back(std::move(temp));
		}
		
		return chats;
	}
};
db::Database ChatService::database = {};
const std::string ChatService::tableName = "Chat";
