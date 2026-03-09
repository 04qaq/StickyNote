#include "mainwindow.h"
#include "tilebar.h"
#include "sidebar.h"
#include "notelistview.h"
#include "notecarddelegate.h"
#include "models/notelistmodel.h"
#include "core/notemanager.h"
#include "models/notefilterproxymodel.h"
#include "ui/noteeditdialog.h"
#include "ui/notepreviewdialog.h"


#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>


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
    restoreWindowState();

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

    // 加载数据
    NoteManager::instance()->load();

    // 首次运行时插入演示便签
    if (NoteManager::instance()->notes().isEmpty()) {
        NoteManager::instance()->addNote(NoteData::createNew(
            "欢迎使用 Sticky Notes",
            "这是你的第一条便签！\n你可以在这里记录任何想法、待办事项或灵感。",
            "工作", "#FFEAA7"));
        NoteManager::instance()->addNote(NoteData::createNew(
            "今日计划",
            "1. 完成项目报告\n2. 回复邮件\n3. 下午 3 点开会",
            "工作", "#FAB1A0"));
        NoteManager::instance()->addNote(NoteData::createNew(
            "购物清单",
            "- 牛奶\n- 面包\n- 鸡蛋\n- 苹果",
            "生活", "#81ECEC"));
        NoteManager::instance()->addNote(NoteData::createNew(
            "Qt 学习笔记",
            "Model/View 架构要点：\n• QAbstractListModel 负责数据\n• QListView 负责展示\n• Delegate 负责绘制每一项",
            "学习", "#A29BFE"));
        NoteManager::instance()->addNote(NoteData::createNew(
            "读书摘录",
            "「代码是写给人读的，只是顺便让机器执行。」\n—— Harold Abelson",
            "学习", "#DFE6E9"));
    }

    note_model_->refresh();

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
    connect(note_list_view_, &NoteListView::doubleClicked, this, &MainWindow::onNoteDoubleClicked);
    connect(note_list_view_, &NoteListView::deleteNoteRequested, this, &MainWindow::onNoteDeleted);
    connect(note_list_view_, &NoteListView::editNoteRequested, this, &MainWindow::onNoteEdited);
    connect(note_list_view_, &NoteListView::pinToggleRequested, this, &MainWindow::onNotePinToggled);
    connect(note_list_view_, &NoteListView::changeCategoryRequested, this, &MainWindow::onNoteCategoryChanged);
    connect(note_list_view_, &NoteListView::changeColorRequested, this, &MainWindow::onNoteColorChanged);
    connect(note_list_view_, &NoteListView::openPreviewRequested, this, &MainWindow::onNotePreviewRequested);
    connect(note_list_view_, &NoteListView::deleteMultipleRequested, this, &MainWindow::onNotesDeletedMultiple);
    connect(tile_bar_, &TileBar::newNoteRequested, this, &MainWindow::onNewNoteRequested);
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


void MainWindow::onNewNoteRequested() {
    NoteData note;
    NoteEditDialog dialog(note, this);
	if (dialog.exec() == QDialog::Accepted) {
		NoteManager::instance()->addNote(dialog.result());
		note_model_->refresh();
	}
}

void MainWindow::onNoteDoubleClicked(const QModelIndex& proxyIndex) {
    QModelIndex sourceIndex = note_proxy_model_->mapToSource(proxyIndex);
    NoteData note = note_model_->data(sourceIndex, NoteDataRole).value<NoteData>();
    NotePreviewDialog dialog(note, this);
    dialog.exec();
}


void MainWindow::onNoteDeleted(const QModelIndex& proxyIndex) {
    QModelIndex sourceIndex = note_proxy_model_->mapToSource(proxyIndex);
    NoteData data = note_model_->data(sourceIndex, NoteDataRole).value<NoteData>();
    NoteManager::instance()->removeNote(data.id);
    note_model_->refresh();

}


void MainWindow::onNoteEdited(const QModelIndex& proxyIndex) {
    QModelIndex sourceIndex = note_proxy_model_->mapToSource(proxyIndex);
    NoteData note = note_model_->data(sourceIndex, NoteDataRole).value<NoteData>();
    NoteEditDialog dialog(note, this);
    if (dialog.exec() == QDialog::Accepted) {
        NoteManager::instance()->updateNote(dialog.result());
        note_model_->refresh();
    }
}

void MainWindow::onNotePinToggled(const QModelIndex& proxyIndex) {
    QModelIndex sourceIndex = note_proxy_model_->mapToSource(proxyIndex);
    NoteData note = note_model_->data(sourceIndex, NoteDataRole).value<NoteData>();
    NoteManager::instance()->togglePin(note.id);
    note_model_->refresh();
}

void MainWindow::onNoteCategoryChanged(const QModelIndex& proxyIndex, const QString& category) {
    QModelIndex sourceIndex = note_proxy_model_->mapToSource(proxyIndex);
    NoteData note = note_model_->data(sourceIndex, NoteDataRole).value<NoteData>();
    NoteManager::instance()->updateNoteCategory(note.id, category);
    note_model_->refresh();
}

void MainWindow::onNoteColorChanged(const QModelIndex& proxyIndex, const QString& color) {
    QModelIndex sourceIndex = note_proxy_model_->mapToSource(proxyIndex);
    NoteData note = note_model_->data(sourceIndex, NoteDataRole).value<NoteData>();
    NoteManager::instance()->updateNoteColor(note.id, color);
    note_model_->refresh();
}

void MainWindow::onNotePreviewRequested(const QModelIndex& proxyIndex) {
    QModelIndex sourceIndex = note_proxy_model_->mapToSource(proxyIndex);
    NoteData note = note_model_->data(sourceIndex, NoteDataRole).value<NoteData>();
    NotePreviewDialog* dialog = new NotePreviewDialog(note, nullptr);  // parent=nullptr 使其独立
    dialog->setAttribute(Qt::WA_DeleteOnClose);  // 关闭时自动释放内存
    dialog->setWindowFlag(Qt::Window);           // 作为独立顶层窗口
    dialog->show();
}

void MainWindow::onNotesDeletedMultiple(const QModelIndexList& proxyIndexes) {
    QStringList ids;
    for (const QModelIndex& proxyIndex : proxyIndexes) {
        QModelIndex sourceIndex = note_proxy_model_->mapToSource(proxyIndex);
        NoteData note = note_model_->data(sourceIndex, NoteDataRole).value<NoteData>();
        ids.append(note.id);
    }
    NoteManager::instance()->removeNotes(ids);
    note_model_->refresh();
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
    saveWindowState();
    QWidget::closeEvent(event);
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





