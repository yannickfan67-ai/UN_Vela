cmake_minimum_required(VERSION 3.16)
project(UN_Vela VERSION 0.3.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(unvela_core STATIC src/vela.c src/image.c)
target_include_directories(unvela_core PUBLIC include vendor/aster/include)

if(WIN32)
    add_library(unvela_host STATIC host/vela_host_win32.c)
    target_link_libraries(unvela_host PUBLIC winhttp)
else()
    find_package(CURL REQUIRED)
    add_library(unvela_host STATIC host/vela_host_posix.c)
    target_link_libraries(unvela_host PUBLIC CURL::libcurl)
endif()
target_include_directories(unvela_host PUBLIC include host)

add_executable(unvela_host_smoke tests/host_smoke.c)
target_link_libraries(unvela_host_smoke PRIVATE unvela_host)
add_executable(unvela_image_smoke tests/image_smoke.c src/image.c)
target_include_directories(unvela_image_smoke PRIVATE include)
add_executable(unvela_core_smoke tests/core_smoke.c src/vela.c)
target_include_directories(unvela_core_smoke PRIVATE include vendor/aster/include)

enable_testing()
add_test(NAME hosted-carrier-smoke COMMAND unvela_host_smoke)
add_test(NAME image-decoder-smoke COMMAND unvela_image_smoke)
add_test(NAME browser-core-smoke COMMAND unvela_core_smoke)

install(TARGETS unvela_core unvela_host ARCHIVE DESTINATION lib LIBRARY DESTINATION lib RUNTIME DESTINATION bin)
install(FILES include/vela.h include/vela_platform.h include/vela_host.h include/vela_image.h vendor/aster/include/aster.h DESTINATION include)
