#pragma once

#include <cstring>
#include <string>
#include <iostream>
#include <map>
#include <stack>
#include <typeinfo>
#include <stdexcept>
#include <memory>
#include <fstream>
#include <unordered_map>
#include <unordered_set>
#include <signal.h>
#include <bitset>
#include <vector>
#include <cassert>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <thread>
#include <functional>

#include <openssl/ssl.h>
#include <openssl/sha.h>
#include <openssl/err.h>

namespace web
{
	static std::string UrlDecode(std::string_view _urlCode)
	{
		std::string result;

		result.reserve(_urlCode.size());
		for(int i = 0; i < _urlCode.size(); i++)
		{
			if(_urlCode[i] == '%')
			{
				char hex[3];

				hex[0] = _urlCode[i + 1];
				hex[1] = _urlCode[i + 2];
				hex[2] = '\0';

				int buffer;
				
				sscanf(hex, "%x", &buffer);
				result += static_cast<char>(buffer);

				i += 2;
			}
			else
			{
				result += _urlCode[i];
			}
		}	

		return result;
	}

	//class UrlParam
	//{
	//private:
	//	std::optional<std::string> val;
	//	size_t arrSize;
	//	std::unordered_map<std::string, std::unique_ptr<UrlParam>> params;

	//	inline UrlParam& Find(std::string_view _key)
	//	{
	//		std::unique_ptr<UrlParam>& result(this->params[_key.data()]);
	//		
	//		if(result == nullptr)
	//		{
	//			result = std::make_unique<UrlParam>();
	//		}

	//		return *result.get();
	//	}

	//public:
	//	UrlParam():
	//		arrSize(0)
	//	{

	//	}

	//	const UrlParam& operator[](int _index) const
	//	{
	//		return *this->params.at(std::to_string(_index)).get();
	//	}

	//	UrlParam& operator[](int _index)
	//	{
	//		return this->Find(std::to_string(_index));
	//	}

	//	const UrlParam& operator[](std::string_view _key) const
	//	{
	//		return *this->params.at(_key.data()).get();
	//	}

	//	UrlParam& operator[](std::string_view _key)
	//	{
	//		return this->Find(_key);
	//	}

	//	UrlParam& operator=(std::string_view _value)
	//	{
	//		this->val = _value;
	//		return *this;
	//	}

	//	const std::string& ToString() const
	//	{
	//		if(!this->val.has_value())
	//		{
	//			throw std::runtime_error("not have val");
	//		}

	//		return *this->val;
	//	}

	//	//添加数组
	//	void PushBack(std::string_view _value)
	//	{
	//		std::string index(std::to_string(this->arrSize));

	//		UrlParam& temp(this->Find(std::to_string(this->arrSize)));

	//		temp[index] = _value;
	//		this->arrSize++;
	//	}

	//	size_t GetArraySize() const
	//	{
	//		return this->arrSize;
	//	}
	//};

	class JsonObj
	{
	private:
		enum JsonType
		{
			STRING,
			NUMBER,
			BOOL,
			NULLVAL,
			ARRAY,
			JSON
		};

		//自身的成员
		std::unordered_map<std::string, std::unique_ptr<JsonObj>> attrs;
		JsonType valType;
		std::optional<std::string> val;
		std::optional<std::vector<std::unique_ptr<JsonObj>>> arr;

		void DataValid()
		{
			if(this->val.has_value() && this->arr.has_value())
			{
				throw std::logic_error("can not set value both arr and value");
			}
		}

		JsonObj& operator=(const JsonObj& _jsonObj)
		{
			return *this;
		}



	public:
		JsonObj():
			valType(JsonType::NULLVAL)
		{

		}

		JsonObj(JsonObj&& _obj):
			attrs(std::move(_obj.attrs)),
			valType(std::move(_obj.valType)),
			val(std::move(_obj.val)),
			arr(std::move(_obj.arr))
		{
		}

		JsonObj& operator=(JsonObj&& _obj)
		{
			this->attrs = std::move(_obj.attrs);
			this->valType = std::move(_obj.valType);
			this->val = std::move(_obj.val);
			this->arr = std::move(_obj.arr);
			return *this;
		}

		static JsonObj ParseFormData(std::string _formData)
		{
			const std::string& json = _formData;

			std::string::size_type left(0);
			std::string::size_type right(0);

			right = json.find("=");

			JsonObj params;
			while(right != std::string::npos)
			{
				std::string key(json.substr(left, right - left));

				//提取[]内的成员
				std::string::size_type keyLeft(0);
				std::string::size_type keyRight(key.find("%5B"));

				JsonObj* curParam(&params[key.substr(keyLeft, keyRight)]);

				while(keyRight != std::string::npos)
				{
					keyLeft = keyRight + 3;
					keyRight = key.find("%5D", keyLeft);

					const std::string attr(key.substr(keyLeft, keyRight - keyLeft));

					if(attr != "")
						curParam = &(*curParam)[attr];
					else
					{
						curParam->Push("");
						curParam = &(*curParam)[curParam->GetArraySize() - 1];
					}

					keyLeft = keyRight + 3;
					keyRight = key.find("%5B", keyLeft);
				}
			

				left = right + 1;
				right = json.find("&", left);
				std::string value(web::UrlDecode(json.substr(left, right - left)));

				*curParam = value;

				if(right == std::string::npos)
					break;

				left = right + 1;
				right = json.find("=", left);
			}

			return params;
		}

		static void ParseJson(JsonObj& curJson, std::string _json)
		{
			static std::unordered_map<char, char> special =
			{
				{'{', '}'},
				{'[', ']'},
				{'"', '"'},
			};

			std::string::size_type pos(0);

			pos = _json.find_first_not_of(' ');
			auto iter = special.find(_json[pos]);
			if(iter == special.end())
			{
				curJson = std::stod(_json);
				return;
			}

			do
			{
				switch(iter->first)
				{
					case '"':
					{
						curJson = _json.substr(pos + 1, _json.find('"', pos + 1) - (pos + 1));
						return;
						break;
					}
					case '{':
					{
						std::string::size_type left(_json.find('"', pos) + 1);
						std::string::size_type right(_json.find('"', left + 1));

						const std::string key = _json.substr(left, right - left);

						left = _json.find(':', right + 1);
						left = _json.find_first_not_of(' ', left + 1);

						auto endChar = special.find(_json[left]);
						if(endChar != special.end())
							right = _json.find(endChar->second, left + 1) + 1;
						else
							right = _json.find(",", left + 1);

						const std::string value = _json.substr(left, right - left);

						//std::cout << "key:" << key << " value:" << value << std::endl;
						ParseJson(curJson[key], value);

						break;
					}
					case '[':
					{
						const std::string value = _json.substr(pos + 1, _json.find(']', pos + 1) - (pos + 1));
						std::string::size_type leftTemp(0);
						std::string::size_type rightTemp(std::string::npos);
					
						do
						{	
							auto endChar = special.find(value[leftTemp]);
							rightTemp = value.find(endChar->second, leftTemp + 1); 
							rightTemp = value.find(",", rightTemp + 1);
							const std::string arrayValue = value.substr(leftTemp, rightTemp - leftTemp);

							//std::cout << "[] value:" << arrayValue<< std::endl;
							curJson.Push("");
							JsonObj::ParseJson(curJson[curJson.GetArraySize() - 1], arrayValue);

							leftTemp = value.find_first_not_of(' ', rightTemp + 1);
						} while(rightTemp != std::string::npos);
						break;
					}
				}


				pos = _json.find(',', pos + 1);
			} while(pos != std::string::npos);
		}

		static JsonObj ParseJson(std::string _json)
		{
			JsonObj result;

			ParseJson(result, _json);
			
			return result;
		}

		JsonObj& operator[](std::string _key)
		{
			auto iter = this->attrs.find(_key); 
			if(iter == this->attrs.end())
			{
				iter = this->attrs.insert(std::pair<std::string, std::unique_ptr<JsonObj>>(_key, std::move(std::make_unique<JsonObj>()))).first;
			}

			return *(iter->second);
		}

		const JsonObj& operator[](std::string _key) const
		{
			return *this->attrs.at(_key).get();
		}

		JsonObj& operator[](int _index)
		{
			auto& obj = this->arr->at(_index); 

			return *obj.get();
		}

		const JsonObj& operator[](int _index) const
		{
			return *this->arr->at(_index).get();
		}

		JsonObj& operator=(const char* _value)
		{
			this->DataValid();
			this->val = std::string("\"") + std::string(_value) + "\"";
			this->valType = JsonType::STRING;

			return *this;
		}

