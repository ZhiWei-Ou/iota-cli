# =============================================================================
# File: CMakeLists
# Author: Oswin
# Email: Oswin_ou@intretech.com
# Created On: 2025-07-01
# Description: 编译libos静态库, Unix下编译 `${root}/src/os/unix` 目录下的源文件
# ============================================================================

cmake_minimum_required(VERSION 3.10)

set(xbox_module "xos")

aux_source_directory(./unix os_srcs)

add_library(
    ${xbox_module} STATIC
    ${os_srcs}
)
target_include_directories(
    ${xbox_module} PUBLIC
    ./unix
)
target_link_libraries(
    ${xbox_module} PUBLIC
    ${PROJECT_NAME}::xlt
    pthread
)
add_library(
    ${PROJECT_NAME}::${xbox_module} ALIAS ${xbox_module}
)
