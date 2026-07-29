file(GLOB_RECURSE test_module_files CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cppm"
)

add_library(test_module STATIC)

target_sources(test_module
        PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES
        ${test_module_files}
)
