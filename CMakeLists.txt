cmake_minimum_required(VERSION 3.10.0)

set(VERSION "0.0.1")
set(PROJECT_NAME "iotad")

project(${PROJECT_NAME} VERSION ${VERSION} LANGUAGES C)

find_package(OpenSSL REQUIRED)
add_subdirectory(xbox)

add_executable(${PROJECT_NAME} main.c exec.c version.c checkout.c update.c)
target_include_directories(${PROJECT_NAME} PRIVATE . ${OPENSSL_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE xbox::xlt xbox::xos ${OPENSSL_CRYPTO_LIBRARY})

# add_subdirectory(test)

install(TARGETS ${PROJECT_NAME} DESTINATION bin)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

include(CTest)
enable_testing()
