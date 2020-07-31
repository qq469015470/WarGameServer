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

	User Register(std::string_view _name, std::string_view _password)
	{
		User user{_name.data(), _password.data()};

		this->database.Add(this->tableName);

		return user;
	}
};
const std::string UserService::tableName = "UserInfo";
