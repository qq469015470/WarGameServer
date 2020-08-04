#pragma once

#include "Database.h"

class ChatService
{
private:
	static db::Database database;
	static const std::string tableName; 

public:
	//添加会话
	std::string AddSession()
	{
		bsoncxx::oid session;

		auto table = this->database.CreateTable("Chat" + session.to_string());
	
		return session.to_string();
	}

	void SendMessage(std::string_view _session, std::string _userName, std::string _message)
	{
		auto table = this->database.GetTable(this->tableName + _session.data());

		if(!table.has_value())
			throw std::logic_error("session not exists");

		table->Add()
			.Update("name", _userName)
			.Update("message", _message);
	}
};
db::Database ChatService::database = {};
const std::string ChatService::tableName = "Chat";
