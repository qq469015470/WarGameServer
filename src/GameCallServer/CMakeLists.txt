# CMake 最低版本要求
cmake_minimum_required(VERSION 3.18.1)

# 项目信息
project(GameCallServer)

# 设置使用哪种方式编译
SET(CMAKE_BUILD_TYPE "Release") 

# 设置Debug编译选项
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")
# 设置Release 编译选项
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3")

# 设置C++语言版本
set(CMAKE_CXX_STANDARD 17)

# 设置硬性要求c++17
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#设置编译文件输出位置
set(EXECUTABLE_OUTPUT_PATH ./bin)

# 设置 RPATH 运行时库的路径
#set(CMAKE_SKIP_BUILD_RPATH FALSE)
#set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
#set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
## ORIGIN 即程序运行的目录
#set(CMAKE_INSTALL_RPATH "$ORIGIN/")

# 编译项目
add_executable(GameCallServer main.cpp ../GameCore/GameCore.cpp)

# 添加头文件搜索路径
target_include_directories(GameCallServer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_include_directories(GameCallServer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include/bsoncxx/v_noabi)
target_include_directories(GameCallServer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include/mongocxx/v_noabi)

# 添加库文件搜索路径
target_link_directories(GameCallServer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../lib/mongo-cxx-driver)
target_link_directories(GameCallServer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../lib/openssl)

# 查找库文件(可填入多个文件进行搜索 按顺序优先使用开头的文件(.a为静态库 .so为动态库)
#find_library(SSL NAMES libssl.a)
#find_library(CRYPTO NAMES libcrypto.a)


# 链接引用的库文件名(上面的库依赖下面库)
target_link_libraries(GameCallServer PRIVATE libssl.a)
target_link_libraries(GameCallServer PRIVATE libcrypto.a)

target_link_libraries(GameCallServer PRIVATE pthread)
target_link_libraries(GameCallServer PRIVATE dl)

# install 设置
SET(CMAKE_INSTALL_PREFIX /opt/GameCallServer)
install(TARGETS GameCallServer RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX})
install(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/service/gamecallserver DESTINATION /etc/init.d/)
