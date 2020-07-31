//mongocxx::instance inst{};
//mongocxx::client conn{mongocxx::uri{}};
//                                                     
//bsoncxx::builder::stream::document document{};
//                                                     
//auto collection = conn["testdb"]["testcollection"];
//document << "hello" << "world";
//                                                     
//collection.insert_one(document.view());
//auto cursor = collection.find({});
//                                                     
//for (auto&& doc : cursor) {
//    std::cout << bsoncxx::to_json(doc) << std::endl;
//}

#include <mongocxx/client.hpp>
#include <mongocxx/stdx.hpp>
#include <mongocxx/pool.hpp>
#include <mongocxx/uri.hpp>
#include <mongocxx/instance.hpp>

class Database
{
private:
	static mongocxx::instance instance;
	static mongocxx::pool pool;
	static const std::string dbname;

public:
	Database()
	{

	}	

	void Add(std::string_view _tableName)
	{
		auto client = this->pool.acquire();
		(*client)[this->dbname.c_str()][_tableName.data()].insert_one({});
	}
};
mongocxx::instance Database::instance = {};
mongocxx::pool Database::pool = mongocxx::pool{mongocxx::uri{}};
const std::string Database::dbname = "testdb";
