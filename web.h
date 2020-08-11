#pragma once

#include <cstring>
#include <string>
#include <iostream>
#include <map>
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

	class UrlParam
	{
	private:
		std::optional<std::string> val;
		size_t arrSize;
		std::unordered_map<std::string, std::unique_ptr<UrlParam>> params;

		inline UrlParam& Find(std::string_view _key)
		{
			std::unique_ptr<UrlParam>& result(this->params[_key.data()]);
			
			if(result == nullptr)
			{
				result = std::make_unique<UrlParam>();
			}

			return *result.get();
		}

	public:
		UrlParam():
			arrSize(0)
		{

		}

		const UrlParam& operator[](int _index) const
		{
			return *this->params.at(std::to_string(_index)).get();
		}

		UrlParam& operator[](int _index)
		{
			return this->Find(std::to_string(_index));
		}

		const UrlParam& operator[](std::string_view _key) const
		{
			return *this->params.at(_key.data()).get();
		}

		UrlParam& operator[](std::string_view _key)
		{
			return this->Find(_key);
		}

		UrlParam& operator=(std::string_view _value)
		{
			this->val = _value;
			return *this;
		}

		const std::string& ToString() const
		{
			if(!this->val.has_value())
			{
				throw std::runtime_error("not have val");
			}

			return *this->val;
		}

		//添加数组
		void PushBack(std::string_view _value)
		{
			std::string index(std::to_string(this->arrSize));

			UrlParam& temp(this->Find(std::to_string(this->arrSize)));

			temp[index] = _value;
			this->arrSize++;
		}

		size_t GetArraySize() const
		{
			return this->arrSize;
		}
	};	

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
		int contentLength;
		std::string connection;
		std::string upgrade;
		std::string secWebSocketKey;
		std::unordered_map<std::string, std::string> cookies;

		static inline std::string GetAttrValue(std::unordered_map<std::string, HttpAttr> _attrs, std::string_view _key)
		{
			auto iter = _attrs.find(_key.data());
			if(iter == _attrs.end())
			{
				return "";
			}
			else
			{
				return iter->second.GetValue();
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
				pos++;
				std::string value(_cookie.substr(pos, rightPos - pos));
				
				this->cookies.insert(std::pair<std::string, std::string>(std::move(key), std::move(value)));				
			                                                                
				leftPos = rightPos;
				rightPos = _cookie.find(";", leftPos);
				if(rightPos == std::string::npos)
					break;
			}
		}

	public:
		HttpHeader(std::string_view _headerStr):
			contentLength(0)
		{
			std::unordered_map<std::string, HttpAttr> attrs = this->ReadAttr(_headerStr);

			const std::string temp = this->GetAttrValue(attrs, "Content-Length");
			if(!temp.empty())
				this->contentLength = std::stoi(temp);

			this->connection = this->GetAttrValue(attrs, "Connection");
			this->upgrade = this->GetAttrValue(attrs, "Upgrade");
			this->secWebSocketKey = this->GetAttrValue(attrs, "Sec-WebSocket-Key");

			this->ReadCookie(this->GetAttrValue(attrs, "Cookie"));
		}

		//HttpHeader(const HttpHeader& _header):
		//	contentLength(_header.contentLength),
		//	connection(_header.connection),
		//	upgrade(_header.upgrade),
		//	secWebSocketKey(_header.secWebSocketKey),
		//	cookies(_header.cookies)
		//{

		//}

		int GetContentLength() const	
		{
			return this->contentLength;
		}

		const char* GetConnection() const
		{
			return this->connection.c_str();
		}
		
		const char* GetUpgrade() const
		{
			return this->upgrade.c_str();
		}

		const char* GetSecWebSocketKey() const
		{
			return this->secWebSocketKey.c_str();
		}

		const char* GetCookie(std::string_view _key) const
		{
			auto iter = this->cookies.find(_key.data());
			if(iter == this->cookies.end())
				return "";
			else
				return iter->second.c_str(); 
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
		char maskingKey[4];
		std::vector<char> payload;
	};

	class HttpRequest
	{
	private:
		//请求类型(例:get post)
		std::string type;
		std::string url;
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
			right = this->url.find("?", left);
			if(right != std::string::npos)
			{
				this->url = line.substr(left, right - left);
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

		const std::string& GetType() const
		{
			return this->type;
		}

		const std::string& GetUrl() const
		{
			return this->url;
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
		std::vector<char> content;
		size_t size;

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
		HttpResponse(int _stateCode,const std::vector<HttpAttr>& _httpAttrs, const char* _body, unsigned long _bodyLen):
			content({'\0'}),
			size(0)
		{
			std::string header;
			
			header = "HTTP/1.1 " + std::to_string(_stateCode) + " " + HttpResponse::GetSpec(_stateCode) + "\r\n";

			header += "Content-Length: " + std::to_string(_bodyLen) + "\r\n";

			for(const auto& item: _httpAttrs)
			{
				header += item.GetKey() + ": " + item.GetValue() + "\r\n";
			}

			header += "\r\n";

			this->content.resize(header.size() + _bodyLen);

			std::copy(header.data(), header.data() + header.size(), this->content.data());
			std::copy(_body, _body + _bodyLen, this->content.data() + header.size());
		}

		const char* GetContent() const
		{
			return this->content.data();
		}

		size_t GetSize() const
		{
			return this->content.size();
		}
	};

	class Websocket
	{
	private:
		SSL* ssl;

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
				std::cout << "7bit + 64bit" << std::endl;

				bytes.push_back(127);
				uint64_t len;

				size = len;

				//将本机字节序转换到网络字节序
				len = htons(size);

				bytes.insert(bytes.end(), reinterpret_cast<char*>(&len), reinterpret_cast<char*>(&len) + sizeof(len));
			}
			else if(_len >= 126)
			{
				std::cout << "7bit + 16bit" << std::endl;

				bytes.push_back(126);
				uint16_t len;

				size = _len;

				//将本机字节序转换到网络字节序
				len = htons(size);
                                                                                                                                     
                                bytes.insert(bytes.end(), reinterpret_cast<char*>(&len), reinterpret_cast<char*>(&len) + sizeof(len));
			}
			else
			{
				std::cout << "7bit" << std::endl;
				bytes.push_back(_len);
				size = _len;
			}

			bytes.insert(bytes.end(), _data, _data + size);

			_len -= size;
			assert(_len == 0);

			return bytes;
		}

	public:
		Websocket(SSL* _ssl):
			ssl(_ssl)
		{

		}

		long GetId() const
		{
			return reinterpret_cast<long>(this->ssl);
		}

		void SendText(std::string_view _text)
		{
			std::vector<char> data(this->PackMessage(_text.data(), _text.size(), false));
			
			SSL_write(this->ssl, data.data(), data.size());
		}

		void SendByte(char* _data, size_t _len)
		{
			std::vector<char> data(this->PackMessage(_data, _len, true));

			SSL_write(this->ssl, data.data(), data.size());
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
		void RegisterUrl(std::string_view _type, std::string_view _url, HttpResponse(_TYPE::*_func)(const UrlParam&), _TYPE* _ptr)
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

	class HttpServer
	{
	private:
		using SSL_Ptr = std::unique_ptr<SSL, decltype(&SSL_free)>;
		using SSL_CTX_Ptr = std::unique_ptr<SSL_CTX, decltype(&SSL_CTX_free)>;

		std::unique_ptr<Router> router;

		bool listenSignal;

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
		static WebsocketData GetWebsocketMessage(SSL* _ssl)
		{
			char buffer[1024];

			memset(buffer, 0, sizeof(buffer));
			const int recvLen = SSL_read(_ssl, buffer, sizeof(buffer));

			if(recvLen <= 0)
			{
				if(recvLen == -1)
				{
					SSL_shutdown(_ssl);
				}

				std::string temp("websocket recv but return ");

				temp += std::to_string(recvLen);
				throw std::runtime_error(temp.c_str());
			}

			WebsocketData info;
			std::bitset<16> bits(buffer[0] & 255 | (buffer[1] << 8));
			info.fin = bits[0];
			info.rsv1 = bits[1];
			info.rsv2 = bits[2];
			info.rsv3 = bits[3];
			info.opcode = buffer[0] & 15;
			info.mask = bits[15];
			unsigned short length(buffer[1] & 127);

			int readOffset(2);

			if(length == 126)
			{
				length = ntohs(*reinterpret_cast<uint16_t*>(&buffer[2]));
				readOffset += sizeof(uint16_t);
			}
			else if(length == 127)
			{
				length = *reinterpret_cast<uint64_t*>(&buffer[2]);
				readOffset += sizeof(uint64_t);
			}

			if(info.mask == true)
			{
				std::copy(buffer + readOffset, buffer + readOffset + 4, info.maskingKey);
				readOffset += 4;
			}

			//payload已读的长度
			const size_t payloadLen = recvLen - readOffset;

			info.payload.resize(length);
			memset(info.payload.data(), 0, info.payload.size());

			std::copy(buffer + readOffset, buffer + readOffset + payloadLen, info.payload.data());

			assert(payloadLen <= length);
			if(payloadLen < length)
			{
				SSL_read(_ssl, info.payload.data() + payloadLen, length - payloadLen);
			}

			if(info.mask == true)
			{
				for(int i = 0; i < length; i++)
				{
					const int j = i % 4;
					//info.payload[i] = buffer[readOffset + i] ^ info.maskingKey[j];
					info.payload[i] = info.payload[i] ^ info.maskingKey[j];
				}
			}
			else
			{
				std::copy(buffer + readOffset, buffer + readOffset + length, info.payload.data());
			}

			return info;
		}

		static HttpRequest GetHttpRequest(SSL* _ssl)
		{
			bool finish(false);
			int contentLen(0);
			std::vector<char> content;

			do
			{

				char buffer[1024];

				memset(buffer, 0, sizeof(buffer));

				const int recvLen =  SSL_read(_ssl, buffer, sizeof(buffer));

				//const int recvLen = recv(_sockfd, buffer, sizeof(buffer)  1, 0);
				

				if(recvLen <= 0)
				{
					//recv超时
					if(recvLen == -1)
					{
						SSL_shutdown(_ssl);
					}

					std::string temp("recv but return ");

					temp += std::to_string(recvLen);

					throw std::runtime_error(temp.c_str());
					//finish = true;
					//ERR_print_errors_fp(stderr);
				}

				contentLen += recvLen;

				//请求头部接收完毕
				content.insert(content.end(), buffer, buffer + recvLen);

				if(strstr(content.data(), "\r\n\r\n") != nullptr)
				{
					finish = true;
				}

			}
			while(!finish);

			return HttpRequest(content.data());
		}

		static void SendHttpResponse(SSL* _ssl, const HttpResponse& _response)
		{

			const size_t contentSize = _response.GetSize();
			const char* content = _response.GetContent();

			SSL_write(_ssl, content, contentSize);
			//send(_sockfd, content, contentSize, 0);
		}

		static UrlParam JsonToUrlParam(const char* _body, size_t _len)
		{
			std::string json(_body, _len);

			std::string::size_type left(0);
			std::string::size_type right(0);

			right = json.find("=");

			UrlParam params;
			while(right != std::string::npos)
			{
				std::string key(json.substr(left, right - left));

				//提取[]内的成员
				std::string::size_type keyLeft(0);
				std::string::size_type keyRight(key.find("%5B"));

				UrlParam* curParam(&params[key.substr(keyLeft, keyRight)]);

				while(keyRight != std::string::npos)
				{
					keyLeft = keyRight + 3;
					keyRight = key.find("%5D", keyLeft);

					const std::string attr(key.substr(keyLeft, keyRight - keyLeft));

					if(attr != "")
						curParam = &(*curParam)[attr];
					else
					{
						curParam->PushBack("");
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

		static inline SSL_Ptr HandleAccept(int _sockfd, int _epfd, SSL_CTX* _ctx)
		{
			//设置超时时间
			struct timeval timeout={3,0};//3s
    			if(setsockopt(_sockfd, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout)) == -1)
				std::cout << "setsoockopt failed!" << std::endl;
    			if(setsockopt(_sockfd, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout)) == -1)
				std::cout << "setsoockopt failed!" << std::endl;
			
			//绑定ssl
			SSL_Ptr ssl(SSL_new(_ctx), SSL_free);
			SSL_set_fd(ssl.get(), _sockfd);
			//ssl握手
			if(SSL_accept(ssl.get()) == -1)
			{
				close(_sockfd);
				throw std::runtime_error("SSL accept failed!");
			}	

			epoll_event ev;

			ev.data.fd = _sockfd;
			ev.events = EPOLLIN;
			epoll_ctl(_epfd, EPOLL_CTL_ADD, _sockfd, &ev);
		
			return ssl;
		}

		static inline void CloseSocket(int _epfd, SSL* _ssl, epoll_event* _ev)
		{
			close(_ev->data.fd);
			epoll_ctl(_epfd, EPOLL_CTL_DEL, _ev->data.fd, _ev);
		}

		static void ListenProc(HttpServer* _httpServer, sockaddr_in _sockAddr)
		{
			const int serverSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

			if(serverSock == -1)
			{
				std::cout << "create socket failed!" << std::endl;
				return;
			}
		
			if(bind(serverSock, reinterpret_cast<sockaddr*>(&_sockAddr), sizeof(_sockAddr)) == -1)
			{	
				std::cout << "bind socket failed!" << std::endl;
				perror("bind");
				return;
			}
		
			const int max = 20;
		
			if(listen(serverSock, max))
			{
				std::cout << "listen failed!" << std::endl;
				return;
			}	
			
			epoll_event events[max];
			const int epfd = epoll_create(max);
		
			if(epfd == -1)
			{
				std::cout << "create epoll faile!" << std::endl;
				return;
			}

			epoll_event ev;	
			ev.data.fd = serverSock;
			ev.events = EPOLLIN;
			if(epoll_ctl(epfd, EPOLL_CTL_ADD, serverSock, &ev) == -1)
			{
				std::cout << "epoll control failed!" << std::endl;
				return;
			}
			
			//openssl 资料
			//https://blog.csdn.net/ck784101777/article/details/103833822
			//https://www.csdn.net/gather_29/NtDagg3sNTAtYmxvZwO0O0OO0O0O.html
			
			///支持ssl绑定证书
			SSL_CTX_Ptr ctx(SSL_CTX_new(SSLv23_server_method()), SSL_CTX_free);
			//SSL_CTX* ctx = SSL_CTX_new(TLS_server_method());
			if(ctx == nullptr)
			{
				std::cout << "ctx is null" << std::endl;
				return;
			}
			
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
			
			std::unordered_map<int, SSL_Ptr> sslMap;
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
					if(events[i].data.fd == serverSock)
					{
						sockaddr_in clntAddr = {};
				             	socklen_t size = sizeof(clntAddr);
		
						const int connfd = accept(serverSock, reinterpret_cast<sockaddr*>(&clntAddr), &size);
						if(connfd == -1)
						{
							std::cout << "accept failed!" << std::endl;
							continue;
						}

						std::cout << "accept client_addr" << inet_ntoa(clntAddr.sin_addr) << std::endl;

						try
						{
							SSL_Ptr temp(HttpServer::HandleAccept(connfd, epfd, ctx.get()));
							sslMap.insert(std::pair<int, SSL_Ptr>(connfd, std::move(temp)));
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
								const WebsocketData info = HttpServer::GetWebsocketMessage(sslMap.at(events[i].data.fd).get());

								//opcode 为8则表示断开连接
								if(info.opcode == 8)
								{
									_httpServer->router->RunWebsocketDisconnectCallback(websocketUrlMap.at(events[i].data.fd), websocketMap.at(events[i].data.fd).get());
									HttpServer::CloseSocket(epfd, sslMap.at(events[i].data.fd).get(), &events[i]);
									sslMap.erase(events[i].data.fd);
									websocketUrlMap.erase(events[i].data.fd);
									websocketMap.erase(events[i].data.fd);
								}
								else
								{
									_httpServer->router->RunWebsocketOnMessageCallback(websocketUrlMap.at(events[i].data.fd), websocketMap.at(events[i].data.fd).get(), info.payload.data(), info.payload.size());
								}
							}
							catch (std::runtime_error _ex)
							{
								_httpServer->router->RunWebsocketDisconnectCallback(websocketUrlMap.at(events[i].data.fd), websocketMap.at(events[i].data.fd).get());
								std::cout << _ex.what() << std::endl;
								HttpServer::CloseSocket(epfd, sslMap.at(events[i].data.fd).get(), &events[i]);
								sslMap.erase(events[i].data.fd);
								websocketUrlMap.erase(events[i].data.fd);
								websocketMap.erase(events[i].data.fd);
							}
						}
						else
						{
							try
							{
								HttpRequest request = HttpServer::GetHttpRequest(sslMap.at(events[i].data.fd).get());

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

									HttpServer::SendHttpResponse(sslMap.at(events[i].data.fd).get(), std::move(response));
									std::cout << "回复websocket完毕" << std::endl;
									websocketUrlMap.insert(std::pair<int, std::string>(static_cast<int>(events[i].data.fd), request.GetUrl()));
									
									std::unique_ptr<Websocket> temp(new Websocket(sslMap.at(events[i].data.fd).get()));

									websocketMap.insert(std::pair<int, std::unique_ptr<Websocket>>(static_cast<int>(events[i].data.fd), std::move(temp)));
									_httpServer->router->RunWebsocketConnectCallback(request.GetUrl(), websocketMap.at(events[i].data.fd).get(), request.GetHeader());
								}
								else
								{
									if(_httpServer->router->FindUrlCallback(request.GetType(), request.GetUrl()))
									{
										const UrlParam params(HttpServer::JsonToUrlParam(request.GetBody(), request.GetBodyLen()));

										HttpResponse response = _httpServer->router->RunCallback(request.GetType(), request.GetUrl(), params, request.GetHeader());
										HttpServer::SendHttpResponse(sslMap.at(events[i].data.fd).get(), std::move(response));
									}
									else if(request.GetType() == "GET")
									{
										try
										{
											const std::vector<char> body = HttpServer::GetRootFile(request.GetUrl());

											HttpResponse response(200, {}, body.data(), body.size());
											                             
											HttpServer::SendHttpResponse(sslMap.at(events[i].data.fd).get(), std::move(response));
										}
										catch(std::runtime_error _ex)
										{
											HttpResponse response(404, {}, nullptr, 0);
											                             
											HttpServer::SendHttpResponse(sslMap.at(events[i].data.fd).get(), std::move(response));
										}
									}
									else
									{
										HttpResponse response(404, {}, nullptr, 0);
                                                        			                             
                                                        			HttpServer::SendHttpResponse(sslMap.at(events[i].data.fd).get(), std::move(response));
									}
									
									if(request.GetHeader().GetConnection() != "keep-alive")
									{
										HttpServer::CloseSocket(epfd, sslMap.at(events[i].data.fd).get(), &events[i]);
										sslMap.erase(events[i].data.fd);
									}
								}

							}
							catch(std::runtime_error _ex)
							{
								std::cout << _ex.what() << std::endl;
								HttpServer::CloseSocket(epfd, sslMap.at(events[i].data.fd).get(), &events[i]);
								sslMap.erase(events[i].data.fd);
							}
						}
					}
					else
					{
						std::cout << "something else happen" << std::endl;
					}
				}

			}

			close(serverSock);
			std::cout << "server close" << std::endl;
		}

		void InitSSL()
		{
			SSL_library_init();//SSL初始化
			SSL_load_error_strings();//为ssl加载更友好的错误提示
			OpenSSL_add_all_algorithms();
		}

		static void sigpipe_handler(int _val)
		{
			std::cout<< "sigpipe_handler:" << _val << std::endl;
		}

	public:
		HttpServer(std::unique_ptr<Router>&& _router):
			router(std::move(_router))
		{
			this->InitSSL();

			struct sigaction sh;
   			struct sigaction osh;

			//当SSL_shutdown、SSL_write 等对远端进行操作但对方已经断开连接时会触发异常信号、导致程序崩溃、
			//以下代码似乎可以忽略这个信号
			//https://stackoverflow.com/questions/32040760/c-openssl-sigpipe-when-writing-in-closed-pipe?r=SearchResults
   			sh.sa_handler = &HttpServer::sigpipe_handler; //Can set to SIG_IGN
   			// Restart interrupted system calls
   			sh.sa_flags = SA_RESTART;

   			// Block every signal during the handler
   			sigemptyset(&sh.sa_mask);

   			if (sigaction(SIGPIPE, &sh, &osh) < 0)
   			{
				std::cout << "sigcation failed!" << std::endl;
   			}
			//
		}

		void Listen(std::string_view _address, int _port)
		{
			this->listenSignal = true;
			sockaddr_in serverAddr = {};

			serverAddr.sin_family = AF_INET;
			serverAddr.sin_addr.s_addr = inet_addr(_address.data());
			serverAddr.sin_port = htons(_port);

			std::thread proc(HttpServer::ListenProc, this, serverAddr);

			std::cout << "server listening... ip:" << _address << " port:" << ntohs(serverAddr.sin_port) << std::endl;

			proc.detach();
		}

		void Stop()
		{
			if(this->listenSignal == false)
			{
				throw std::runtime_error("server is not listening.");
			}

			this->listenSignal = false;
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

	HttpResponse Json(std::string_view _str)
	{
		return HttpResponse(200, {}, _str.data(), _str.size());
	}
};
