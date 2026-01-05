cmake_minimum_required(VERSION 3.10.0)

set(VERSION "0.0.1")
set(PROJECT_NAME "iota-cli")

project(${PROJECT_NAME} VERSION ${VERSION} LANGUAGES C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(OpenSSL REQUIRED)

file(GLOB UTILS_SRC "utils/*.c")
add_executable(${PROJECT_NAME} main.c exec.c version.c checkout.c upgrade.c ${UTILS_SRC})
target_include_directories(${PROJECT_NAME} PRIVATE . ${OPENSSL_INCLUDE_DIR} "utils")
target_link_libraries(${PROJECT_NAME} PRIVATE ${OPENSSL_CRYPTO_LIBRARY} archive)

install(TARGETS ${PROJECT_NAME} DESTINATION bin)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

include(CTest)
enable_testing()
