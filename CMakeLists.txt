cmake_minimum_required(VERSION 3.10)
project(lisp-interpreter C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# VS 特有配置
if (MSVC)
    # 将 example 中的示例文件加入到工程中
    file(GLOB Examples "${CMAKE_SOURCE_DIR}/examples/*")
    set_source_files_properties(${Examples} PROPERTIES HEADER_FILE_ONLY TRUE)
    source_group("examples" FILES ${Examples})

    # 将 system.lsp 加入到工程
    set_source_files_properties(system.lsp PROPERTIES HEADER_FILE_ONLY TRUE)
endif()

add_executable(lisp
    src/lisp.h
    src/lisp_core.c
    src/lisp_dump.c
    src/lisp_eval.c
    src/lisp_list.c
    src/lisp_main.c
    src/lisp_print.c
    src/lisp_read.c
    src/lisp_repl.c
    src/lisp_symbol.c

    system.lsp
    ${Examples})

# 增加编译选项
if (MSVC)
    # 部分 VS 版本 add_compile_options 不生效
    target_compile_options(lisp PRIVATE
        /WX         # 警告视为错误
        /W4         # 警告等级4
        /wd4996     # 忽略 C4996: 'strcpy': This function or variable may be unsafe. Consider using strcpy_s instead.
        /wd4200     # 忽略 C4200: 使用了非标准扩展: 结构/联合中的零大小数组.
    )
else()
    target_compile_options(lisp PRIVATE
        -Werror     # 警告视为错误
        -Wall       # 常规警告
        -Wextra     # 额外警告
    )
endif()

# VS 特有配置
if (MSVC)
    # 设置启动项
    set_property(DIRECTORY ${CMAKE_SOURCE_DIR}
            PROPERTY VS_STARTUP_PROJECT lisp)
    # 设置工作目录
    set_property(TARGET lisp PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
    if (ExampleID)
        set_property(TARGET lisp PROPERTY VS_DEBUGGER_COMMAND_ARGUMENTS "${CMAKE_SOURCE_DIR}/examples/${ExampleID}")
    endif()
endif()