		JsonObj& operator=(std::string_view _value)
		{
			this->DataValid();
			this->val = std::string("\"") + std::string(_value) + "\"";
			this->valType = JsonType::STRING;
			                                                            
			return *this;
		}

		JsonObj& operator=(int _value)
		{
			this->DataValid();
			this->val = std::to_string(_value);
			this->valType = JsonType::NUMBER;
		                                            
			return *this;
		}

		JsonObj& operator=(int64_t _value)
		{
			this->DataValid();
			this->val = std::to_string(_value);
			this->valType = JsonType::NUMBER;

			return *this;
		}

		JsonObj& operator=(double _value)
		{
			this->DataValid();
			this->val = std::to_string(_value);
			this->valType = JsonType::NUMBER;

			return *this;
		}
		
		JsonObj& operator=(bool _value)
		{
			this->DataValid();
			if(_value == true)
			{
				this->val = "true";
			}
			else
			{
				this->val = "false";
			}
			this->valType = JsonType::BOOL;

			return *this;
		}

		//JsonObj& operator=(const JsonObj& _obj)
		//{
		//	this->val = _obj.ToJson();
		//	this->valType = JsonType::JSON;

		//	return *this;
		//}

		void SetNull()
		{
			this->DataValid();
			this->val = "null";
			this->valType = JsonType::NULLVAL;
		}

		template<typename T>
		void Push(T _value)
		{
			this->DataValid();
			
			if(!this->arr.has_value())
				this->arr = std::vector<std::unique_ptr<JsonObj>>{};

			this->arr->push_back(std::make_unique<JsonObj>());
			*this->arr->back() = _value;
			this->valType = JsonType::ARRAY;
		}

		void Push(const JsonObj& _obj)
		{
			this->Push<const JsonObj&>(_obj);
		}

		size_t GetArraySize() const
		{
			return this->arr->size();
		}

		std::string ToString() const
		{
			if(this->valType != JsonType::STRING)
				return "";

			return this->val->substr(1,this->val->size() - 2);
		}

		double ToDouble() const
		{
			if(this->valType != JsonType::NUMBER)
				return std::numeric_limits<double>::infinity();

			return std::stod(this->val.value());
		}

		std::string ToJson() const
		{
			if(!this->val.has_value() && !this->arr.has_value() && this->attrs.size() == 0)
			{
				return "{}";
			}

			if(this->val.has_value())
			{
				return *this->val;	
			}
			else if(this->arr.has_value())
			{
				std::string res("[");

				for(const auto& item: this->arr.value())
				{
					res += item->ToJson() + ",";
				}

				res.back() = ']';

				return res;
			}
			else
			{
				std::string res("{");
				for(const auto& item: this->attrs)
				{
					res += "\"" + item.first  + "\": " + item.second->ToJson() + ",";
				}
				res.back() = '}';

				return res;
			}
		}
	};

	using UrlParam = JsonObj;

	class HttpAttr
	{
	private:
		std::string key;
		std::string value;

	public:
		HttpAttr(std::string _key, std::string _value):
			key(std::move(_key)),
			value(std::move(_value))
		{
		}

		const std::string& GetKey() const
		{
			return this->key;
		}

		const std::string& GetValue() const
		{
			return this->value;	
		}

	};

	class HttpHeader
	{
	private:
		size_t contentLength;

		std::unordered_map<std::string, HttpAttr> attrs;
		std::unordered_map<std::string, std::string> cookies;

		static inline const char* GetAttrValue(const std::unordered_map<std::string, HttpAttr>& _attrs, std::string_view _key)
		{
			auto iter = _attrs.find(_key.data());
			if(iter == _attrs.end())
			{
				return "";
			}
			else
			{
				return iter->second.GetValue().c_str();
			}
		}

		static std::unordered_map<std::string, HttpAttr> ReadAttr(std::string_view _content)
		{
			std::unordered_map<std::string, HttpAttr> attrs;
			std::string::size_type left(0);
			std::string::size_type right(_content.find("\r\n"));
			while(right != std::string::npos)
			{
				std::string_view temp(_content.substr(left, right - left));

				std::string::size_type pos = temp.find(":");
				if(pos == std::string::npos)
				{
					break;
				}

				std::string key(temp.substr(0, pos));
				std::string value(temp.substr(pos + 2, temp.size() - pos - 1));
				
				attrs.insert(std::pair<std::string, HttpAttr>(std::move(key), HttpAttr(key, std::move(value))));

				left = right + 2;
				right = _content.find("\r\n", left);
			}

			return attrs;
		}

		void ReadCookie(std::string_view _cookie)
		{
			std::string::size_type leftPos(0);
			std::string::size_type rightPos(_cookie.find(";"));
			                                                                
			while(_cookie.size() > 0)
			{
				std::string::size_type pos(_cookie.find("=", leftPos));
				std::string key(_cookie.substr(leftPos, pos - leftPos));
				//去掉开头空格
				key = key.substr(key.find_first_not_of(" "));

				pos++;
				std::string value(_cookie.substr(pos, rightPos - pos));
				
				this->cookies.insert(std::pair<std::string, std::string>(std::move(key), std::move(value)));				
			                                                                
				leftPos = rightPos + 1;
				if(rightPos == std::string::npos)
					break;

				rightPos = _cookie.find(";", leftPos);
			}
		}

	public:
		//传递Http头属性开始即首行的路径地址或状态码下一行开始
		HttpHeader(std::string_view _attrStr):
			attrs(this->ReadAttr(_attrStr)),
			contentLength(0)
		{
			const std::string temp = this->GetAttrValue(this->attrs, "Content-Length");
			if(!temp.empty())
				this->contentLength = std::stoll(temp);


			this->ReadCookie(this->GetAttrValue(this->attrs, "Cookie"));
		}

		HttpHeader(const std::vector<HttpAttr>& _attrs):
			contentLength(0)
		{
			for(const auto& item: _attrs)
			{
				this->attrs.insert(std::pair<std::string, HttpAttr>(item.GetKey(), item));
			}

			const std::string temp = this->GetAttrValue(this->attrs, "Content-Length");
			if(!temp.empty())
				this->contentLength = std::stoll(temp);
			
			this->ReadCookie(this->GetAttrValue(this->attrs, "Cookie"));
		}

		size_t GetContentLength() const	
		{
			return this->contentLength;
		}

		const char* GetConnection() const
		{
			return this->GetAttrValue(this->attrs, "Connection");
		}
		
		const char* GetUpgrade() const
		{
			return this->GetAttrValue(this->attrs, "Upgrade");
		}

		const char* GetSecWebSocketKey() const
		{
			return this->GetAttrValue(this->attrs, "Sec-WebSocket-Key");
		}

		const char* GetCookie(std::string_view _key) const
		{
			auto iter = this->cookies.find(_key.data());
			if(iter == this->cookies.end())
				return "";
			else
				return iter->second.c_str(); 
		}
	
		std::vector<HttpAttr> GetHttpAttrs() const
		{
			std::vector<HttpAttr> attrs;
			for(const auto& item: this->attrs)
			{
				attrs.push_back(item.second);
			}

			return attrs;
		}
	};

	struct WebsocketData
	{
		bool fin;
		bool rsv1;
		bool rsv2;
		bool rsv3;
		unsigned short int opcode;
		bool mask;
		std::array<char, 4> maskingKey;
		std::vector<char> payload;
	};

	class HttpRequest
	{
	private:
		//请求类型(例:get post)
		std::string type;
		std::string url;
		std::string queryString;
		std::string version;
		std::optional<HttpHeader> header;
		std::vector<char> body;

		inline std::string::size_type ReadBasic(std::string_view _header)
		{
			std::string::size_type left = 0;
			std::string::size_type right = _header.find("\r\n");

			if(right == std::string::npos)
			{
				throw std::runtime_error("http reqeust not vaild!");
			}

			std::string_view line(_header.substr(left, right));

			//读取type
			right = line.find(" ");
			if(right == std::string::npos)
			{
				throw std::runtime_error("could not read type!");
			}
			this->type = line.substr(left, right - left);
			left = right + 1;

			//读取url
			right = line.find(" ", left);
			if(right == std::string::npos)
			{
				throw std::runtime_error("could not read url!");
			}
			this->url = line.substr(left, right - left);
			left = right + 1;
			//去掉地址?开头的参数
			right = this->url.find("?");
			if(right != std::string::npos)
			{
				this->queryString = this->url.substr(right + 1);
				this->url = this->url.substr(0, right);
			}
	

			//读取http协议版本
			right = line.size();
			if(left == right)
			{
				throw std::runtime_error("could not read version!");
			}
			this->version = line.substr(left, right);

			return right + 2;
		}

