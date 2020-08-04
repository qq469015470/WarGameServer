#pragma once

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

namespace db
{
	using bsoncxx::builder::basic::kvp;
	using bsoncxx::builder::basic::make_array;
	using bsoncxx::builder::basic::make_document;
	//using bsoncxx::builder::stream::document;
	//using bsoncxx::builder::stream::finalize;
	
	class Entity
	{
	private:
		mongocxx::collection collection;
		std::optional<bsoncxx::document::value> value;
		bsoncxx::oid oid;
		
	public:
		Entity(mongocxx::collection _collection, bsoncxx::oid _oid):
			collection(std::move(_collection)),
			oid(_oid)
		{
	
		}
	
		Entity(mongocxx::collection _collection, bsoncxx::document::value _value):
			collection(std::move(_collection)),
			value(_value),
			oid(this->value->view()["_id"].get_oid().value)
		{
			
		}
	
		template<typename T>
		Entity& Update(std::string _key, const T _value)
		{
			mongocxx::options::update options;
	
			options.upsert(true);
	
			collection.update_one
			(
				make_document(kvp("_id", this->oid)),
				make_document(kvp("$set", make_document(kvp(std::move(_key), _value)))),
				options	
			);
	
			this->value.reset();
			
			return *this;	
		}
	
		bsoncxx::document::view GetView()
		{
			if(!this->value.has_value())
			{
				this->value = *this->collection.find_one(make_document(kvp("_id", this->oid)));
			}
	
			return this->value->view();
		}
			
	};
	
	class Querier
	{
	private:
		mongocxx::collection collection;
		bsoncxx::builder::basic::document builder;
	
		template<typename T>
		Querier& VaildEqual(std::string _key, const T& _value)
		{
			this->builder.append(kvp(_key, _value));
	
			return *this;
		}
	
	public:
		Querier(mongocxx::collection _collection):
			collection(std::move(_collection))
		{
	
		}
	
		
		template<typename T>
		Querier& Equal(std::string _key, const T& _value)
		{
			static_assert(sizeof(T) == -1, "value type not exists");
			return *this;
		}
	
		Querier& Equal(std::string _key, const char* _value)
		{
			return this->VaildEqual(_key, bsoncxx::types::b_utf8(_value));
		}
	
		Querier& Equal(std::string _key, const std::string& _value)
		{
			return this->VaildEqual(_key, bsoncxx::types::b_utf8(_value));
		}
	
		Querier& Equal(std::string _key, const bsoncxx::types::b_utf8& _value)
		{
			return this->VaildEqual(_key, _value);
		}
	
		Querier& Equal(std::string _key, const bsoncxx::oid& _value)
		{
			return this->VaildEqual(_key, _value);
		}
	
		//std::vector<Entity> Find()
		//{
		//	mongocxx::cursor cursor = this->collection.find(this->BuildDocument());
		//	std::vector<Entity> result;
		//	for(auto pos: cursor)
		//	{
		//		result.push_back(Entity(this->collection, *pos));
		//	}
		//	return result;
		//}
	
		std::optional<Entity> FindOne()
		{
			bsoncxx::stdx::optional<bsoncxx::document::value> value(this->collection.find_one(this->builder.extract()));
		
			std::optional<Entity> result;
	
			if(!value)
				return result;
	
			result = Entity(this->collection, *value);
	
			return result;
		}	
	};
	
	class Table
	{
	private:
		mongocxx::collection collection;
	
	public:
		Table(mongocxx::collection _collection):
			collection(_collection)
		{
	
		}
	
		Querier Query()
		{
			return Querier(this->collection);
		}
	
	
		Entity Add()
		{
			auto result = this->collection.insert_one({});
	
			if(!result)
				throw std::runtime_error("mongo insert failed");
	
			if(result->inserted_id().type() == bsoncxx::type::k_oid)
			{
				//std::cout << result->inserted_id().get_oid().value.to_string() << std::endl;
				return Entity(this->collection, result->inserted_id().get_oid().value);
			}
			else
			{
				throw std::runtime_error("Inserted id was not an OID type");
			}
		}
	};
	
	class Database
	{
	private:
		static mongocxx::instance instance;
		static mongocxx::pool pool;
		static std::string dbname;
	
	public:
		Database()
		{
	
		}

		Database& UseDb(std::string _dbname)
		{
			this->dbname = _dbname;
			return *this;
		}
	
		std::optional<Table> GetTable(std::string_view _tableName)
		{
			mongocxx::pool::entry client = this->pool.acquire();
			mongocxx::database db = (*client)[this->dbname.c_str()];
	
			if(!db.has_collection(_tableName.data()))
				return {};
	
			mongocxx::collection col = db[_tableName.data()];
	
			return Table(col);
		}

		Table CreateTable(std::string_view _tableName)
		{
			mongocxx::pool::entry client = this->pool.acquire();
			mongocxx::database db = (*client)[this->dbname.c_str()];

			if(db.has_collection(_tableName.data()))
				throw std::logic_error("table had create");

			return Table(db.create_collection(_tableName.data()));
		}
	};
	mongocxx::instance Database::instance = {};
	mongocxx::pool Database::pool = mongocxx::pool{mongocxx::uri{}};
	std::string Database::dbname = "testdb";
}
