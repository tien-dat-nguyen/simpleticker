cmake_minimum_required(VERSION 3.17)
project(bnbbtc_ticker)

set(CMAKE_CXX_STANDARD 20)

set(Boost_INCLUDE_DIR "/home/dat.tien.nguyen/projects/boost_1_76_0")

find_package(Boost COMPONENTS system unit_test_framework REQUIRED)
if (Boost_VERSION VERSION_LESS 1.76)
    message(FATAL_ERROR "Boost 1.76 or higher required")
endif ()

find_package(OpenSSL COMPONENTS)
find_package(Threads)

file(GLOB_RECURSE src_files CONFIGURE_DEPENDS "src/*.cpp" "src/*.hpp")
add_executable(bnbbtc_ticker ${src_files})
target_link_libraries(bnbbtc_ticker PUBLIC
        Boost::system
        OpenSSL::SSL 
        OpenSSL::Crypto
        Threads::Threads)
target_include_directories(bnbbtc_ticker PRIVATE src)

target_compile_options(bnbbtc_ticker PUBLIC -fcoroutines)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(bnbbtc_ticker PUBLIC -Wall -Werror -Wextra -pedantic)
endif ()

# Tests
set(TESTS_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/main.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/websocket-client.cpp"
)
add_executable(simple-ticker-tests ${TESTS_SOURCES})
target_compile_features(simple-ticker-tests
    PRIVATE
        cxx_std_20
)
target_link_libraries(simple-ticker-tests
    PRIVATE
    Boost::system
    OpenSSL::SSL 
    OpenSSL::Crypto
    Threads::Threads
)

add_test(
    NAME simple-ticker-tests
    COMMAND $<TARGET_FILE:simple-ticker-tests>
)