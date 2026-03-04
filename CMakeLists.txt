cmake_minimum_required(VERSION 3.25)

project(StickyNoteManager VERSION 1.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置Qt自动处理
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 设置Qt6路径，优先使用MSVC版本
list(APPEND CMAKE_PREFIX_PATH "D:/Qt/6.10.2/msvc2022_64")


# 查找Qt6组件
find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets)

# 启用Qt6默认编译器设置
qt_standard_project_setup()

# ---- 源文件 ----
set(SOURCES
    src/main.cpp
    src/ui/mainwindow.cpp
    src/ui/tilebar.cpp
    src/ui/sidebar.cpp
    src/utils/windowhelper.cpp
)


# ---- 头文件 ----
set(HEADERS
    src/ui/mainwindow.h
    src/ui/tilebar.h
    src/ui/sidebar.h
    src/utils/windowhelper.h
)


# 添加可执行文件
add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
)


# 链接Qt库
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

# 设置包含目录
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils
)



# 设置目标属性
set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

# Windows特定设置
if(WIN32)
    set_property(TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")

endif()

# MSVC特定设置：启用正确的 __cplusplus 宏报告和标准符合性，/utf-8 解决中文源码编译问题
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /Zc:__cplusplus /permissive- /utf-8)
endif()


# 安装配置
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION .
)