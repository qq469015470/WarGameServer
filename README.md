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

  *注意：该项目使用CMake管理，最低版本需要3.18*
  
# 引用的库
  *注意：以下库不需要再另外下载，该项目已将第三方库包含在src/include及src/lib下*
  ## OpenSSL
  https://github.com/openssl/openssl 用于加密Https通讯
  
  ## mongo-cxx-driver
  http://mongocxx.org/mongocxx-v3/installation/ mongodb数据库驱动库，用于与mongodb数据库通讯

  ## glm
  https://github.com/g-truc/glm 数学库用于数学运算
  
# 编译安装

  ## 编译
  进入到目录下执行以下命令<br>
  mkdir build<br>
  cd build<br>
  cmake ..<br>
  make && make install<br>
  
  ## 配置nginx
  因为是分布式设计，RoomServer会固定请求7500端口，可以在GameCallServer的 upstream 添加多个GameCallServer ip地址
  ```
        ##
        #GameCallServer
        ##
        upstream GameCallServer {
			##可添加多个GameCallServer地址
                server localhost:7510;
        }

        server {
                listen 7500;
                server_name localhost;
                location / {
                        proxy_pass http://GameCallServer/;
                }
        }
        ##

        ##
        #GameServer
        ##
        server {
                listen 15000;
                server_name localhost;
                #rewrite ^/(.*) http://$arg_ip:$arg_port?;
                #rewrite ^/(.*) http://$arg_ip?;
                # 转发websocket需要的设置
                proxy_set_header X-Real_IP $remote_addr;
                proxy_set_header Host $host;
                proxy_set_header X_Forward_For $proxy_add_x_forwarded_for;
                proxy_http_version 1.1;
                proxy_set_header Upgrade $http_upgrade;
                proxy_set_header Connection 'upgrade';

                location / {
                        proxy_pass http://$arg_ip/chat;
                }
        }
        ##
  ```

  ## 运行
  make install 后会在Linux /opt文件夹生成 WebServer, GameServer, GameCallServer, GameServer, RoomServer,  这些文件皆为多个服务器的执行文件夹。<br>
  还会将服务器启动脚本生成于 /etc/init.d/ 下。<br>
  所以调用服务命令即可
  ```
  sudo service webserver start
  sudo service userserver start
  sudo service roomserver start
  sudo service gamecallserver start
  ```
