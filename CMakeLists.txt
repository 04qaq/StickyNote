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

# 设置Qt6路径（使用环境变量或自动查找）
if(DEFINED ENV{Qt6_DIR})
    list(APPEND CMAKE_PREFIX_PATH $ENV{Qt6_DIR})
endif()

# 查找Qt6组件
find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets)

# 启用Qt6默认编译器设置
qt_standard_project_setup()

# ---- 源文件 ----
set(SOURCES
    src/main.cpp
    # UI 层
    src/ui/mainwindow.cpp
    src/ui/titlebar.cpp
    src/ui/sidebar.cpp
    # 工具类
    src/utils/windowhelper.cpp
)

# ---- 头文件 ----
set(HEADERS
    src/ui/mainwindow.h
    src/ui/titlebar.h
    src/ui/sidebar.h
    src/utils/windowhelper.h
)

# ---- 资源文件 ----
set(RESOURCES
    resources/StickyNoteManager.qrc
)

# 添加可执行文件
add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
    ${RESOURCES}
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
    set_property(TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

# MSVC特定设置：启用正确的 __cplusplus 宏报告和标准符合性
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /Zc:__cplusplus /permissive-)
endif()

# 安装配置
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION .
)