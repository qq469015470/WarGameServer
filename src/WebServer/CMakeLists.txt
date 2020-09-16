# CMake 最低版本要求
cmake_minimum_required(VERSION 3.18.1)

# 项目信息
project(WebServer)

# 设置使用哪种方式编译
SET(CMAKE_BUILD_TYPE "Release") 

# 设置Debug编译选项
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")
# 设置Release 编译选项
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#设置编译文件输出位置
set(EXECUTABLE_OUTPUT_PATH ./bin)

# 编译项目
add_executable(WebServer main.cpp)

# 添加头文件路径
target_include_directories(WebServer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)

# 添加库文件路径
target_link_directories(WebServer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../lib/openssl)

# 引用的库文件名
target_link_libraries(WebServer PRIVATE libssl.a)
target_link_libraries(WebServer PRIVATE libcrypto.a)

target_link_libraries(WebServer PRIVATE dl)
target_link_libraries(WebServer PRIVATE pthread)

# 复制ssl公钥密钥(证书)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/wwwroot DESTINATION ${EXECUTABLE_OUTPUT_PATH})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/view DESTINATION ${EXECUTABLE_OUTPUT_PATH})

# install设置
SET(CMAKE_INSTALL_PREFIX /opt/WebServer) 
install(TARGETS WebServer RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX})
install(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/service/webserver DESTINATION /etc/init.d/)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/wwwroot DESTINATION ${CMAKE_INSTALL_PREFIX})
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/view DESTINATION ${CMAKE_INSTALL_PREFIX})

