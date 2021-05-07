cmake_minimum_required(VERSION 3.17)
project(bnbbtc_ticker)

set(CMAKE_CXX_STANDARD 20)

set(Boost_INCLUDE_DIR "/home/dat.tien.nguyen/projects/boost_1_76_0")

find_package(Boost COMPONENTS system REQUIRED)
if (Boost_VERSION VERSION_LESS 1.76)
    message(FATAL_ERROR "Boost 1.76 or higher required")
endif ()

find_package(OpenSSL COMPONENTS)
find_package(Threads)

file(GLOB_RECURSE src_files CONFIGURE_DEPENDS "src/*.cpp" "src/*.hpp")
add_executable(bnbbtc_ticker ${src_files})
target_link_libraries(bnbbtc_ticker PUBLIC
        Boost::system
        OpenSSL::SSL OpenSSL::Crypto
        Threads::Threads)
target_include_directories(bnbbtc_ticker PRIVATE src)

target_compile_options(bnbbtc_ticker PUBLIC -fcoroutines)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(bnbbtc_ticker PUBLIC -Wall -Werror -Wextra -pedantic)
endif ()