#include "mainwindow.h"
#include "tilebar.h"
#include "sidebar.h"
#include "notelistview.h"
#include "notecarddelegate.h"
#include "models/notelistmodel.h"
#include "core/notemanager.h"
#include "core/notecontroller.h"
#include "core/undostack.h"
#include "models/notefilterproxymodel.h"
#include "ui/components/toastmanager.h"





#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QShortcut>
#include <QKeySequence>



#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QCloseEvent>
#include <QMouseEvent>
#include <QEvent>
#include <QSettings>


#include <QScreen>

MainWindow::MainWindow(QWidget* parent)
    : QWidget(parent)
{
    // 去掉系统边框，保留任务栏图标
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowMinimizeButtonHint);
    // 背景透明，用于实现圆角和阴影
    setAttribute(Qt::WA_TranslucentBackground);

    setMinimumSize(400 + SHADOW_MARGIN * 2, 300 + SHADOW_MARGIN * 2);
    resize(900 + SHADOW_MARGIN * 2, 600 + SHADOW_MARGIN * 2);

    // 初始化缩放辅助类
    window_helper_ = new WindowHelper(this, SHADOW_MARGIN, RESIZE_MARGIN);

    initUI();
    connectSignals();
    initTray();
    initShortcuts();
    restoreWindowState();

    // 初始化 Toast 管理器，绑定父窗口（用于定位）
    ToastManager::instance()->setParentWidget(this);


    // 为 MainWindow 自身安装过滤器（覆盖阴影区域等无子控件区域）
    installEventFilter(this);


}


MainWindow::~MainWindow()
{
}

// ----------------------------------------------------------------
// 界面初始化
// ----------------------------------------------------------------
void MainWindow::initUI()
{
    // 最外层布局（为阴影留出边距）
    QVBoxLayout* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(SHADOW_MARGIN, SHADOW_MARGIN,
                                    SHADOW_MARGIN, SHADOW_MARGIN);
    outerLayout->setSpacing(0);

    // 内容容器
    QWidget* contentWidget = new QWidget(this);
    contentWidget->setObjectName("contentWidget");
    outerLayout->addWidget(contentWidget);

    // 内容容器布局
    QVBoxLayout* contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);

    // 标题栏占位（可拖拽区域）
    tile_bar_ = new TileBar(contentWidget);
    contentLayout->addWidget(tile_bar_);

    // ---- 左右分栏容器 ----
    QWidget* bodyWidget = new QWidget(contentWidget);
    bodyWidget->setObjectName("bodyWidget");
    QHBoxLayout* bodyLayout = new QHBoxLayout(bodyWidget);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(0);

    // 左侧侧边栏
    sidebar_ = new SideBar(bodyWidget);
    bodyLayout->addWidget(sidebar_);

    // 右侧内容区
    content_widget_ = new QWidget(bodyWidget);
    content_widget_->setObjectName("contentArea");
    content_layout_ = new QVBoxLayout(content_widget_);
    content_layout_->setContentsMargins(0, 0, 0, 0);
    content_layout_->setSpacing(0);


    // 便签列表视图
    note_model_     = new NoteListModel(this);
    note_proxy_model_ = new NoteFilterProxyModel(this);
    note_proxy_model_->setSourceModel(note_model_);   // 绑定数据源，筛选才能生效
    note_delegate_  = new NoteCardDelegate(this);
    note_list_view_ = new NoteListView(content_widget_);
    note_list_view_->setModel(note_proxy_model_);
    note_list_view_->setItemDelegate(note_delegate_);
    content_layout_->addWidget(note_list_view_);


    bodyLayout->addWidget(content_widget_, 1);   // stretch = 1，占满剩余宽度

    // 创建 Controller，注入 View 和 Model
    note_controller_ = new NoteController(note_list_view_, note_model_, note_proxy_model_, this);

    // 初始化数据（加载持久化数据，首次运行插入演示便签）
    note_controller_->initData();

    // 把 NoteManager 中已有的分类同步到侧边栏
    sidebar_->refreshCategories();




    contentLayout->addWidget(bodyWidget, 1);

    contentWidget->setStyleSheet(R"(
        QWidget#contentWidget {
            background-color: #F5F6FA;
            border-radius: 8px;
        }
        QWidget#bodyWidget {
            background: transparent;
        }
        QWidget#contentArea {
            background-color: #F5F6FA;
            border-bottom-right-radius: 8px;
        }
    )");

    // 递归为所有子孙控件安装事件过滤器并开启鼠标追踪
    for (QWidget* w : findChildren<QWidget*>()) {
        w->installEventFilter(this);
        w->setMouseTracking(true);
    }
}