		inline void ReadBody(const char* _buffer, size_t _size)
		{
			this->body.resize(_size);
			std::copy(_buffer, _buffer + _size, this->body.data());	
		}

	public:
		HttpRequest(const char* _buffers)
		{
			const char* pos = strstr(_buffers, "\r\n\r\n");		
			
			if(pos == nullptr)
			{
				throw std::runtime_error("http request not vaild!");
			}

			const std::string headerStr(_buffers, pos + 4);

			const std::string::size_type headerStart = this->ReadBasic(headerStr);

			this->header = HttpHeader(headerStr.substr(headerStart));	

			size_t bodyLen = this->header->GetContentLength(); 

			this->ReadBody(pos + 4, bodyLen);
		}

		HttpRequest(std::string _type, std::string _url, std::vector<HttpAttr> _attrs, std::vector<char> _body):
			type(_type),
			url(_url.substr(0, _url.find("?"))),
			queryString(_url.substr(_url.find("?") + 1)),
			version("HTTP/1.1"),
			header(HttpHeader(std::move(_attrs))),
			body(std::move(_body))
		{
			
		}

		const std::string& GetType() const
		{
			return this->type;
		}

		const std::string& GetUrl() const
		{
			return this->url;
		}

		const std::string& GetQueryString() const
		{
			return this->queryString;
		}
		
		const char* GetBody() const
		{
			return this->body.data();
		}

		size_t GetBodyLen() const
		{
			return this->body.size();
		}

		const HttpHeader& GetHeader() const
		{
			return *this->header;
		}
	};

	class HttpResponse
	{
	private:
		int stateCode;
		std::vector<char> content;
		std::vector<char> body;

		static inline std::string GetSpec(int _code)
		{
			switch (_code)
			{
				case 101:
					return std::string("Switching Protocols");	
					break;
				case 200:
					return std::string("OK");
					break;
				case 404:
					return std::string("NOT FOUND");
					break;
				default:
					return std::string("NONE SPEC");
					break;
			}
		}

	public:
		HttpResponse(int _stateCode,const std::vector<HttpAttr>& _httpAttrs, const char* _body, size_t _bodyLen):
			stateCode(_stateCode),
			content({'\0'})
		{
			std::string header;
			
			header = "HTTP/1.1 " + std::to_string(_stateCode) + " " + HttpResponse::GetSpec(_stateCode) + "\r\n";

			if(_bodyLen > 0)
				header += "Content-Length: " + std::to_string(_bodyLen) + "\r\n";

			for(const auto& item: _httpAttrs)
			{
				header += item.GetKey() + ": " + item.GetValue() + "\r\n";
			}

			header += "\r\n";

			this->content.resize(header.size() + _bodyLen);
			this->body.resize(_bodyLen);

			std::copy(header.data(), header.data() + header.size(), this->content.data());
			std::copy(_body, _body + _bodyLen, this->content.data() + header.size());
			std::copy(_body, _body + _bodyLen, this->body.data());
		}

		int GetStateCode() const
		{
			return this->stateCode;
		}

		const char* GetContent() const
		{
			return this->content.data();
		}

		size_t GetContentSize() const
		{
			return this->content.size();
		}

		const char* GetBody() const
		{
			return this->body.data();
		}

		size_t GetBodySize() const
		{
			return this->body.size();
		}
	};

	class ISocket
	{
	public:
		virtual ~ISocket() = default;

		virtual int Bind(const sockaddr* _addr, socklen_t _addrlen) = 0;
		virtual int Connect(const sockaddr* _addr, socklen_t _addrlen) = 0;
		virtual int Listen(int _backlog) = 0;
		virtual int Accept(sockaddr* _addr, socklen_t* _addrlen) = 0;
		virtual int Read(void* _buffer, size_t _len) = 0;
		virtual int Write(const void* _buffe, size_t _len) = 0;
		virtual int Get() const = 0;
	};

	class Socket: virtual public ISocket
	{
	private:
		int sockfd;

	public:
		Socket(int _domain, int _type, int _protocol):
			sockfd(-1)
		{
			this->sockfd = socket(_domain, _type, _protocol);
			if(this->sockfd == -1)
			{
				throw std::runtime_error("create sock failed");
			}
		}

		Socket(int _sockfd):
			sockfd(_sockfd)
		{

		}

		~Socket()
		{
			close(this->sockfd);
		}

		virtual int Bind(const sockaddr* _addr, socklen_t _addrlen) override
		{
			//bind 普遍遭遇的问题是试图绑定一个已经在使用的端口。
			//该陷阱是也许没有活动的套接字存在，但仍然禁止绑定端口（bind 返回 EADDRINUSE），
			//它由 TCP 套接字状态 TIME_WAIT 引起。该状态在套接字关闭后约保留 2 到 4 分钟。
			//在 TIME_WAIT 状态退出之后，套接字被删除，该地址才能被重新绑定而不出问题。
			//等待 TIME_WAIT 结束可能是令人恼火的一件事，特别是如果您正在开发一个套接字服务器，就需要停止服务器来做一些改动，然后重启。
			//幸运的是，有方法可以避开 TIME_WAIT 状态。可以给套接字应用 SO_REUSEADDR 套接字选项，以便端口可以马上重用。
			
			int opt = 1;
			if(setsockopt(this->sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) != 0)
			{
				throw std::runtime_error("setsockopt failed");
			}
			return bind(this->sockfd, _addr, _addrlen);
		}

		virtual int Connect(const sockaddr* _addr, socklen_t _addrlen) override
		{
			return connect(this->sockfd, _addr, _addrlen);
		}

		virtual int Listen(int _backlog) override
		{
			return listen(this->sockfd, _backlog);
		}

		virtual int Accept(sockaddr* _addr, socklen_t* _addrlen) override
		{
			return accept(this->sockfd, _addr, _addrlen);
		}

		virtual int Read(void* _buffer, size_t _len)
		{
			return read(this->sockfd, _buffer, _len);
		}

		virtual int Write(const void* _buffer, size_t _len) override
		{
			return write(this->sockfd, _buffer, _len);
		}

		virtual int Get() const override
		{
			return this->sockfd;
		}
	};

	class SSLSocket: virtual public ISocket
	{
	private:
		using SSL_Ptr = std::unique_ptr<SSL, decltype(&SSL_free)>;

		Socket sock;
		SSL_Ptr ssl;

	public:
		SSLSocket(int _sockfd, SSL_CTX* _ctx):
			sock(_sockfd),
			ssl(SSL_new(_ctx), SSL_free)
		{
			//绑定ssl
                	SSL_set_fd(this->ssl.get(), this->sock.Get());

			if(SSL_accept(this->ssl.get()) == -1)
			{
				//close(_sockfd);
				throw std::runtime_error("SSL accept failed!");
			}	
		}

		virtual int Bind(const sockaddr* _addr, socklen_t _addrlen) override
		{
			return this->sock.Bind(_addr, _addrlen);
		}

		virtual int Connect(const sockaddr* _addr, socklen_t _addrlen) override
		{
			return this->sock.Connect(_addr, _addrlen);
		}

		virtual int Listen(int _backlog) override
		{
			return this->sock.Listen(_backlog);
		}

		virtual int Accept(sockaddr* _addr, socklen_t* _addrlen) override
		{
			return this->sock.Accept(_addr, _addrlen);
		}

		virtual int Read(void* _buffer, size_t _len)  override
		{
			return SSL_read(this->ssl.get(), _buffer, _len);
		}

		virtual int Write(const void* _buffer, size_t _len) override
		{
			return SSL_write(this->ssl.get(), _buffer, _len);
		}

		virtual int Get() const override
		{
			return this->sock.Get();
		}

		SSL* GetSSL() const
		{
			return this->ssl.get();
		}


	};

