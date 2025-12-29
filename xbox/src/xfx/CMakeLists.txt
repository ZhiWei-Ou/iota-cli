# =============================================================================
# File: CMakeLists.txt
# Author: Oswin
# Email: Oswin_ou@intretech.com
# Created On: 2025-07-09
# Description: 编译`libxframework.a`静态库
#              编译选项`xframework_xxxx`, 使用`cmake -LAH`查看
# ============================================================================
cmake_minimum_required(VERSION 3.10)
set(xbox_module "xfx")

aux_source_directory(. dependencies_srcs)
list(APPEND dependencies_srcs
    ./sinks/basic_file_sink.c
    ./sinks/console_sink.c
    ./sinks/rotating_file_sink.c
    ./sinks/broker_sink.c
)
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    list(APPEND dependencies_srcs
        ./sinks/syslog_sink.c
    )

    if (xframework_enable_systemd)
        # if NOT find PkgConfig, try to install the `pkg-config`. e.g. `sudo apt install pkg-config`
        find_package(PkgConfig REQUIRED)
        # if NOT find systemd, try to install the `libsystemd-dev`. e.g. `sudo apt install libsystemd-dev`
        pkg_check_modules(SYSTEMD REQUIRED libsystemd)

        list(APPEND dependencies_srcs
            ./sinks/systemd_sink.c
        )
    endif()
endif()

if (xframework_enable_NATS)
    # If NOT find cnats, try to install the `libnats-dev`. e.g. `sudo apt install libnats-dev`
    find_package(cnats REQUIRED)
    message (STATUS "Found cnats: ${cnats_FOUND}")

    list(APPEND dependencies_srcs
        ./broker/broker_nats.c
    )
endif()

if (xframework_enable_MQTT)
    # non-exists related CMake configuration file
    # if NOT find mosquitto, try to install the `libmosquitto-dev`. e.g. `sudo apt install libmosquitto-dev`
    # find_package(mosquitto REQUIRED)

    list(APPEND dependencies_srcs
        ./broker/broker_mqtt.c
    )

endif()

if (xframework_enable_mbus)
  list(APPEND dependencies_srcs
    mbus/mbus.c
    mbus/mbus_message.c
  )
endif()

add_library(
    ${xbox_module} STATIC
    ${dependencies_srcs}
)
target_include_directories(
    ${xbox_module} BEFORE
    PUBLIC ..
    PRIVATE . sinks
    PUBLIC $<$<BOOL:${xframework_enable_systemd}>:${SYSTEMD_INCLUDE_DIRS}>
)
target_link_libraries(
    ${xbox_module}
    PUBLIC ${PROJECT_NAME}::xos ${PROJECT_NAME}::xlt
    PRIVATE $<$<BOOL:${xframework_enable_NATS}>:cnats::nats>
    PUBLIC $<$<BOOL:${xframework_enable_MQTT}>:mosquitto>
    PUBLIC $<$<BOOL:${xframework_enable_systemd}>:${SYSTEMD_LIBRARIES}>
)
target_link_directories(
    ${xbox_module}
    PUBLIC /opt/homebrew/lib
)
add_library(
    ${PROJECT_NAME}::${xbox_module} ALIAS ${xbox_module}
)


