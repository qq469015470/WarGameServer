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

#include <bsoncxx/json.hpp>

#include <mongocxx/client.hpp>
#include <mongocxx/stdx.hpp>
#include <mongocxx/pool.hpp>
#include <mongocxx/uri.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/options/update.hpp>
//#include <bsoncxx/builder/stream/document.hpp>

#include <iostream>
#include <stdexcept>
#include <string>
#include <queue>

using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_array;
using bsoncxx::builder::basic::make_document;
//using bsoncxx::builder::stream::document;
//using bsoncxx::builder::stream::finalize;

class Entity
{
private:
	mongocxx::collection collection;
	bsoncxx::oid oid;
	
public:
	Entity(mongocxx::collection _collection, bsoncxx::oid _oid):
		collection(std::move(_collection)),
		oid(_oid)
	{

	}

	template<typename T>
	Entity& Update(std::string _key, const T& _value)
	{
		mongocxx::options::update options;

		options.upsert(true);

		collection.update_one
		(
			make_document(kvp("_id", this->oid)),
			make_document(kvp("$set", make_document(kvp(std::move(_key), _value)))),
			options	
		);
		
		return *this;	
	}
		
};

class Querier
{
private:
	mongocxx::collection collection;
	std::queue<std::string> documents;	

	bsoncxx::document::value BuildDocument()
	{
		bsoncxx::builder::basic::document builder{};	
		while(!this->documents.empty())
		{
			const auto key = this->documents.front();
			this->documents.pop();
			const auto value = this->documents.front();
			this->documents.pop();
		                                                    
			builder.append(kvp(key, value));
		}


		return builder.extract();
	}
	
public:
	Querier(mongocxx::collection _collection):
		collection(std::move(_collection))
	{

	}

	Querier Equal(std::string _key, std::string _value)
	{
		this->documents.push(_key);
		this->documents.push(_value);
		
		return *this;
	}

	mongocxx::cursor Find()
	{
		return this->collection.find(this->BuildDocument());
	}

	bsoncxx::stdx::optional<bsoncxx::document::value> FindOne()
	{
		return this->collection.find_one(this->BuildDocument());
	}	
};

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

	Querier Query(std::string_view _tableName)
	{
		mongocxx::pool::entry client = this->pool.acquire();
		mongocxx::database db = (*client)[this->dbname.c_str()];
		mongocxx::collection col = db[_tableName.data()];

		return Querier(std::move(col));
	}	

	Entity Add(std::string_view _tableName)
	{
		mongocxx::pool::entry client = this->pool.acquire();
		mongocxx::database db = (*client)[this->dbname.c_str()];
		mongocxx::collection col = db[_tableName.data()];
		auto result = col.insert_one({});

		if(!result)
			throw std::runtime_error("mongo insert failed");

		if(result->inserted_id().type() == bsoncxx::type::k_oid)
		{
			//std::cout << result->inserted_id().get_oid().value.to_string() << std::endl;
			return Entity(col, result->inserted_id().get_oid().value);
		}
		else
		{
			throw std::runtime_error("Inserted id was not an OID type");
		}
	}
};
mongocxx::instance Database::instance = {};
mongocxx::pool Database::pool = mongocxx::pool{mongocxx::uri{}};
const std::string Database::dbname = "testdb";