	class Websocket
	{
	private:
		ISocket* sock;
		//将数据变为websocket协议格式
		//若超出uint64范围，则需要大数类及注意websocekt数据帧
		std::vector<char> PackMessage(const char* _data, size_t _len, bool _isbuffer)
		{
			std::vector<char> bytes;

			if(_isbuffer)
			{
				bytes.push_back(130);
			}
			else
			{
				bytes.push_back(129);
			}

			size_t size(0);
			
			if(_len > std::numeric_limits<uint16_t>::max())
			{
				//std::cout << "7bit + 64bit" << std::endl;

				bytes.push_back(127);
				uint64_t len;

				size = len;

				//将本机字节序转换到网络字节序
				len = htonl(size);

				bytes.insert(bytes.end(), reinterpret_cast<char*>(&len), reinterpret_cast<char*>(&len) + sizeof(len));
			}
			else if(_len >= 126)
			{
				//std::cout << "7bit + 16bit" << std::endl;

				bytes.push_back(126);
				uint16_t len;

				size = _len;

				//将本机字节序转换到网络字节序
				len = htons(size);
                                                                                                                                     
                                bytes.insert(bytes.end(), reinterpret_cast<char*>(&len), reinterpret_cast<char*>(&len) + sizeof(len));
			}
			else
			{
				//std::cout << "7bit" << std::endl;
				bytes.push_back(_len);
				size = _len;
			}

			bytes.insert(bytes.end(), _data, _data + size);

			_len -= size;
			assert(_len == 0);

			return bytes;
		}

	public:
		Websocket(ISocket* _sock):
			sock(_sock)
		{

		}

		long GetId() const
		{
			return reinterpret_cast<long>(this->sock);
		}

		void SendText(std::string_view _text)
		{
			std::vector<char> data(this->PackMessage(_text.data(), _text.size(), false));
			
			if(sock->Write(data.data(), data.size()) == -1) 
			{
				throw std::runtime_error("websocket send failed!");
			}
		}

		void SendByte(char* _data, size_t _len)
		{
			std::vector<char> data(this->PackMessage(_data, _len, true));

			if(sock->Write(data.data(), data.size()) == -1)
			{
				throw std::runtime_error("websocket send failed!");
			}                                                          		
		}
	};

	class Router
	{
	private:
		using UrlCallback = HttpResponse(const UrlParam&, const HttpHeader&);
		using WebsocketConnectCallback = void(Websocket* _id, const HttpHeader& header);
		using WebsocketOnMessageCallback = void(Websocket* _id, const char* _data, size_t _size);
		using WebsocketDisconnectCallback = void(Websocket* _id);

		class IUrlCallbackObj
		{
		public:
			virtual ~IUrlCallbackObj() = default;

			virtual HttpResponse Callback(const UrlParam& _params, const HttpHeader&) = 0;
		};

		template<typename _TYPE, typename _METHOD>
		class UrlCallbackObj: virtual public IUrlCallbackObj
		{
		private:
			_TYPE* ptr;
			_METHOD func;
		                                                                        
		public:
			UrlCallbackObj(_TYPE* _ptr, _METHOD _func):
				ptr(_ptr),
				func(_func)
			{
		                                                                        
			}
			
			virtual HttpResponse Callback(const UrlParam& _params, const HttpHeader& _header) override
			{
				return ((this->ptr)->*(this->func))(_params, _header);
			}
		};

		class IWebsocketeCallbackObj
		{
		public:
			virtual ~IWebsocketeCallbackObj() = default;

			virtual void ConnectCallback(Websocket* _websocket, const HttpHeader& _header) = 0;
			virtual void OnMessageCallback(Websocket* _websocket, const char* _data, size_t _len) = 0;
			virtual void DisconnectCallback(Websocket* _websocket) = 0;
		};

		template<typename _CONNTYPE, typename _CONNMETHOD, typename _MSGTYPE, typename _MSGMETHOD, typename _DISCONNTYPE, typename _DISCONNMETHOD>
		class WebsocketCallbackObj: virtual public IWebsocketeCallbackObj
		{
		private:
			_CONNTYPE* connPtr;
			_MSGTYPE* msgPtr;
			_DISCONNTYPE* disConnPtr;

			_CONNMETHOD connFunc;
			_MSGMETHOD msgFunc;
			_DISCONNMETHOD disConnFunc;

		public:
			WebsocketCallbackObj(_CONNTYPE* _connPtr, _CONNMETHOD _connFunc, _MSGTYPE* _msgPtr, _MSGMETHOD _msgFunc, _DISCONNTYPE* _disConnPtr, _DISCONNMETHOD _disConnFunc):
				connPtr(_connPtr),
				connFunc(_connFunc),
				msgPtr(_msgPtr),
				msgFunc(_msgFunc),
				disConnPtr(_disConnPtr),
				disConnFunc(_disConnFunc)
			{

			}	

			virtual void ConnectCallback(Websocket* _websocket, const HttpHeader& _header) override
			{
				((this->connPtr)->*this->connFunc)(_websocket, _header);
			}

			virtual void OnMessageCallback(Websocket* _websocket, const char* _data, size_t _len)
			{
				((this->msgPtr)->*this->msgFunc)(_websocket, _data, _len);
			}

			virtual void DisconnectCallback(Websocket* _websocket) override
			{
				((this->disConnPtr)->*this->disConnFunc)(_websocket);
			}

		};

		struct UrlInfo
		{
			std::string type;
			std::string url;
			UrlCallback* callback;
		};

		struct WebsocketInfo
		{
			std::string url;
			WebsocketConnectCallback* connectCallback;
			WebsocketOnMessageCallback* onMessageCallback;
			WebsocketDisconnectCallback* disconnectCallback;
		};

		//第一层用url映射，第二层用http GET SET等类型映射
		std::unordered_map<std::string, std::unordered_map<std::string, UrlInfo>> urlInfos;
		std::unordered_map<std::string, WebsocketInfo> websocketInfos;

		std::unordered_map<std::string, std::unordered_map<std::string, std::unique_ptr<IUrlCallbackObj>>> urlObjInfos;
		std::unordered_map<std::string, std::unique_ptr<IWebsocketeCallbackObj>> websocketObjInfos;

		const UrlInfo* GetUrlInfo(std::string_view _type, std::string_view _url) const
		{
			auto urlIter = this->urlInfos.find(_url.data());
			if(urlIter == this->urlInfos.end())
				return nullptr; 
			                                                           
			auto typeIter = urlIter->second.find(_type.data());
			if(typeIter == urlIter->second.end())
				return nullptr;

			return &typeIter->second;
		}

		IUrlCallbackObj* GetUrlCallbackObj(std::string_view _type, std::string_view _url) const
		{
			auto urlIter = this->urlObjInfos.find(_url.data());
			if(urlIter == this->urlObjInfos.end())
				return nullptr;

			auto typeIter = urlIter->second.find(_type.data());
			if(typeIter == urlIter->second.end())
				return nullptr;

			return typeIter->second.get();
		}

		const WebsocketInfo* GetWebsocketInfo(std::string_view _url) const
		{
			auto iter = this->websocketInfos.find(_url.data());
			if(iter == this->websocketInfos.end())
				return nullptr;

			return &iter->second;	
		}

		IWebsocketeCallbackObj* GetWebsocketObj(std::string_view _url) const
		{
			auto iter = this->websocketObjInfos.find(_url.data());
			if(iter == this->websocketObjInfos.end())
				return nullptr;

			return iter->second.get();
		}

	public:
		void RegisterUrl(std::string_view _type, std::string_view _url, UrlCallback* _func)
		{
			if(this->urlInfos.find(_url.data()) != this->urlInfos.end())
			{
				throw std::logic_error("url has been register");
			}

			UrlInfo temp = {};

			temp.type = _type;
			temp.url = std::string(_url.data());
			temp.callback = std::move(_func);
	
			this->urlInfos[_url.data()].insert(std::pair<std::string, UrlInfo>(_type, std::move(temp)));
		}

		template<typename _TYPE>
		void RegisterUrl(std::string_view _type, std::string_view _url, HttpResponse(_TYPE::*_func)(const UrlParam&, const HttpHeader&), _TYPE* _ptr)
		{
			//成员指针调用成员函数指针语法
			//(_ptr->*_func)(UrlParam());
			
			std::unique_ptr<IUrlCallbackObj> temp(std::make_unique<UrlCallbackObj<_TYPE, decltype(_func)>>(_ptr, _func));
			this->urlObjInfos[_url.data()].insert(std::pair<std::string, std::unique_ptr<IUrlCallbackObj>>(_type.data(), std::move(temp)));
		}

		void RegisterWebsocket(std::string_view _url, WebsocketConnectCallback* _connect, WebsocketOnMessageCallback* _onMessage, WebsocketDisconnectCallback* _disconnect)
		{
			if(this->websocketInfos.find(_url.data()) != this->websocketInfos.end())
			{
				throw std::logic_error("url has been register");
			}

			WebsocketInfo temp = {};

			temp.url = _url;
			temp.connectCallback = _connect;
			temp.onMessageCallback = _onMessage;
			temp.disconnectCallback = _disconnect;
			this->websocketInfos.insert(std::pair<std::string, WebsocketInfo>(_url, std::move(temp)));	
		}

