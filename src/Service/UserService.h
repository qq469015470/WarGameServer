#pragma once

#include "Database.h"

#include <string>

struct User
{
	std::string name;
	std::string password;
};

class UserService
{
private:
	db::Table table;

public:
	UserService():
		table(std::move(*db::Database().Query().Equal("name", "UserInfo").FindOne()))
	{
	}

	User Register(std::string _name, std::string _password)
	{
		if(_name.size() == 0 || _name.size() > 20)
			throw std::logic_error("username must be 0 - 20 length");
		if(_password.size() == 0 || _password.size() > 20)
			throw std::logic_error("password must be 0 - 20 length");

		auto result = this->table.Query()
					.Equal("name", bsoncxx::types::b_utf8(_name))
					.FindOne();

		if(result)
			throw std::logic_error("username had registered");


		User user{std::move(_name), std::move(_password)};

		this->table.Add()
			.Update("name", user.name)
			.Update("password", user.password);

		return user;
	}

	std::optional<std::string> Login(std::string _name, std::string _password)	
	{
		//auto cursor = this->database.Query(this->tableName)
		//				.Equal("name", _name)
		//				.Equal("password", _password)
		//				.Find();
		//
		//for(auto doc: cursor)
		//{
		//      std::cout << bsoncxx::to_json(doc) << std::endl;

		//      std::cout << doc["name"].get_utf8().value.to_string() << std::endl;
		//}

		auto result = this->table.Query()
		  			.Equal("name", _name)
		  			.Equal("password", _password)
					.FindOne();

		if(!result)
			return {};

		result->Update("token", bsoncxx::oid());	

		return result->GetView()["token"].get_oid().value.to_string();
	}

	std::optional<User> GetUser(std::string _token)
	{
		auto result = this->table.Query()
					.Equal("token", bsoncxx::oid(_token))
					.FindOne();

		if(!result)
			return {};

		return User{result->GetView()["name"].get_utf8().value.to_string(), result->GetView()["password"].get_utf8().value.to_string()};
	}
};
