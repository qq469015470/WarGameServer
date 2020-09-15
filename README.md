# 项目描述
  该项目是一个贪吃蛇的游戏服务器，使用了CocosCreator开发前端页面，C++开发后端服务器，数据库使用了mongodb，使用Nginx负载均衡。
	主要流程是用户登录后进入一个房间列表，显示了服务器所有的游戏房间，全部玩家可见，可创建房间及加入房间的多人在线贪吃蛇游戏。
	
	主要架构为有五个服务器：
	WebServer、UserServer、RoomServer、GameCallServer、GameServer。
	
	WebServer负责展现前端游戏网页，使用了CocosCreator开发页面。

	UserServer 为用户服务器负责注册及登录与Token登录验证，及获取用户信息。

	RoomServer为房间服务器，负责管理服务器的全部房间，即所有GameServer的ip地址，创建房间时发送请求到GameCallServer，GameCallServer的地址由Nginx代理用于负载均衡请求到不同的GameCallServer服务器，获得GameServer的ip地址及端口后，使用tcp保持连接，当断开后即认为GameServer已关闭，从房间列表移除。

	GameCallServer 为游戏唤醒服务器，接收到请求后唤醒GameServer，然后返回GameServer的ip地址及端口号。

	GameServer为游戏服务器，负责运行游戏逻辑及与用户通讯，例如处理玩家加入房间，玩家的输入输出信息交互等，前端没有游戏逻辑只负责显示，游戏逻辑写在服务器。

# CMake版本需求

  该项目使用CMake管理，最低版本需要3.18
  
# 引用的库
  ## OpenSSL
  https://github.com/openssl/openssl 用于加密Https通讯
  
  ## mongo-cxx-driver
  http://mongocxx.org/mongocxx-v3/installation/ mongodb数据库驱动库，用于与mongodb数据库通讯