		template<typename _CONNTYPE, typename _MSGTYPE, typename _DISCONNTYPE>
		void RegisterWebsocket(std::string_view _url, void(_CONNTYPE::*_connect)(Websocket*, const HttpHeader&), void(_MSGTYPE::*_onMessage)(Websocket*, const char*, size_t), void(_DISCONNTYPE::*_disconnect)(Websocket*), _CONNTYPE* const _connPtr, _MSGTYPE* const _msgPtr, _DISCONNTYPE* const _disConnPtr)
		{
			if(this->GetWebsocketInfo(_url) != nullptr
			|| this->GetWebsocketObj(_url) != nullptr)
				throw std::logic_error("websocket url had been register");

			std::unique_ptr<IWebsocketeCallbackObj> temp(new WebsocketCallbackObj<_CONNTYPE, decltype(_connect), _MSGTYPE, decltype(_onMessage), _DISCONNTYPE, decltype(_disconnect)>(_connPtr, _connect, _msgPtr, _onMessage, _disConnPtr, _disconnect));
			this->websocketObjInfos.insert(std::pair<std::string, std::unique_ptr<IWebsocketeCallbackObj>>(_url.data(), std::move(temp)));
		}

		template<typename _TYPE>
                void RegisterWebsocket(std::string_view _url, void(_TYPE::*_connect)(Websocket*, const HttpHeader&), void(_TYPE::*_onMessage)(Websocket*, const char*, size_t), void(_TYPE::*_disconnect)(Websocket*), _TYPE* const _ptr)
                {
			this->RegisterWebsocket(_url, _connect, _onMessage, _disconnect, _ptr, _ptr, _ptr);
                }

		bool FindUrlCallback(std::string_view _type, std::string_view _url) const
		{
			if(this->GetUrlInfo(_type, _url) != nullptr)
				return true;

			if(this->GetUrlCallbackObj(_type, _url) != nullptr)
				return true;	

			return false;
		}

		HttpResponse RunCallback(std::string_view _type, std::string_view _url, const UrlParam& _params, const HttpHeader& _header)
		{
			auto urlInfo = this->GetUrlInfo(_type, _url);
			if(urlInfo != nullptr)
			{
				return (urlInfo->callback)(_params, _header);
			}

			auto urlCallbackObj = this->GetUrlCallbackObj(_type.data(), _url.data());
			if(urlCallbackObj != nullptr)
			{
				return urlCallbackObj->Callback(_params, _header);
			}

			throw std::runtime_error("callback not found");
		}
		
		bool FindWebsocketCallback(std::string_view _url) const
		{
			if(this->GetWebsocketInfo(_url) != nullptr 
			|| this->GetWebsocketObj(_url) != nullptr)
				return true;
			else
				return false;
		}

		void RunWebsocketConnectCallback(std::string_view _url, Websocket* const _websocket, const HttpHeader& _header) 
		{
			auto info = this->GetWebsocketInfo(_url);
                	if(info != nullptr)
                	{
                		return info->connectCallback(_websocket, _header);
                	}
                	                                                                   
                	auto objInfo = this->GetWebsocketObj(_url);
                	if(objInfo != nullptr)
                	{
                		return objInfo->ConnectCallback(_websocket, _header);
                	}

			throw std::logic_error("websocket url not exists");
		}

		void RunWebsocketOnMessageCallback(std::string_view _url, Websocket* const _websocket, const char* _data, size_t _len)
		{
			auto info = this->GetWebsocketInfo(_url);
			if(info != nullptr)
			{
				return info->onMessageCallback(_websocket, _data, _len);
			}

			auto objInfo = this->GetWebsocketObj(_url);
			if(objInfo != nullptr)
			{
				return objInfo->OnMessageCallback(_websocket, _data, _len);
			}

			throw std::logic_error("websocket url not exists");
		}

		void RunWebsocketDisconnectCallback(std::string_view _url, Websocket* const _websocket)
		{
			auto info = this->GetWebsocketInfo(_url);
			if(info != nullptr)
			{
				return info->disconnectCallback(_websocket);
			}
			                                                                   
			auto objInfo = this->GetWebsocketObj(_url);
			if(objInfo != nullptr)
			{
				return objInfo->DisconnectCallback(_websocket);
			}

			throw std::logic_error("websocket url not exists");
		}
	};

	namespace
	{
		HttpRequest GetHttpRequest(ISocket* _sock)
		{
			int bodyAccLen(0);
			int bodyReqLen(0);
			std::vector<char> content;

			std::string_view view;
			std::string::size_type left;
			std::string::size_type right;

			//首次读取数据分析header
			char buffer[1024];
			do
			{
				const int recvLen = _sock->Read(buffer, sizeof(buffer));
				if(recvLen <= 0)
				{
					//recv超时
					if(recvLen == -1)
					{
						//SSL_read发生io错误时似乎不应该调用shutdown?
						//SSL_shutdown(_ssl);
					}
				                                                
					std::string temp("recv but return ");
				                                                
					temp += std::to_string(recvLen);
				                                                
					throw std::runtime_error(temp);
					//ERR_print_errors_fp(stderr);
				}
				
				content.insert(content.end(), buffer, buffer + recvLen);
				view = std::string_view(content.data(), content.size());

				left = view.find("\r\n\r\n");
			}while(left == std::string::npos);

			bodyAccLen = content.size() - (left + 4);
			
			left = view.find("Content-Length: ");
			if(left != std::string::npos)
			{
				left += 16; 
				right = view.find("\r\n", left);
				if(right != std::string::npos)
				{
					bodyReqLen = std::stoi(view.substr(left, right).data());
				}
			}
			

			//接收body
			while(bodyAccLen < bodyReqLen)
			{
				const int recvLen = _sock->Read(buffer, sizeof(buffer));

				if(recvLen <= 0)
				{
					//recv超时
					if(recvLen == -1)
					{
						//SSL_shutdown(_ssl);
					}

					std::string temp("recv but return ");

					temp += std::to_string(recvLen);

					throw std::runtime_error(temp);
					//ERR_print_errors_fp(stderr);
				}

				content.insert(content.end(), buffer, buffer + recvLen);
				bodyAccLen += recvLen;
			}

			return HttpRequest(content.data());
		}

		void SendHttpResponse(ISocket* _sock, const HttpResponse& _response)
		{

			const size_t contentSize = _response.GetContentSize();
			const char* content = _response.GetContent();

			_sock->Write(content, contentSize);
		}

		HttpResponse GetHttpResponse(ISocket* _sock)
		{
			std::vector<char> content;

			std::array<char, 1024> buffer;
			std::string::size_type left;
			std::string::size_type right;
			std::string_view view;
			
			//分析header
			do
			{
				const int recvLen = _sock->Read(buffer.data(), buffer.size());
				if(recvLen <= 0)
				{
					throw std::runtime_error(std::string("recv but return ") + std::to_string(recvLen));
				}

				content.insert(content.end(), buffer.begin(), buffer.begin() + recvLen);
				view = std::string_view(content.data(), content.size());
				left = view.find("\r\n\r\n");
			} while(left == std::string::npos);

			view = std::string_view(content.data(), left);

			int bodyReqLen = 0;
			int bodyAccLen = content.size() - (left + 4);
			std::vector<HttpAttr> attrs;

			left = view.find(" ") + 1;
			right = view.find(" ", left);
			const int stateCode = std::stoi(std::string(view.substr(left, right - left)));

			left = view.find("\r\n");
			right = 0;
			while(left != std::string::npos && right != std::string::npos)
			{
				left += 2;
				right = view.find(":", left);
				if(right == std::string::npos)
					throw std::runtime_error("invaild header");

				const std::string key = std::string(view.substr(left, right - left));

				left = view.find_first_not_of(" ", right) + 2;
				right = view.find("\r\n", left);

				const std::string value = std::string(view.substr(left, right - left));

				//因为httpresponse类会自动增加content-length字段
				//所以跳过attrs
				if(key == "Content-Length")
				{
					bodyReqLen = std::stoll(value);
				}
				else
				{
					attrs.push_back(HttpAttr(std::move(key), std::move(value)));
				}

				left = view.find("\r\n", right);
			}

			//获取body
			content.erase(content.begin(), content.end() - bodyAccLen);
			while(bodyAccLen < bodyReqLen)
			{
				const int recvLen = _sock->Read(buffer.data(), buffer.size());
				if(recvLen <= 0)
				{
					throw std::runtime_error(std::string("recv but return ") + std::to_string(recvLen));
				}

				bodyAccLen += recvLen;

				content.insert(content.end(), buffer.begin(), buffer.begin() + recvLen);
			}

			HttpResponse response(stateCode, std::move(attrs), content.data(), content.size());

			return response;
		}
	
