cmake_minimum_required(VERSION 3.16)
project(UN_Vela LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(unvela_core STATIC src/vela.c)
target_include_directories(unvela_core PUBLIC include vendor/aster/include)

if(WIN32)
    add_library(unvela_host STATIC host/vela_host_win32.c)
    target_link_libraries(unvela_host PUBLIC ws2_32)
else()
    add_library(unvela_host STATIC host/vela_host_posix.c)
endif()
target_include_directories(unvela_host PUBLIC include host)

add_executable(unvela_host_smoke tests/host_smoke.c)
target_link_libraries(unvela_host_smoke PRIVATE unvela_host)

enable_testing()
add_test(NAME hosted-carrier-smoke COMMAND unvela_host_smoke)
