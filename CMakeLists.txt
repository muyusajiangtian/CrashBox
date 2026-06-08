cmake_minimum_required(VERSION 3.16)
project(CrashBox VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# SFML 路径配置
set(SFML_DIR "E:/SFML/SFML-3.0.1-gcc15/lib/cmake/SFML" CACHE PATH "SFML CMake配置路径")

# 静态链接SFML
set(SFML_STATIC_LIBRARIES TRUE)
set(BUILD_SHARED_LIBS OFF)

# 查找 SFML
find_package(SFML 3.0 REQUIRED COMPONENTS Graphics Window System)

# 头文件目录
include_directories(${CMAKE_SOURCE_DIR}/include)

# 源文件
set(SOURCES
    src/main.cpp
)

# 可执行文件
add_executable(${PROJECT_NAME} ${SOURCES})

# 静态链接运行时
target_link_options(${PROJECT_NAME} PRIVATE -static-libgcc -static-libstdc++ -static)

# 链接 SFML（静态）
target_link_libraries(${PROJECT_NAME} PRIVATE SFML::Graphics SFML::Window SFML::System)

# 复制配置文件到构建目录
file(COPY ${CMAKE_SOURCE_DIR}/config DESTINATION ${CMAKE_BINARY_DIR})

message(STATUS "CrashBox 配置完成 (静态链接)")
message(STATUS "SFML_DIR: ${SFML_DIR}")
