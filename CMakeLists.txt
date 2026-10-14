cmake_minimum_required(VERSION 3.21)

project(OS_MEM_11_Nhom03 VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

qt_add_executable(OS_MEM_11 WIN32
    source/main.cpp
)

target_link_libraries(OS_MEM_11 PRIVATE Qt6::Widgets)

if(MSVC)
    target_compile_options(OS_MEM_11 PRIVATE /utf-8)
endif()
