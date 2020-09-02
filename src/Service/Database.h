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
	
	class EntityQuerier
	{
	private:
		mongocxx::pool::entry entry;
		mongocxx::collection collection;
		bsoncxx::builder::basic::document builder;
		mongocxx::options::find options;
	
		template<typename T>
		EntityQuerier& VaildEqual(std::string _key, const T& _value)
		{
			this->builder.append(kvp(_key, _value));
	
			return *this;
		}
	
	public:
		EntityQuerier(mongocxx::pool* _pool, std::string _dbname, std::string _colname):
			entry(_pool->acquire()),
			collection((*this->entry)[_dbname][_colname])
		{
	
		}
	
		
		template<typename T>
		EntityQuerier& Equal(std::string _key, const T& _value)
		{
			static_assert(sizeof(T) == -1, "value type not exists");
			return *this;
		}
	
		EntityQuerier& Equal(std::string _key, const char* _value)
		{
			return this->VaildEqual(_key, bsoncxx::types::b_utf8(_value));
		}
	
		EntityQuerier& Equal(std::string _key, const std::string& _value)
		{
			return this->VaildEqual(_key, bsoncxx::types::b_utf8(_value));
		}
	
		EntityQuerier& Equal(std::string _key, const bsoncxx::types::b_utf8& _value)
		{
			return this->VaildEqual(_key, _value);
		}
	
		EntityQuerier& Equal(std::string _key, const bsoncxx::oid& _value)
		{
			return this->VaildEqual(_key, _value);
		}

		EntityQuerier& LessThan(std::string _key, const bsoncxx::types::b_date& _value)
		{
			this->builder.append(kvp(_key, make_document(kvp("$lt", _value))));

			return *this;
		}

		EntityQuerier& Limit(uint64_t _limit)
		{
			this->options.limit(_limit);

			return *this;
		}

		EntityQuerier& Sort(std::string _key, int _value)
		{
			this->options.sort(make_document(kvp(_key, _value)));

			return *this;
		}
	
		std::vector<Entity> Find()
		{
			mongocxx::cursor cursor = this->collection.find(this->builder.extract(), this->options);
			std::vector<Entity> result;
			for(auto pos: cursor)
			{
				result.push_back(Entity(this->collection, bsoncxx::document::value(pos)));
			}
			return result;
		}
	
		std::optional<Entity> FindOne()
		{
			bsoncxx::stdx::optional<bsoncxx::document::value> value(this->collection.find_one(this->builder.extract(), this->options));
		
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
		mongocxx::pool* pool;
		mongocxx::pool::entry entry;
		std::string dbname;
		std::string colname;
		mongocxx::collection collection;
	
	public:
		Table(mongocxx::pool* _pool, std::string _dbname, std::string _colname):
			pool(_pool),
			entry(_pool->acquire()),
			dbname(_dbname),
			colname(_colname),
			collection((*this->entry)[_dbname][_colname])
		{
	
		}
	
		EntityQuerier Query()
		{
			return EntityQuerier(this->pool, this->dbname, this->colname);
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

	class TableQuerier
	{
	private:
		mongocxx::pool* pool;
		mongocxx::pool::entry entry;
		std::string dbname;
		bsoncxx::builder::basic::document builder;

		template<typename T>
		TableQuerier& VaildEqual(std::string _key, const T& _value)
		{
			this->builder.append(kvp(_key, _value));
		                                                             
			return *this;
		}

	public:
		TableQuerier(mongocxx::pool* _pool, std::string _dbname):
			pool(_pool),
			entry(this->pool->acquire()),
			dbname(_dbname)
		{

		}

		template<typename T>
		TableQuerier& Equal(std::string _key, const T& _value)
		{
			static_assert(sizeof(T) == -1, "value type not exists");
			return *this;
		}
		                                                                                                                     
		TableQuerier& Equal(std::string _key, const char* _value)
		{
			return this->VaildEqual(_key, bsoncxx::types::b_utf8(_value));
		}
		                                                                                                                     
		TableQuerier& Equal(std::string _key, const std::string& _value)
		{
			return this->VaildEqual(_key, bsoncxx::types::b_utf8(_value));
		}
		                                                                                                                     
		TableQuerier& Equal(std::string _key, const bsoncxx::types::b_utf8& _value)
		{
			return this->VaildEqual(_key, _value);
		}
		                                                                                                                     
		TableQuerier& Equal(std::string _key, const bsoncxx::oid& _value)
		{
			return this->VaildEqual(_key, _value);
		}
		                                                                                                                     
		std::optional<Table> FindOne()
		{
			mongocxx::database db((*this->entry)[this->dbname]);

			auto cursor = db.list_collections(this->builder.extract());

			std::optional<Table> result;
		                                                                                                                     
			if(cursor.begin() == cursor.end())
				return {};
		                                                 
			std::string colname((*cursor.begin())["name"].get_utf8().value.to_string());
			result = std::move(Table(this->pool, this->dbname, colname));
		                                                                                                                     
			return result;
		}	
	};

	class Database
	{
	private:
		static mongocxx::instance instance;
		static mongocxx::pool pool;
		static std::string dbname;

		mongocxx::pool::entry entry;
		mongocxx::client* client;
	
	public:
		Database():
			entry(this->pool.acquire()),
			client(&*this->entry)
		{
	
		}

		Database& UseDb(std::string _dbname)
		{
			this->dbname = _dbname;
			return *this;
		}

		TableQuerier Query()
		{
			mongocxx::database db = (*this->client)[this->dbname];

			return TableQuerier(&this->pool, this->dbname);	
		}

		Table CreateTable(std::string_view _tableName)
		{
			mongocxx::database db = (*this->client)[this->dbname.c_str()];

			if(db.has_collection(_tableName.data()))
				throw std::logic_error("table had create");

			return Table(&this->pool, this->dbname, _tableName.data());
		}
	};
	mongocxx::instance Database::instance = {};
	//默认链接本地mongodb
	mongocxx::pool Database::pool = mongocxx::pool{mongocxx::uri{"mongodb://localhost:27017"}};
	std::string Database::dbname = "testdb";
}
