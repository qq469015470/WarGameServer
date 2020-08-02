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
};
const std::string UserService::tableName = "UserInfo";