		void SendHttpRequest(ISocket* _sock, const HttpRequest& _request)
		{
			std::string temp;

			temp += _request.GetType() + " ";
			temp += _request.GetUrl() + " ";
			temp += "HTTP/1.1\r\n";
			
			for(const auto& item: _request.GetHeader().GetHttpAttrs())
			{
				temp += item.GetKey() + ": " + item.GetValue() + "\r\n";
			}

			temp += "\r\n";

			if(_sock->Write(temp.data(), temp.size()) == -1)
			{
				perror("write");
				throw std::runtime_error("send http request failed!");	
			}
		}		
	}

	class HttpServer
	{
	private:
		using SSL_Ptr = std::unique_ptr<SSL, decltype(&SSL_free)>;
		using SSL_CTX_Ptr = std::unique_ptr<SSL_CTX, decltype(&SSL_CTX_free)>;

		std::unique_ptr<Router> router;

		bool useSSL;
		bool listenSignal;
		Socket serverSock;

		//返回websocket的Sec-Websocket-Accept码
		//https://www.zhihu.com/question/67784701
		//在WebSocket通信协议中服务端为了证实已经接收了握手，它需要把两部分的数据合并成一个响应。
		//一部分信息来自客户端握手的Sec-WebSocket-Keyt头字段：Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==。
		//对于这个字段，服务端必须得到这个值并与"258EAFA5-E914-47DA-95CA-C5AB0DC85B11"组合成一个字符串，
		//经过SHA-1掩码，base64编码后在服务端的握手中返回。
		static std::string GetWebsocketCode(std::string_view _code)
		{
			const char* magicString = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

			//获取sha1编码
			SHA_CTX shaCtx;
			SHA1_Init(&shaCtx);

			const std::string code = std::string(_code) + magicString;
			SHA1_Update(&shaCtx, code.c_str(), code.size());
			
			unsigned char sha1Code[20];
			SHA1_Final(sha1Code, &shaCtx);

			//base64编码
			BIO *bmem = NULL;
			BIO *b64 = NULL;
			BUF_MEM *bptr;
			b64 = BIO_new(BIO_f_base64());
			BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
			bmem = BIO_new(BIO_s_mem());
			b64 = BIO_push(b64, bmem);
			BIO_write(b64, sha1Code, sizeof(sha1Code));
			BIO_flush(b64);
			BIO_get_mem_ptr(b64, &bptr);
			BIO_set_close(b64, BIO_NOCLOSE);
			
			std::string result;

			result.resize(bptr->length);
			memcpy(result.data(), bptr->data, bptr->length);
			BIO_free_all(b64);
			
			return result;
		}

		//解析websocket通讯的内容
		//参考:
		//https://blog.csdn.net/zhusongziye/article/details/80316127
		//https://blog.csdn.net/hnzwx888/article/details/84021754
		//有可能tcp粘包，所以返回多个
		static std::vector<WebsocketData> GetWebsocketMessage(ISocket* _sock)
		{
			//tcp粘包时第二个websocket包已读部分
			std::vector<char> lastBuffer;

			std::array<char, 1024> buffer;
			decltype(buffer)::iterator leftIter;

			std::vector<WebsocketData> result;

			do
			{
				//复制上次的缓冲区
				if(lastBuffer.size() > 0)
				{
					std::cout << "多次循环 size" << lastBuffer.size() << std::endl;
					std::copy(lastBuffer.begin(), lastBuffer.end(), buffer.begin());
				}

				//读取websocket头信息
				int recvLen = _sock->Read(buffer.data() + lastBuffer.size(), buffer.size() - lastBuffer.size());

				leftIter = buffer.begin();	

				lastBuffer.clear();

				if(recvLen <= 0)
				{
					std::string temp("wesocket recv but return ");

					temp += std::to_string(recvLen);
					throw std::runtime_error(temp);
				}

				//读取2byte Websocket信息
                		WebsocketData info = {};
                		//std::bitset<16> bits(*leftIter & 255 | (*(leftIter + 1) << 8));
                		//info.fin = bits[0];
                		//info.rsv1 = bits[1];
                		//info.rsv2 = bits[2];
                		//info.rsv3 = bits[3];
                		//info.opcode = *leftIter & 15;
                		//info.mask = bits[15];
                		//unsigned short length(*(leftIter + 1) & 127);
				
				std::bitset<8> bits(*leftIter);
				info.fin = bits[0];
				info.rsv1 = bits[1];
				info.rsv2 = bits[2];
				info.rsv3 = bits[3];
				info.opcode = *leftIter & 15;

				leftIter++;

				bits = htons(*leftIter);
				info.mask = bits[0];
				unsigned short length(*leftIter & 127);
		
				//移动读取下标
				leftIter++;

				if(length == 126)
				{
					length = ntohs(*reinterpret_cast<uint16_t*>(leftIter));
					leftIter += sizeof(uint16_t);
				}
				else if(length == 127)
				{
					length = ntohl(*reinterpret_cast<uint64_t*>(leftIter));
					leftIter += sizeof(uint64_t);
				}
				                                                               
				if(info.mask == true)
				{
					std::copy(leftIter, leftIter + 4, info.maskingKey.begin());
					leftIter += 4;
				}

				//读取websocket内容
				info.payload.resize(length);

				decltype(buffer)::iterator rightIter(buffer.begin() + recvLen);

				//判断能否一次读取完毕payload
				if((rightIter - leftIter) < length)
				{
					std::copy(leftIter, rightIter, info.payload.begin());

					int unRead(length - (rightIter - leftIter));
					while (unRead > 0) 
					{
						memset(buffer.data(), 0, buffer.size());
						recvLen = _sock->Read(buffer.data(), buffer.size());
					
						if(recvLen <= 0)
						{
							std::string temp("wesocket recv but return ");
						                                                       
							temp += std::to_string(recvLen);
							throw std::runtime_error(temp);
						}

						leftIter = buffer.begin();

						if(recvLen > unRead)
							rightIter = buffer.begin() + unRead;
						else
							rightIter = buffer.begin() + recvLen;
							
						const int hadRead = length - unRead;
						std::copy(leftIter, rightIter, info.payload.begin() + hadRead);

						unRead -= rightIter - leftIter;
					}
				}
				else
				{
					std::copy(leftIter, leftIter + length, info.payload.begin());
				}

				//发现还有未读数据则加入到缓冲区
				//以致下次循环继续接收数据	
				if(rightIter != buffer.begin() + recvLen)
				{
					lastBuffer.insert(lastBuffer.begin(), rightIter, buffer.begin() + recvLen);
				}

				if(info.mask == true)
				{
					for(int i = 0; i < info.payload.size(); i++)
					{
						const int j = i % 4;
						info.payload[i] = info.payload[i] ^ info.maskingKey[j];
					}
				}

				result.emplace_back(std::move(info));

			} while(lastBuffer.size() > 0);

			return result;
		}

		static std::vector<char> GetRootFile(std::string_view _view)
		{
			const std::string root = "wwwroot/";

			std::ifstream file(root + _view.data());
			std::vector<char> bytes;

			if(file.is_open())
			{
				file.seekg(0, std::ios::end);
				bytes.resize(file.tellg());
				file.seekg(0, std::ios::beg);
	
				file.read(bytes.data(), bytes.size());
			}
			else
			{
				throw std::runtime_error("could not find wwwroot file");
			}

			file.close();

			return bytes;
		}

		inline std::unique_ptr<ISocket> HandleAccept(int _sockfd, int _epfd, SSL_CTX* _ctx)
		{
			//设置超时时间
			struct timeval timeout={3,0};//3s
    			if(setsockopt(_sockfd, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout)) == -1)
				std::cout << "setsoockopt failed!" << std::endl;
    			if(setsockopt(_sockfd, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout)) == -1)
				std::cout << "setsoockopt failed!" << std::endl;
		
			std::unique_ptr<ISocket> sock(nullptr);
			if(this->useSSL)
			{
				sock = std::unique_ptr<ISocket>(new SSLSocket(_sockfd, _ctx));
			}
			else
			{
				sock = std::unique_ptr<ISocket>(new Socket(_sockfd));	
			}