void MainWindow::connectSignals() {

    connect(tile_bar_, &TileBar::minimizeRequested, this, &MainWindow::onMinimizeRequested);
    connect(tile_bar_, &TileBar::maximizeRequested, this, &MainWindow::onMaximizeRequested);
    connect(tile_bar_, &TileBar::closeRequested, this, &MainWindow::onCloseRequested);
    // 将 View 的用户操作信号连接到 Controller（业务逻辑由 Controller 处理）
    connect(note_list_view_, &NoteListView::doubleClicked,          note_controller_, &NoteController::onNoteDoubleClicked);
    connect(note_list_view_, &NoteListView::deleteNoteRequested,    note_controller_, &NoteController::onNoteDeleted);
    connect(note_list_view_, &NoteListView::editNoteRequested,      note_controller_, &NoteController::onNoteEdited);
    connect(note_list_view_, &NoteListView::pinToggleRequested,     note_controller_, &NoteController::onNotePinToggled);
    connect(note_list_view_, &NoteListView::changeCategoryRequested,note_controller_, &NoteController::onNoteCategoryChanged);
    connect(note_list_view_, &NoteListView::changeColorRequested,   note_controller_, &NoteController::onNoteColorChanged);
    connect(note_list_view_, &NoteListView::openPreviewRequested,   note_controller_, &NoteController::onNotePreviewRequested);
    connect(note_list_view_, &NoteListView::deleteMultipleRequested,note_controller_, &NoteController::onNotesDeletedMultiple);
    connect(tile_bar_,       &TileBar::newNoteRequested,            note_controller_, &NoteController::onNewNoteRequested);

    connect(sidebar_, &SideBar::categoryChanged, note_proxy_model_,
        &NoteFilterProxyModel::setCategory);
    connect(tile_bar_, &TileBar::searchTextChanged, note_proxy_model_,
        &NoteFilterProxyModel::setKeyword);

}
    


void MainWindow::onMinimizeRequested()
{
    showMinimized();
}

void MainWindow::onMaximizeRequested()
{
    if (isMaximized()) {
        showNormal();
    }
    else {
        showMaximized();
    }
}

void MainWindow::onCloseRequested()
{
    close();  // 会触发 closeEvent，自动保存窗口状态
}


// ----------------------------------------------------------------
// 窗口状态持久化

// ----------------------------------------------------------------
void MainWindow::saveWindowState()
{
    QSettings settings("StickyNoteManager", "StickyNoteManager");
    settings.setValue("mainwindow/geometry", saveGeometry());
    settings.setValue("mainwindow/maximized", isMaximized());
}

void MainWindow::restoreWindowState()
{
    QSettings settings("StickyNoteManager", "StickyNoteManager");

    if (settings.contains("mainwindow/geometry")) {
        restoreGeometry(settings.value("mainwindow/geometry").toByteArray());
    } else {
        QScreen* screen = QApplication::primaryScreen();
        if (screen) {
            QRect screenRect = screen->availableGeometry();
            move(screenRect.center() - rect().center());
        }
    }

    if (settings.value("mainwindow/maximized", false).toBool()) {
        showMaximized();
    }
}

