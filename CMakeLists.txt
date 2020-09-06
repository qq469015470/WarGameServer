cmake_minimum_required(VERSION 3.18)

# 引用CMakeLists.txt
project(WarGameServer)

add_subdirectory(./src/UserServer)
add_subdirectory(./src/GameServer)
add_subdirectory(./src/GameCallServer)
add_subdirectory(./src/RoomServer)

