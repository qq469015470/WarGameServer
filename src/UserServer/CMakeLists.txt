# CMake 最低版本要求
cmake_minimum_required(VERSION 3.18.1)

# 项目信息
project(UserServer)

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
add_executable(UserServer main.cpp)

# 添加头文件路径
target_include_directories(UserServer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_include_directories(UserServer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include/bsoncxx/v_noabi)
target_include_directories(UserServer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include/mongocxx/v_noabi)

# 添加库文件路径
target_link_directories(UserServer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../lib/mongo-cxx-driver)
target_link_directories(UserServer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../lib/openssl)

# 引用的库文件名
find_library(BSONC NAMES libbson-static-1.0.a)
find_library(MONGOC NAMES libmongoc-static-1.0.a)
find_library(BSONCXX NAMES libbsoncxx-static.a)
find_library(MONGOCXX NAMES libmongocxx-static.a)
find_library(SSL NAMES libssl.a)
find_library(CRYPTO NAMES libcrypto.a)

target_link_libraries(UserServer PRIVATE ${MONGOCXX})
target_link_libraries(UserServer PRIVATE ${BSONCXX})
target_link_libraries(UserServer PRIVATE ${MONGOC})
target_link_libraries(UserServer PRIVATE ${BSONC})

target_link_libraries(UserServer PRIVATE ${SSL})
target_link_libraries(UserServer PRIVATE ${CRYPTO})

target_link_libraries(UserServer PRIVATE dl)
target_link_libraries(UserServer PRIVATE pthread)

# mongoc driver库的依赖
target_link_libraries(UserServer PRIVATE resolv)
target_link_libraries(UserServer PRIVATE z)
target_link_libraries(UserServer PRIVATE rt)

# 复制ssl公钥密钥(证书)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/cert.key DESTINATION ${EXECUTABLE_OUTPUT_PATH})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/cert.pem DESTINATION ${EXECUTABLE_OUTPUT_PATH})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/wwwroot DESTINATION ${EXECUTABLE_OUTPUT_PATH})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/view DESTINATION ${EXECUTABLE_OUTPUT_PATH})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/service DESTINATION ${EXECUTABLE_OUTPUT_PATH})
