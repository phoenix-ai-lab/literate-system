cmake_minimum_required(VERSION 3.16)

project(QtNoMoc
    VERSION 1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_executable(QtNoMoc
    src/nomoc.cpp
)

target_link_libraries(QtNoMoc
    PRIVATE Qt6::Widgets
)

target_compile_options(QtNoMoc PRIVATE
    -Wall -Wextra -Wpedantic
)
