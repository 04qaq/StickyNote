# StickyNoteManager 开发文档

## 1. 项目概述
StickyNoteManager 是一款基于 Qt 6 (Widgets) 和 CMake 开发的桌面端便签管理工具。
本项目旨在提供一个美观、易用且功能丰富的便签管理体验，支持富文本编辑、分类管理、拖拽排序、深色模式等特性。

## 2. 开发环境
- **操作系统**: Windows / macOS / Linux
- **编译器**: MSVC 2019+ / GCC / Clang
- **构建系统**: CMake 3.25+
- **Qt 版本**: Qt 6.x (推荐 6.5 LTS)
- **IDE**: Visual Studio 2026 (推荐) / Qt Creator / VS Code


## 3. 项目结构
项目采用标准的 CMake + Qt 结构，源代码位于 `src` 目录下，按功能模块划分。

```
StickyNoteManager/
├── CMakeLists.txt              # 项目构建定义
├── resources/                  # 资源文件 (图标, QSS, 翻译等)
│   ├── icons/
│   ├── themes/
│   └── StickyNoteManager.qrc
├── src/
│   ├── main.cpp                # 程序入口
│   ├── common/                 # 通用定义与数据结构
│   │   ├── notedata.h          # 便签数据结构定义
│   │   └── global.h            # 全局常量与枚举
│   ├── core/                   # 核心业务逻辑
│   │   ├── notemanager.h/cpp   # 便签数据管理 (增删改查, JSON读写)
│   │   └── undocommands.h/cpp  # 撤销/重做命令实现
│   ├── models/                 # Qt 数据模型
│   │   ├── notelistmodel.h/cpp # 便签列表模型
│   │   └── notefilterproxy.h/cpp # 搜索与筛选代理模型
│   ├── ui/                     # 界面组件
│   │   ├── mainwindow.h/cpp    # 主窗口 (无边框容器)
│   │   ├── titlebar.h/cpp      # 自定义标题栏
│   │   ├── sidebar.h/cpp       # 侧边栏 (分类列表)
│   │   ├── notelistview.h/cpp  # 便签列表视图
│   │   ├── notecarddelegate.h/cpp # 便签卡片绘制委托
│   │   ├── noteeditdialog.h/cpp # 便签编辑/新建弹窗
│   │   ├── notewindow.h/cpp    # 独立便签小窗口
│   │   └── components/         # 通用 UI 组件
│   │       ├── iconbutton.h/cpp    # 三态图标按钮
│   │       ├── colorselector.h/cpp # 颜色选择器
│   │       └── toast.h/cpp         # 轻提示组件
│   └── utils/                  # 工具类
│       ├── windowhelper.h/cpp  # 窗口拖拽与缩放辅助
│       └── thememanager.h/cpp  # 主题切换管理
└── tests/                      # 单元测试
```

## 4. 开发里程碑与类设计

本项目开发周期预计为 1-2 周，划分为五个里程碑。

### 一期：项目骨架与自定义窗口 (Day 1-2)
**目标**: 搭建项目基础，实现无边框主窗口框架。

*   **功能点**:
    *   CMake 工程配置。
    *   无边框窗口实现（去除原生标题栏）。
    *   自定义标题栏（最小化、最大化、关闭）。
    *   窗口拖拽移动与边缘缩放。
    *   左右分栏布局（侧边栏 + 内容区）。
    *   窗口位置与尺寸的持久化保存。

*   **核心类**:
    *   `MainWindow`: 主窗口容器，负责整体布局和无边框逻辑。
    *   `TitleBar`: 自定义标题栏控件，包含窗口控制按钮。
    *   `WindowHelper`: 处理 Windows `nativeEvent` 或鼠标事件，实现无边框窗口的拖拽和缩放。
    *   `Sidebar`: 左侧分类导航栏的占位实现。

### 二期：数据层与便签列表 (Day 3-4)
**目标**: 实现便签数据的加载、存储与列表展示。

