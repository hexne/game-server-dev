cmake_minimum_required(VERSION 4.3.0)
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS 1 CACHE INTERNAL "")

set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "f35a9ac6-8463-4d38-8eec-5d6008153e7d")
set(CMAKE_CXX_MODULE_STD 1)
set(EMBED_DIR "${CMAKE_CURRENT_LIST_DIR}" CACHE INTERNAL "embed resource directory" FORCE)
add_compile_options(
        -freflection
        --embed-dir=${EMBED_DIR}
)

project(game-server-dev)

set(CMAKE_CXX_STANDARD 26)

add_subdirectory(src)
add_subdirectory(tests)

add_executable(game-server-dev src/main.cpp)
target_link_libraries(game-server-dev PRIVATE
        module
        test_module
)
target_link_libraries(test_module PRIVATE module)

add_executable(manager tests/tests.cpp)
target_link_libraries(manager PRIVATE
        test_module
)

include(CTest)
add_test(NAME test_login     COMMAND manager ${CMAKE_SOURCE_DIR}/tests/test_files/test_login)
add_test(NAME test_room      COMMAND manager ${CMAKE_SOURCE_DIR}/tests/test_files/test_room)
add_test(NAME test_chat      COMMAND manager ${CMAKE_SOURCE_DIR}/tests/test_files/test_chat)
add_test(NAME test_match     COMMAND manager ${CMAKE_SOURCE_DIR}/tests/test_files/test_match)
add_test(NAME test_reconnect COMMAND manager ${CMAKE_SOURCE_DIR}/tests/test_files/test_reconnect)