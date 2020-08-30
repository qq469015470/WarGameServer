# CMake 最低版本要求
cmake_minimum_required(VERSION 3.18.1)

# 项目信息
project(UserServer)

# 检查c++编译器标志，设置c++17支持变量
# include(CheckCXXCompilerFlag)
# CHECK_CXX_COMPILER_FLAG("-std=c++17" COMPILER_SUPPORTS_CXX11)
# 
# # 使用变量设置编译标志
# if(COMPILER_SUPPORTS_CXX11)
# 	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
# else()
# 	message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++17 support. Please use a different C++ compiler.")
# endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

#设置编译文件输出位置
set(EXECUTABLE_OUTPUT_PATH ./bin)

# 编译项目
add_executable(UserServer main.cpp)

# 添加头文件路径
target_include_directories(UserServer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_include_directories(UserServer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include/bsoncxx/v_noabi)
target_include_directories(UserServer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include/mongocxx/v_noabi)

# 添加库文件路径
target_link_directories(UserServer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../lib/mongo-cxx-driver/lib)

# 引用的库文件名
target_link_libraries(UserServer PRIVATE mongocxx)
target_link_libraries(UserServer PRIVATE bsoncxx)
target_link_libraries(UserServer PRIVATE ssl)
target_link_libraries(UserServer PRIVATE crypto)
target_link_libraries(UserServer PRIVATE dl)
target_link_libraries(UserServer PRIVATE pthread)

# 复制ssl公钥密钥(证书)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/cert.key DESTINATION ${EXECUTABLE_OUTPUT_PATH})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/cert.pem DESTINATION ${EXECUTABLE_OUTPUT_PATH})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/wwwroot DESTINATION ${EXECUTABLE_OUTPUT_PATH})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/view DESTINATION ${EXECUTABLE_OUTPUT_PATH})
