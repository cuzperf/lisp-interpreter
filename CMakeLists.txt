cmake_minimum_required(VERSION 3.10)
project(lisp-interpreter C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(lisp lisp.c)

if (MSVC)
    # 部分 VS 版本 add_compile_options 不生效
    target_compile_options(lisp PRIVATE 
        /W4      # 警告等级4
        /WX      # 警告视为错误
        /wd4996  # 忽略 C4996  #'strcpy': This function or variable may be unsafe. Consider using strcpy_s instead.
    )
else()
     add_compile_options(-Werror -Wall -Wextra)
endif()

if (MSVC)
    # 设置启动项
    set_property(DIRECTORY ${CMAKE_SOURCE_DIR}
            PROPERTY VS_STARTUP_PROJECT lisp)
    # 设置工作目录
    set_property(TARGET lisp PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
endif()
