cmake_minimum_required(VERSION 3.21)

project(OS_MEM_11_Nhom03 VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

add_library(memory_core STATIC source/memory_manager.cpp)
target_include_directories(memory_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/source)
set_target_properties(memory_core PROPERTIES AUTOMOC OFF AUTOUIC OFF)

qt_add_executable(OS_MEM_11 WIN32
    source/main.cpp
    source/main_window.cpp
    source/memory_map_widget.cpp
)

target_link_libraries(OS_MEM_11 PRIVATE Qt6::Widgets)

include(CTest)
if(BUILD_TESTING)
    add_executable(test_memory_manager tests/test_memory_manager.cpp)
    target_link_libraries(test_memory_manager PRIVATE memory_core)
    set_target_properties(test_memory_manager PROPERTIES AUTOMOC OFF AUTOUIC OFF)
    add_test(NAME memory_manager COMMAND test_memory_manager)
endif()

if(MSVC)
    target_compile_options(OS_MEM_11 PRIVATE /utf-8)
endif()