// ----------------------------------------------------------------
// 事件处理
// ----------------------------------------------------------------
void MainWindow::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)

    if (isMaximized()) {
        QPainter painter(this);
        painter.fillRect(rect(), QColor("#F5F6FA"));
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QRect shadowRect = rect().adjusted(SHADOW_MARGIN, SHADOW_MARGIN,
                                       -SHADOW_MARGIN, -SHADOW_MARGIN);

    // 绘制多层模糊阴影
    for (int i = SHADOW_MARGIN; i > 0; --i) {
        int alpha = static_cast<int>(50.0 * (1.0 - static_cast<double>(i) / SHADOW_MARGIN));
        QPen shadowPen(QColor(0, 0, 0, alpha));
        shadowPen.setWidth(1);
        painter.setPen(shadowPen);
        painter.setBrush(Qt::NoBrush);
        QRect r = shadowRect.adjusted(-i, -i, i, i);
        QPainterPath path;
        path.addRoundedRect(r, CORNER_RADIUS + i, CORNER_RADIUS + i);
        painter.drawPath(path);
    }

    // 绘制圆角背景
    QPainterPath bgPath;
    bgPath.addRoundedRect(shadowRect, CORNER_RADIUS, CORNER_RADIUS);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor("#F5F6FA"));
    painter.drawPath(bgPath);
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    update();
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    // 如果托盘图标可见，关闭时隐藏到托盘而不是退出
    if (tray_icon_ && tray_icon_->isVisible()) {
        hide();
        event->ignore();  // 阻止默认的关闭行为
        tray_icon_->showMessage(
            "便签管理器",
            "程序已最小化到系统托盘，双击图标可恢复窗口",
            QSystemTrayIcon::Information,
            2000
        );
    } else {
        saveWindowState();
        QWidget::closeEvent(event);
    }
}


void MainWindow::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::WindowStateChange) {
        if (tile_bar_) {
            tile_bar_->updateMaximizeButton(isMaximized());
        }

        QVBoxLayout* outerLayout = qobject_cast<QVBoxLayout*>(layout());
        if (outerLayout) {
            int margin = isMaximized() ? 0 : SHADOW_MARGIN;
            outerLayout->setContentsMargins(margin, margin, margin, margin);
        }
        update();
    }
    QWidget::changeEvent(event);
}

// ----------------------------------------------------------------
// 拖拽移动
// ----------------------------------------------------------------
void MainWindow::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && !isMaximized()) {
        QPoint pos = event->position().toPoint();

        // 优先检测边缘缩放
        ResizeEdge edge = window_helper_->hitTest(pos);
        if (edge != ResizeEdge::None) {
            window_helper_->startResize(edge, event->globalPosition().toPoint());
            event->accept();
            return;
        }

        // 标题栏区域：拖拽移动
        int titleBottom = SHADOW_MARGIN + TITLE_BAR_HEIGHT;
        if (pos.y() <= titleBottom && pos.y() >= SHADOW_MARGIN) {
            m_dragStartPos = event->globalPosition().toPoint() - frameGeometry().topLeft();
            m_isDragging = true;
            event->accept();
            return;
        }
    }
    QWidget::mousePressEvent(event);
}


void MainWindow::mouseMoveEvent(QMouseEvent* event)
{
    QPoint pos       = event->position().toPoint();
    QPoint globalPos = event->globalPosition().toPoint();

    // 正在缩放
    if (window_helper_->isResizing()) {
        window_helper_->doResize(globalPos);
        event->accept();
        return;
    }

    // 正在拖拽移动
    if (m_isDragging && (event->buttons() & Qt::LeftButton)) {
        if (isMaximized()) {
            showNormal();
            m_dragStartPos = QPoint(width() / 2, TITLE_BAR_HEIGHT / 2);
        }
        move(globalPos - m_dragStartPos);
        event->accept();
        return;
    }

    // 未按下鼠标：更新光标形状（悬停反馈）
    if (!isMaximized()) {
        ResizeEdge edge = window_helper_->hitTest(pos);
        window_helper_->updateCursor(edge);
    }

    QWidget::mouseMoveEvent(event);
}


