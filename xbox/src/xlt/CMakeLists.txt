# =============================================================================
# File: CMakeLists
# Author: Oswin
# Email: Oswin_ou@intretech.com
# Created On: 2025-07-01
# Description: 编译基本库`libxtool.a`静态库, 该库被其他模块依赖, 并且只依赖标准C99库
# ============================================================================

cmake_minimum_required(VERSION 3.10)

set(xbox_module "xlt")

# Define the path for the generated version header
set(version_header ${CMAKE_CURRENT_SOURCE_DIR}/xbox_version.h)

aux_source_directory(. xbox_module_srcs)

add_library(
    ${xbox_module} STATIC
    ${xbox_module_srcs}
    ${version_header}  # Add generated header to the library sources
)

target_include_directories(
    ${xbox_module} PUBLIC
    .
)
target_link_libraries(
    ${xbox_module} PUBLIC
    ${PROJECT_NAME}::dependencies
)
add_library(
    ${PROJECT_NAME}::${xbox_module} ALIAS ${xbox_module}
)
target_compile_definitions(
    ${xbox_module} PUBLIC
    XBOX_ENABLE_XJSON
#     XBOX_ENABLE_BACKTRACE
)
