# =============================================================================
# File: CMakeLists
# Author: Oswin
# Email: Oswin_ou@intretech.com
# Created On: 2025-07-01
# Description: 编译第三方依赖库，最终生产`libdependencies.a`静态库
# ============================================================================

cmake_minimum_required(VERSION 3.10)

aux_source_directory(. DEPENDENCIES_SRCS)

set(xbox_module "dependencies")

add_library(
    ${xbox_module} STATIC
    ${DEPENDENCIES_SRCS}
)
target_include_directories(
    ${xbox_module} PUBLIC
    .
)
add_library(
    ${PROJECT_NAME}::${xbox_module} ALIAS ${xbox_module}
)
