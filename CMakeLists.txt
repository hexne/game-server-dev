cmake_minimum_required(VERSION 4.3.0)
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS 1 CACHE INTERNAL "")

set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "f35a9ac6-8463-4d38-8eec-5d6008153e7d")
set(CMAKE_CXX_MODULE_STD 1)
add_compile_options(-freflection)

project(game-server-dev)

set(CMAKE_CXX_STANDARD 26)
add_subdirectory(src)

add_executable(game-server-dev src/main.cpp)
target_link_libraries(game-server-dev PRIVATE
        module
)

add_executable(manager src/manager.cpp)
target_link_libraries(manager PRIVATE
        module
)