			epoll_event ev;

			ev.data.fd = sock->Get();
			ev.events = EPOLLIN;
			epoll_ctl(_epfd, EPOLL_CTL_ADD, sock->Get(), &ev);
		
			return sock;
		}

		static inline void CloseSocket(int _epfd, epoll_event* _ev)
		{
			//close(_ev->data.fd);
			epoll_ctl(_epfd, EPOLL_CTL_DEL, _ev->data.fd, _ev);
		}

		void ListenProc(HttpServer* _httpServer, sockaddr_in _sockAddr)
		{
			if(this->serverSock.Get() == -1)
			{
				std::cout << "create socket failed!" << std::endl;
				return;
			}
		
			if(this->serverSock.Bind(reinterpret_cast<sockaddr*>(&_sockAddr), sizeof(_sockAddr)) == -1)
			{	
				std::cout << "bind socket failed!" << std::endl;
				perror("bind");
				return;
			}
		
			const int max = 20;
		
			if(this->serverSock.Listen(max))
			{
				std::cout << "listen failed!" << std::endl;
				return;
			}
			
			epoll_event events[max];
			const int epfd = epoll_create(max);
		
			if(epfd == -1)
			{
				std::cout << "create epoll failed!" << std::endl;
				return;
			}

			epoll_event ev;	
			ev.data.fd = this->serverSock.Get();
			ev.events = EPOLLIN;
			if(epoll_ctl(epfd, EPOLL_CTL_ADD, this->serverSock.Get(), &ev) == -1)
			{
				std::cout << "epoll control failed!" << std::endl;
				return;
			}
			
			//openssl 资料
			//https://blog.csdn.net/ck784101777/article/details/103833822
			//https://www.csdn.net/gather_29/NtDagg3sNTAtYmxvZwO0O0OO0O0O.html
			//生成证书示例
			//[root@proxy conf]# openssl genrsa > cert.key                            #生成私钥,文件名必须与配置文件内相同
			//[root@proxy conf]# openssl req -new -x509 -key cert.key > cert.pem     #生成证书,需要输入信息
			//Country Name (2 letter code) [XX]: china                      #国家
			//State or Province Name (full name) []:hunan                   #省份
			//Locality Name (eg, city) [Default City]:changsha              #城市
			//Organization Name (eg, company) [Default Company Ltd]:xxx     #公司名
			//Organizational Unit Name (eg, section) []:xxx                 #单位名
			//Common Name (eg, your name or your server's hostname) []:主机名            #主机名hostname查看
			//Email Address []:xx@xx.com                                    #邮箱
			
			///支持ssl绑定证书
			SSL_CTX_Ptr ctx(SSL_CTX_new(SSLv23_server_method()), SSL_CTX_free);
			if(ctx == nullptr)
			{
				std::cout << "ctx is null" << std::endl;
				return;
			}
		
			if(this->useSSL)
			{	
				SSL_CTX_set_options(ctx.get(), SSL_OP_SINGLE_DH_USE | SSL_OP_SINGLE_ECDH_USE | SSL_OP_NO_SSLv2);
				SSL_CTX_set_verify(ctx.get(), SSL_VERIFY_NONE, NULL);
				EC_KEY* ecdh = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
				if(ecdh == nullptr)
				{
					std::cout << "EC_KEY_new_by_curve_name failed!" << std::endl;
					return;
				}

				if(SSL_CTX_set_tmp_ecdh(ctx.get(), ecdh) != 1)
				{
					std::cout << "SSL_CTX_set_tmp_ecdh failed!" << std::endl;
					return;
				}
				//

				const char* publicKey = "cert.pem";
				const char* privateKey = "cert.key";

				//配置证书公匙
				if(SSL_CTX_use_certificate_file(ctx.get(), publicKey, SSL_FILETYPE_PEM) != 1)
				{
					std::cout << "SSL_CTX_use_cretificate_file failed!" << std::endl;
					return;
				}
				//配置证书私钥
				if(SSL_CTX_use_PrivateKey_file(ctx.get(), privateKey, SSL_FILETYPE_PEM) != 1)
				{
					std::cout << "SSL_CTX_use_PrivateKey_file failed!" << std::endl;
					return;
				}

				//验证证书
				if(SSL_CTX_check_private_key(ctx.get()) != 1)
				{
					std::cout << "SSL_CTX_check_private_key failed" << std::endl;
					return;
				}
			}
			
			std::unordered_map<int, std::unique_ptr<ISocket>> socketMap;
			std::unordered_map<int, std::unique_ptr<Websocket>> websocketMap;
			std::unordered_map<int, std::string> websocketUrlMap;

			while(_httpServer->listenSignal == true)
			{
				int nfds = epoll_wait(epfd, events, max, -1);
		
				if(nfds == -1)
				{
					std::cout << "epoll wait failed!" << std::endl;
				}
		
				for(int i = 0; i < nfds; i++)
				{
					//该描述符为服务器则accept
					if(events[i].data.fd == this->serverSock.Get())
					{
						sockaddr_in clntAddr = {};
				             	socklen_t size = sizeof(clntAddr);
		
						const int connfd = this->serverSock.Accept(reinterpret_cast<sockaddr*>(&clntAddr), &size);
						if(connfd == -1)
						{
							std::cout << "accept failed!" << std::endl;
							continue;
						}

						std::cout << "accept client_addr" << inet_ntoa(clntAddr.sin_addr) << std::endl;

						try
						{
							std::unique_ptr<ISocket> sock(HttpServer::HandleAccept(connfd, epfd, ctx.get()));
							socketMap.insert(std::pair<int, std::unique_ptr<ISocket>>(connfd, std::move(sock)));
						}
						catch(std::runtime_error _ex)
						{
							std::cout << _ex.what() << std::endl;
							ERR_print_errors_fp(stderr);
						}
					}
					//是客户端则返回信息
					else if(events[i].events & EPOLLIN)
					{
						if(websocketUrlMap.find(events[i].data.fd) != websocketUrlMap.end())
						{
							try
							{
								const std::vector<WebsocketData> infos = HttpServer::GetWebsocketMessage(socketMap.at(events[i].data.fd).get());
								for(const auto& info: infos)
								{
									//opcode 为8则表示断开连接
									if(info.opcode == 8)
									{
										_httpServer->router->RunWebsocketDisconnectCallback(websocketUrlMap.at(events[i].data.fd), websocketMap.at(events[i].data.fd).get());
										HttpServer::CloseSocket(epfd, &events[i]);
										socketMap.erase(events[i].data.fd);
										websocketUrlMap.erase(events[i].data.fd);
										websocketMap.erase(events[i].data.fd);
									}
									else
									{
										_httpServer->router->RunWebsocketOnMessageCallback(websocketUrlMap.at(events[i].data.fd), websocketMap.at(events[i].data.fd).get(), info.payload.data(), info.payload.size());
									}
								}
							}
							catch (std::runtime_error _ex)
							{
								_httpServer->router->RunWebsocketDisconnectCallback(websocketUrlMap.at(events[i].data.fd), websocketMap.at(events[i].data.fd).get());
								std::cout << "websocke error:" << _ex.what() << std::endl;
								std::cout << "url:" << websocketUrlMap.at(events[i].data.fd) << std::endl;
								HttpServer::CloseSocket(epfd, &events[i]);
								socketMap.erase(events[i].data.fd);
								websocketUrlMap.erase(events[i].data.fd);
								websocketMap.erase(events[i].data.fd);
							}
						}
						else
						{
							try
							{
								HttpRequest request = web::GetHttpRequest(socketMap.at(events[i].data.fd).get());

								//检测是否是websocket
								if(std::string(request.GetHeader().GetUpgrade()) == "websocket"
									&& _httpServer->router->FindWebsocketCallback(request.GetUrl()))
								{	
									const std::string secWebsocketKey = request.GetHeader().GetSecWebSocketKey();
									const std::string secWebsocketAccept = HttpServer::GetWebsocketCode(secWebsocketKey);

									const std::vector<HttpAttr> httpAttrs =
									{
										HttpAttr("Upgrade", "websocket"),
										HttpAttr("Connection", "Upgrade"),
										HttpAttr("Sec-WebSocket-Accept", secWebsocketAccept),
									};


									HttpResponse response(101, httpAttrs, nullptr, 0);

									web::SendHttpResponse(socketMap.at(events[i].data.fd).get(), std::move(response));

									std::cout << "回复websocket完毕" << std::endl;

									
									std::unique_ptr<Websocket> temp(new Websocket(socketMap.at(events[i].data.fd).get()));
									
									try 
									{
										_httpServer->router->RunWebsocketConnectCallback(request.GetUrl(), temp.get(), request.GetHeader());

										websocketMap.insert(std::pair<int, std::unique_ptr<Websocket>>(static_cast<int>(events[i].data.fd), std::move(temp)));
										websocketUrlMap.insert(std::pair<int, std::string>(static_cast<int>(events[i].data.fd), request.GetUrl()));
									}
									catch(std::runtime_error _ex)
									{
										std::cout << "websocket error:" << _ex.what() << std::endl;
										std::cout << "url:" << request.GetUrl() << std::endl;
									}
									catch(std::logic_error _ex)
									{
										std::cout << "websocket error:" << _ex.what() << std::endl;
										std::cout << "url:" << request.GetUrl() << std::endl;
									}
								}
								else
								{
									//错误回调
									std::function<void(std::string_view, std::string_view, ISocket*)> logError = 
									[](std::string_view _url, std::string_view _error, ISocket* _socket)
									{
										std::cout << "url:" << _url << std::endl;
										std::cout << "error:" << _error << std::endl;

										HttpResponse response(404, {}, nullptr, 0);
										                             
										web::SendHttpResponse(_socket, std::move(response));
									};

									if(_httpServer->router->FindUrlCallback(request.GetType(), request.GetUrl()))
									{
										UrlParam params;
										if(request.GetType() == "POST")
											params = std::move(JsonObj::ParseFormData(std::string(request.GetBody(), request.GetBodyLen())));
										else if(request.GetType() == "GET")
											params = std::move(JsonObj::ParseFormData(request.GetQueryString()));

										try
										{
											HttpResponse response = _httpServer->router->RunCallback(request.GetType(), request.GetUrl(), params, request.GetHeader());
											web::SendHttpResponse(socketMap.at(events[i].data.fd).get(), std::move(response));
										}
										catch(std::out_of_range _ex)
										{
											logError(request.GetUrl(), _ex.what(), socketMap.at(events[i].data.fd).get());	
										}
										catch(std::runtime_error _ex)
										{
											logError(request.GetUrl(), _ex.what(), socketMap.at(events[i].data.fd).get());	
										}

									}
									else if(request.GetType() == "GET")
									{
										try
										{
											const std::vector<char> body = HttpServer::GetRootFile(request.GetUrl());
											std::vector<HttpAttr> attrs = 
											{
												{"Cache-Control", "no-store"}
											};

											HttpResponse response(200, std::move(attrs), body.data(), body.size());
											                             
											web::SendHttpResponse(socketMap.at(events[i].data.fd).get(), std::move(response));
										}
										catch(std::runtime_error _ex)
										{
											logError(request.GetUrl(), _ex.what(), socketMap.at(events[i].data.fd).get());	
										}
									}
									else
									{
										HttpResponse response(404, {}, nullptr, 0);
                                                        			                             
                                                        			web::SendHttpResponse(socketMap.at(events[i].data.fd).get(), std::move(response));
									}
									
									if(request.GetHeader().GetConnection() != "keep-alive")
									{
										HttpServer::CloseSocket(epfd, &events[i]);
										socketMap.erase(events[i].data.fd);
									}
								}

							}
							catch(std::runtime_error _ex)
							{
								std::cout << _ex.what() << std::endl;
								HttpServer::CloseSocket(epfd, &events[i]);
								socketMap.erase(events[i].data.fd);
							}
						}
					}
					else
					{
						std::cout << "something else happen" << std::endl;
					}
				}

			}

			std::cout << "server close" << std::endl;
		}

		void InitSSL()
		{
			SSL_library_init();//SSL初始化
			SSL_load_error_strings();//为ssl加载更友好的错误提示
			OpenSSL_add_all_algorithms();
		}

	public:
		HttpServer(std::unique_ptr<Router>&& _router):
			router(std::move(_router)),
			useSSL(false),
			serverSock(AF_INET, SOCK_STREAM, IPPROTO_TCP)
		{
			this->InitSSL();

			struct sigaction sh;
   			struct sigaction osh;

			//**忽略socket发送数据远端关闭时,触发的SIGIPIPE信号
			signal(SIGPIPE, SIG_IGN);

			//**当SSL_shutdown、SSL_write 等对远端进行操作但对方已经断开连接时会触发异常信号、导致程序崩溃、
			//https://stackoverflow.com/questions/32040760/c-openssl-sigpipe-when-writing-in-closed-pipe?r=SearchResults
		}

		HttpServer& UseSSL(bool _isUse)
		{
			this->useSSL = _isUse;
			return *this;
		}

		HttpServer& Listen(std::string_view _address, int _port)
		{
			this->listenSignal = true;
			sockaddr_in serverAddr = {};

			serverAddr.sin_family = AF_INET;
			serverAddr.sin_addr.s_addr = inet_addr(_address.data());
			serverAddr.sin_port = htons(_port);

			std::cout << "server listening... ip:" << _address << " port:" << ntohs(serverAddr.sin_port) << std::endl;
			this->ListenProc(this, serverAddr);

			return *this;
		}

		HttpServer& Stop()
		{
			if(this->listenSignal == false)
			{
				throw std::logic_error("server is not listening.");
			}

			this->listenSignal = false;

			return *this;
		}
	
		std::string GetIpAddress()
		{
			sockaddr_in sa;
			socklen_t len = sizeof(sa);
			if(getsockname(this->serverSock.Get(), reinterpret_cast<sockaddr*>(&sa), &len) != 0)
			{
				throw std::runtime_error("get server address failed!");
			}

			return std::string(inet_ntoa(sa.sin_addr)) + ":" + std::to_string(ntohs(sa.sin_port));
		}	
	};

	class HttpClient
	{
	private:
		Socket sock;
		std::string ip;
		uint16_t port;

	public:
		HttpClient():
			sock(AF_INET, SOCK_STREAM, IPPROTO_TCP)
		{

		}

		void Connect(std::string_view _ip, uint16_t _port = 80)
		{
			this->ip = _ip;
			this->port = _port;

			sockaddr_in serverAddr = {};

			serverAddr.sin_family = AF_INET;
			serverAddr.sin_addr.s_addr = inet_addr(_ip.data());
			serverAddr.sin_port = htons(_port);

			const int result = this->sock.Connect(reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr));
			if(result != 0)
			{
				perror("connect");
				throw std::runtime_error("connect failed!");
			}
		}

		HttpResponse SendRequest(std::string _type, std::string _path, const char* _body = nullptr, size_t _bodySize = 0)
		{
			const std::string content = _type + " " + _path + " HTTP/1.1\r\n"
						"Accept-Encoding: identity\r\n"
						"Host: " + this->ip + ":" + std::to_string(this->port) + "\r\n"
						"\r\n";
			//HttpRequest request(content.data());
			
			const std::vector<HttpAttr> attrs = 
			{
				{"Accept-Encoding", "identity"},
				{"Host", this->ip + ":" + std::to_string(this->port)}
			};
		
			std::vector<char> body(_body, _body + _bodySize);	

			HttpRequest request(_type, _path, attrs, std::move(body));
		
			web::SendHttpRequest(&this->sock, request);

			const HttpResponse response = web::GetHttpResponse(&this->sock);

			return response;
		}
	};

	HttpResponse View(std::string_view _path)
	{
		const std::string root ="view/";
		const std::string path = root + _path.data();

		std::ifstream file(path);

		int stateCode(200);
		std::vector<char> body;

		if(file.is_open())
		{
			file.seekg(0, std::ios::end);
			body.resize(file.tellg());
			file.seekg(0, std::ios::beg);
			
			file.read(body.data(), body.size());
		}
		else
		{
			const char* temp = "Ops! file not found!";

			body.resize(strlen(temp));

			std::copy(temp, temp + body.size(), body.data());
		}

		file.close();

		return HttpResponse(stateCode, {}, body.data(), body.size());
	}

	HttpResponse Json(const JsonObj& _json)
	{
		std::vector<HttpAttr> attrs = 
		{
			{"Access-Control-Allow-Origin", "*"}
		};

		std::string res(_json.ToJson());

		return HttpResponse(200, std::move(attrs), res.data(), res.size());
	}

	HttpResponse Json(std::string_view _str)
	{
		std::vector<HttpAttr> attrs = 
		{
			{"Access-Control-Allow-Origin", "*"}
		};

		return HttpResponse(200, std::move(attrs), _str.data(), _str.size());
	}
};
