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
    src/ui/notelistview.cpp
    src/ui/notecarddelegate.cpp
    src/ui/noteeditdialog.cpp
    src/ui/notepreviewdialog.cpp
    src/utils/windowhelper.cpp

    src/ui/components/colorselector.cpp


    src/common/notedata.cpp
    src/core/notemanager.cpp
    src/core/notecontroller.cpp
    src/models/notelistmodel.cpp
    src/models/notefilterproxymodel.cpp
)


# ---- 头文件 ----
set(HEADERS
    src/ui/mainwindow.h
    src/ui/tilebar.h
    src/ui/sidebar.h
    src/ui/notelistview.h
    src/ui/notecarddelegate.h
    src/ui/noteeditdialog.h
    src/ui/notepreviewdialog.h
    src/utils/windowhelper.h

    src/ui/components/colorselector.h


    src/common/notedata.h
    src/common/global.h
    src/core/notemanager.h
    src/core/notecontroller.h

    src/models/notelistmodel.h
    src/models/notefilterproxymodel.h
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
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/components
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/models
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

# ================================================================
# 单元测试目标
# ================================================================
find_package(Qt6 REQUIRED COMPONENTS Test)
enable_testing()

# 测试目标共享的业务逻辑源文件（去掉 main.cpp，避免 main 重复定义）
set(TEST_SOURCES
    src/common/notedata.cpp
    src/core/notemanager.cpp
    src/models/notelistmodel.cpp
    src/models/notefilterproxymodel.cpp
)

add_executable(test_notefilterproxymodel
    tests/test_notefilterproxymodel.cpp
    ${TEST_SOURCES}
)

target_include_directories(test_notefilterproxymodel PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/models
)

target_link_libraries(test_notefilterproxymodel PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Test
)

if(MSVC)
    target_compile_options(test_notefilterproxymodel PRIVATE /Zc:__cplusplus /permissive- /utf-8)
    set_property(TARGET test_notefilterproxymodel PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
endif()

# 测试程序必须是控制台程序，否则 QtTest 的输出会被 Windows 吞掉
set_target_properties(test_notefilterproxymodel PROPERTIES
    WIN32_EXECUTABLE FALSE
)


# 注册到 CTest，执行 ctest 时自动运行
add_test(NAME FilterProxyModelTest COMMAND test_notefilterproxymodel)

# ---- NoteManager 增删改测试 ----
add_executable(test_notemanager
    tests/test_notemanager.cpp
    ${TEST_SOURCES}
)

target_include_directories(test_notemanager PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/models
)

target_link_libraries(test_notemanager PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Test
)

if(MSVC)
    target_compile_options(test_notemanager PRIVATE /Zc:__cplusplus /permissive- /utf-8)
    set_property(TARGET test_notemanager PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
endif()

set_target_properties(test_notemanager PROPERTIES
    WIN32_EXECUTABLE FALSE
)

add_test(NAME NoteManagerTest COMMAND test_notemanager)

