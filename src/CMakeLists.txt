file(GLOB_RECURSE module_files CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cppm"
)

add_library(module STATIC)

target_sources(module
        PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES
        ${module_files}
)

# ---------------------------------------------------------
# hiredis（手动查找）
# ---------------------------------------------------------
find_path(HIREDIS_INCLUDE_DIR
        NAMES hiredis/hiredis.h
        PATHS /usr/include
)

find_library(HIREDIS_LIBRARY
        NAMES hiredis
        PATHS /usr/lib
)

if(NOT HIREDIS_INCLUDE_DIR OR NOT HIREDIS_LIBRARY)
    message(FATAL_ERROR "hiredis not found")
endif()

# ---------------------------------------------------------
# redis-plus-plus（手动查找，不使用 find_package）
# ---------------------------------------------------------
find_path(REDIS_PLUS_PLUS_INCLUDE_DIR
        NAMES sw/redis++/redis++.h
        PATHS /usr/include
)

find_library(REDIS_PLUS_PLUS_LIBRARY
        NAMES redis++
        PATHS /usr/lib
)

if(NOT REDIS_PLUS_PLUS_INCLUDE_DIR OR NOT REDIS_PLUS_PLUS_LIBRARY)
    message(FATAL_ERROR "redis-plus-plus not found")
endif()

# ---------------------------------------------------------
# MariaDB（手动查找）
# ---------------------------------------------------------
find_path(MARIADB_INCLUDE_DIR
        NAMES mysql.h
        PATH_SUFFIXES mysql
        PATHS /usr/include
)

find_library(MARIADB_LIBRARY
        NAMES mariadb
        PATHS /usr/lib
)

if(NOT MARIADB_INCLUDE_DIR OR NOT MARIADB_LIBRARY)
    message(FATAL_ERROR "MariaDB Connector C not found")
endif()

# ---------------------------------------------------------
# 链接依赖到 module
# ---------------------------------------------------------


target_link_libraries(module PRIVATE
        ${MARIADB_LIBRARY}
        ${HIREDIS_LIBRARY}
        ${REDIS_PLUS_PLUS_LIBRARY}
        -lcrypto
        -lssl
)
