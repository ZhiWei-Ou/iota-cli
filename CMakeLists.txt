cmake_minimum_required(VERSION 3.10.0)

set(VERSION "0.1.0")
set(PROJECT_NAME "iota-cli")

project(${PROJECT_NAME} VERSION ${VERSION} LANGUAGES C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(OpenSSL REQUIRED)
pkg_check_modules(DBUS REQUIRED dbus-1)

find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_BRANCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_SHORT
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    execute_process(
        COMMAND ${GIT_EXECUTABLE} show -s --format=%cd --date=iso
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_DATE
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --tags --dirty --always
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_DESCRIBE
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-list --count HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_COUNT
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
else()
    set(GIT_BRANCH        "unknown")
    set(GIT_COMMIT_HASH  "unknown")
    set(GIT_COMMIT_SHORT "unknown")
    set(GIT_COMMIT_DATE  "unknown")
    set(GIT_DESCRIBE     "unknown")
    set(GIT_COMMIT_COUNT "0")
endif()
string(TIMESTAMP BUILD_TIME "%Y-%m-%d %H:%M:%S")
configure_file(
    ${CMAKE_SOURCE_DIR}/version.h.in
    ${CMAKE_SOURCE_DIR}/version.h
    @ONLY
)

file(GLOB SOURCES "*.c" "utils/*.c")
add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE . ${OPENSSL_INCLUDE_DIR} "utils" ${DBUS_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${OPENSSL_CRYPTO_LIBRARY} archive ${DBUS_LIBRARIES})

install(TARGETS ${PROJECT_NAME} DESTINATION bin)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

include(CTest)
enable_testing()
