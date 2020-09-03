cmake_minimum_required(VERSION 3.18.1)

project(ChatServer)

SET(CMAKE_BUILD_TYPE "Release")

SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")
SET(CMAKE_CXXFLAGS_RELEASE "$ENV{CXXFLAGS} -O3")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(EXECUTABLE_OUTPUT_PATH ./bin)

add_executable(ChatServer main.cpp)

target_include_directories(ChatServer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_include_directories(ChatServer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include/bsoncxx/v_noabi)
target_include_directories(ChatServer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include/mongocxx/v_noabi)

target_link_directories(ChatServer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../lib/mongo-cxx-driver)
target_link_directories(ChatServer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../lib/openssl)

find_library(BSON NAMES libbson-static-1.0.a)
find_library(MONGOC NAMES libmongoc-static-1.0.a)
find_library(BSONCXX NAMES libbsoncxx-static.a)
find_library(MONGOCXX NAMES libmongocxx-static.a)
find_library(SSL NAMES libssl.a)
find_library(CRYPTO NAMES libcrypto.a)

target_link_libraries(ChatServer PRIVATE ${MONGOCXX})
target_link_libraries(ChatServer PRIVATE ${BSONCXX})
target_link_libraries(ChatServer PRIVATE ${MONGOC})
target_link_libraries(ChatServer PRIVATE ${BSONC})
target_link_libraries(ChatServer PRIVATE ${SSL})
target_link_libraries(ChatServer PRIVATE ${CRYPTO})

target_link_libraries(ChatServer PRIVATE dl)
target_link_libraries(ChatServer PRIVATE pthread)

target_link_libraries(ChatServer PRIVATE resolv)
target_link_libraries(ChatServer PRIVATE z)
target_link_libraries(ChatServer PRIVATE rt)


file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/cert.key DESTINATION ${EXECUTABLE_OUTPUT_PATH})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/cert.pem DESTINATION ${EXECUTABLE_OUTPUT_PATH})