void MainWindow::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        if (window_helper_->isResizing()) {
            window_helper_->stopResize();
            event->accept();
            return;
        }
        if (m_isDragging) {
            // 只有真正在拖拽时才消费事件
            m_isDragging = false;
            event->accept();
            return;
        }
    }
    QWidget::mouseReleaseEvent(event);
}


void MainWindow::leaveEvent(QEvent* event)
{
    // 鼠标离开窗口时恢复默认光标（缩放中不恢复）
    if (!window_helper_->isResizing()) {
        setCursor(Qt::ArrowCursor);
    }
    QWidget::leaveEvent(event);
}

// ----------------------------------------------------------------
// 事件过滤器：拦截所有子控件的鼠标移动事件，更新光标形状
// 子控件坐标需转换为 MainWindow 坐标后再做 hitTest
// ----------------------------------------------------------------
// ----------------------------------------------------------------
// 系统托盘初始化
// ----------------------------------------------------------------
void MainWindow::initTray()
{
    tray_icon_ = new QSystemTrayIcon(QIcon(":/icons/app.png"), this);

    tray_menu_ = new QMenu(this);
    tray_menu_->addAction("显示主窗口", this, [this]() {
        show();
        raise();
        activateWindow();
    });
    tray_menu_->addAction("快速新建便签", this, [this]() {
        show();
        raise();
        activateWindow();
        note_controller_->onNewNoteRequested();
    });
    tray_menu_->addSeparator();
    tray_menu_->addAction("退出", this, [this]() {
        // 真正退出：先保存状态，再退出
        saveWindowState();
        tray_icon_->hide();
        QApplication::quit();
    });

    tray_icon_->setContextMenu(tray_menu_);
    tray_icon_->setToolTip("便签管理器");
    tray_icon_->show();

    // 双击托盘图标恢复窗口
    connect(tray_icon_, &QSystemTrayIcon::activated,
            this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::DoubleClick) {
            show();
            raise();
            activateWindow();
        }
    });
}

// ----------------------------------------------------------------
// 快捷键初始化
// ----------------------------------------------------------------
void MainWindow::initShortcuts()
{
    UndoStack* undo_stack = note_controller_->undoStack();

    // Ctrl+Z 撤销
    auto* undo_shortcut = new QShortcut(QKeySequence::Undo, this);
    connect(undo_shortcut, &QShortcut::activated, this, [this, undo_stack]() {
        if (undo_stack->canUndo()) {
            undo_stack->undo();
            ToastManager::instance()->show("已撤销");
        }
    });

    // Ctrl+Y 重做
    auto* redo_shortcut = new QShortcut(QKeySequence::Redo, this);
    connect(redo_shortcut, &QShortcut::activated, this, [this, undo_stack]() {
        if (undo_stack->canRedo()) {
            undo_stack->redo();
            ToastManager::instance()->show("已重做");
        }
    });
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event)

{
    if (event->type() == QEvent::MouseMove && !isMaximized()) {
        QWidget* w = qobject_cast<QWidget*>(watched);
        if (w) {
            // 如果鼠标在 NoteListView 内容区，不修改光标，避免干扰列表交互
            if (note_list_view_ && (w == note_list_view_ ||
                note_list_view_->isAncestorOf(w))) {
                return QWidget::eventFilter(watched, event);
            }

            QMouseEvent* me = static_cast<QMouseEvent*>(event);
            QPoint posInWindow = w->mapTo(this, me->position().toPoint());
            ResizeEdge edge = window_helper_->hitTest(posInWindow);
            // 将光标设置到鼠标实际所在的子控件上，避免被父窗口光标覆盖
            window_helper_->updateCursor(edge, w);
        }
    }
    return QWidget::eventFilter(watched, event);
}