*   **功能点**:
    *   定义 `NoteData` 数据结构。
    *   实现 JSON 文件的读写（启动加载，变更保存）。
    *   Model/View 架构展示便签列表。
    *   自定义 Delegate 绘制便签卡片（标题、预览、时间、背景色）。

*   **核心类**:
    *   `NoteData` (struct): 包含 id, title, content, color, category, createdTime 等字段。
    *   `NoteManager`: 单例或核心服务类，负责管理 `QList<NoteData>`，提供 `load()` 和 `save()` 接口。
    *   `NoteListModel`: 继承 `QAbstractListModel`，将 `NoteManager` 的数据适配给 View。
    *   `NoteCardDelegate`: 继承 `QStyledItemDelegate`，使用 `QPainter` 绘制卡片样式。
    *   `NoteListView`: 继承 `QListView`，配置 Grid 布局模式。

### 三期：编辑功能与交互 (Day 5-7)
**目标**: 实现便签的增删改查完整流程。

*   **功能点**:
    *   新建/编辑便签弹窗。
    *   富文本编辑器（加粗、颜色、列表）。
    *   分类管理（新建、删除自定义分类）。
    *   搜索功能（实时过滤）。
    *   颜色选择器。

*   **核心类**:
    *   `NoteEditDialog`: 模态对话框，包含 `QLineEdit` (标题), `QTextEdit` (内容), `ColorSelector` 等。
    *   `ColorSelector`: 自定义 Widget，绘制圆形颜色块，支持键盘选择。
    *   `NoteFilterProxyModel`: 继承 `QSortFilterProxyModel`，实现基于关键字和分类的过滤。
    *   `CategoryManager`: 管理分类列表（内置+自定义）。

### 四期：体验打磨 (Day 8-9)
**目标**: 提升交互体验，增加高级交互功能。

*   **功能点**:
    *   自定义三态图标按钮 (Hover/Pressed 效果)。
    *   Toast 轻提示。
    *   卡片 Hover 动画与拖拽排序。
    *   撤销/重做 (Undo/Redo) 操作栈。
    *   系统托盘与全局快捷键。

*   **核心类**:
    *   `IconButton`: 继承 `QPushButton` 或 `QWidget`，重写 `paintEvent` 实现三态。
    *   `ToastManager`: 管理 Toast 窗口的创建和队列显示。
    *   `UndoStack`: 封装 `QUndoStack`。
    *   `AddNoteCommand`, `DeleteNoteCommand`, `EditNoteCommand`: 继承 `QUndoCommand`，实现具体操作的撤销重做。
    *   `SystemTray`: 封装 `QSystemTrayIcon`。

### 五期：进阶挑战 (Day 10)
**目标**: 实现差异化高级功能。

*   **功能点**:
    *   独立便签小窗口（多窗口同步）。
    *   深色/浅色模式切换。
    *   数据导入/导出。
    *   统计面板。

*   **核心类**:
    *   `NoteWindow`: 独立的小型无边框窗口，复用编辑逻辑。
    *   `ThemeManager`: 负责加载 `light.qss` / `dark.qss` 并通知界面刷新。
    *   `StatisticsDialog`: 使用 `QPainter` 绘制柱状图。
    *   `DataImporter`, `DataExporter`: 处理 JSON 序列化与反序列化。

## 5. 编码规范
- **命名**: 类名大驼峰 (`NoteManager`)，变量名小驼峰 (`noteList`) 或下划线后缀 (`m_noteList`)。
- **文件**: 全小写，类名对应文件名 (`notemanager.h`, `notemanager.cpp`)。
- **注释**: 关键逻辑和头文件接口需添加注释。
- **格式**: 保持代码缩进一致（推荐 4 空格）。

## 6. 构建与运行
1.  使用 Visual Studio 2026 "打开本地文件夹" 选择项目根目录。
2.  等待 CMake 配置完成。
3.  选择启动目标 `StickyNoteManager.exe` 并运行。

