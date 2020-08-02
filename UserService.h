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
	Database database;
	static const std::string tableName;
	
public:
	UserService()
	{

	}

	User Register(std::string _name, std::string _password)
	{
		User user{std::move(_name), std::move(_password)};

		this->database.Add(this->tableName)
				.Update("name", user.name)
				.Update("password", user.password);

		return user;
	}

	User Login(std::string _name, std::string _password)	
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

		auto result = this->database.Query(this->tableName)
		  				.Equal("name", _name)
		  				.Equal("password", _password)
						.FindOne();

		User user
		{
			result->view()["name"].get_utf8().value.to_string(),
			result->view()["password"].get_utf8().value.to_string()
		};

		std:: cout << "user:" << user.name << " password:" << user.password << std::endl;

		return user;
	}
};
const std::string UserService::tableName = "UserInfo";
