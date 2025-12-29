
# 模版文件转换
# @VARIABLE:
#   APP_VERSION_MAJOR: 主版本号（自定义）
#   APP_VERSION_MINOR: Git Tag数量
#   APP_VERSION_PATCH: Git Commit提交次数
#   APP_COMMIT_HASH: Git 提交短哈希
function(XBoxVersion_ConvertTemplateFileByGit TEMPLATE_FILE)

    cmake_parse_arguments(
        ARG "" "OUTPUT_PATH;OUTPUT_FILE;VERSION_MAJOR" "" ${ARGN}
    )

    if(NOT ARG_OUTPUT_PATH)
        message(FATAL_ERROR "OUTPUT_PATH is required.")
    endif()

    if (NOT ARG_OUTPUT_FILE)
        message(FATAL_ERROR "OUTPUT_FILE is required.")
    endif()

    if(NOT ARG_VERSION_MAJOR)
        message(FATAL_ERROR "VERSION_MAJOR is required.")
    endif()

    execute_process(
        COMMAND git tag
        COMMADN wc -l
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_TAG_COUNT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    string(STRIP "${GIT_TAG_COUNT}" GIT_TAG_COUNT)
    if (GIT_TAG_COUNT STREQUAL "")
        set(GIT_TAG_COUNT 0)
    endif()

    execute_process(
        COMMAND git rev-list --count HEAD
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_COUNT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )

    execute_process(
        COMMAND git rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )

    execute_process(
        COMMAND git rev-parse HEAD
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_FULL_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )

    set(APP_VERSION_MAJOR ${ARG_VERSION_MAJOR})
    set(APP_VERSION_MINOR ${GIT_TAG_COUNT})
    set(APP_VERSION_PATCH ${GIT_COMMIT_COUNT})
    set(APP_COMMIT_HASH ${GIT_COMMIT_HASH})
    set(APP_COMMIT_FULL_HASH ${GIT_COMMIT_FULL_HASH})

    configure_file(
        ${TEMPLATE_FILE}
        ${CMAKE_CURRENT_LIST_DIR}/${ARG_OUTPUT_PATH}/${ARG_OUTPUT_FILE}
    )

    message(STATUS "Convert version template file to: ${CMAKE_CURRENT_LIST_DIR}/${ARG_OUTPUT_PATH}/${ARG_OUTPUT_FILE}")
endfunction()

