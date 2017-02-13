cmake_minimum_required(VERSION 2.8.0)
#Add custom cmake scripts
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

# Enable ExternalProject CMake module
include(ExternalProject)

# Set default ExternalProject root directory
set_directory_properties(PROPERTIES EP_PREFIX ${CMAKE_BINARY_DIR}/Rust)

project(rust_udf)
find_package(MySQL REQUIRED)
include_directories(${MYSQL_INCLUDE_DIR})
link_directories(${MYSQL_LIB_DIR})

include_directories("include")
ADD_LIBRARY(summation_udf MODULE src/summation_udf.c)

install(TARGETS summation_udf LIBRARY DESTINATION "${MYSQL_LIB_DIR}/mysql/plugin" COMPONENT libraries)

#Add rust_example as a CMake target
ExternalProject_Add(
   rust_summation
   DOWNLOAD_COMMAND ""
   CONFIGURE_COMMAND ""
   BUILD_COMMAND cargo build COMMAND cargo build --release
   BINARY_DIR "${CMAKE_SOURCE_DIR}"
   INSTALL_COMMAND ""
   LOG_BUILD ON)
   
# Create dependency of Example on rust_example
add_dependencies(summation_udf rust_summation)

# Specify Example's link libraries
target_link_libraries(summation_udf
    debug "${CMAKE_SOURCE_DIR}/target/debug/librust_summation.a"
    optimized "${CMAKE_SOURCE_DIR}/target/release/librust_summation.a")

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_DEBUG} -Wall -g")
