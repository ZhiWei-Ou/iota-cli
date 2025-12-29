# =============================================================================
# File: CMakeLists
# Author: Oswin
# Email: Oswin_ou@intretech.com
# Created On: 2025-06-25
# Description: 
# ============================================================================

cmake_minimum_required(VERSION 3.10)

project(xbox LANGUAGES C CXX)

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(CMAKE_BUILD_TYPE Debug)
    option(xbox_build_module_os "Build os submodule of xbox" ON)
    option(xbox_build_module_xframework "Build xframework submodule of xbox" ON)
    option(xbox_build_benchmarks "Build xbox benchmarks" ON)
    option(xbox_build_tests "Build GTest-based unit tests" ON)
    option(xbox_build_examples "Build examples" ON)
    option(xframework_enable_NATS "Enable NATS as a broker backend in xframework" ON)
    option(xframework_enable_MQTT "Enable MQTT as a broker backend in xframework" ON)
    option(xframework_enable_mbus "Enable mbus in xframework" ON)
    option(xframework_enable_systemd "Enable systemd support in xframework" ON)
    set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin/${CMAKE_SYSTEM_PROCESSOR})
endif()

option(xbox_build_module_os "Build os submodule of xbox" ON)
option(xbox_build_module_xframework "Build xframework submodule of xbox" ON)
option(xbox_build_tests "Build GTest-based unit tests" OFF)
option(xbox_build_benchmarks "Build xbox benchmarks" OFF)
option(xbox_build_examples "Build examples" OFF)
option(xframework_enable_NATS "Enable NATS as a broker backend in xframework" OFF)
option(xframework_enable_MQTT "Enable MQTT as a broker backend in xframework" OFF)
option(xframework_enable_mbus "Enable mbus in xframework" OFF)
option(xframework_enable_systemd "Enable systemd support in xframework" OFF)

add_subdirectory(src/dependencies)
add_subdirectory(src/xlt)

if (xbox_build_module_os OR xbox_build_module_xframework)
    add_subdirectory(src/xos)
endif()

if (xbox_build_module_xframework)
    add_subdirectory(src/xfx)
endif()

if (xbox_build_benchmarks OR xbox_build_tests)
    add_subdirectory(test)
endif()
