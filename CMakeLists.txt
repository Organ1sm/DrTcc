message("Identifying the OS...")
if (WIN32)
    cmake_minimum_required(VERSION 3.17)
    message("This is Windows.")
elseif (APPLE)
    message("This is MacOS.")
elseif (UNIX)
    cmake_minimum_required(VERSION 3.10)
    message("This is Linux.")
endif ()

project(DrTcc)
set(CMAKE_CXX_FLAGS -m32)
aux_source_directory(src DIR_SRCS)
include_directories("${PROJECT_SOURCE_DIR}/include") # 头文件包含目录
SET(EXECUTABLE_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/bin") # 可执行文件输出目录

set(CMAKE_CXX_STANDARD 14)

add_executable(happy
        ${DIR_SRCS}
        src/main.cpp
        include/Type.h
        include/ImportSTL.h
        include/MemoryPool.h
        include/Token.h include/Token.cpp
        include/Lexer.h include/Lexer.cpp
        include/Parser.h include/Parser.cpp
        include/AST.h include/AST.cpp
        include/GenCode.h include/GenCode.cpp
        include/VM.h include/VM.cpp
        )