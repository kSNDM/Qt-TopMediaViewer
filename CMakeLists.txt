set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ⚠️ CMake 最低版本建议调高到 3.16，Qt6 对低版本支持不好
cmake_minimum_required(VERSION 3.16) 
project(OnShow LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_PREFIX_PATH "D:/QT/6.12.0/mingw_64") # Qt Kit Dir
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# ⚠️ Qt6 强制要求 C++17，把原来的 11 改掉
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ⚠️【核心修改】加上了 Multimedia 和 MultimediaWidgets 模块
find_package(Qt6 COMPONENTS Widgets Multimedia MultimediaWidgets REQUIRED)

aux_source_directory(./src srcs)

# Specify MSVC UTF-8 encoding   
add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

add_executable(${PROJECT_NAME}
    WIN32 # If you need a terminal for debug, please comment this statement 
    ${srcs} 
) 

# ⚠️【核心修改】链接库时要加上多媒体库
target_link_libraries(${PROJECT_NAME} PRIVATE 
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::MultimediaWidgets
)