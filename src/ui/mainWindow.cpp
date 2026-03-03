#include "mainwindow.h"
#include "titlebar.h"
#include "sidebar.h"
#include "../utils/windowhelper.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTextEdit>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QCloseEvent>
#include <QEvent>
#include <QSettings>
#include <QScreen>
#include <QLabel>

MainWindow::MainWindow(QWidget* parent)
    : QWidget(parent)
{
    // 去掉系统边框，保留任务栏图标
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowMinimizeButtonHint);
    // 背景透明，用于实现圆角和阴影
    setAttribute(Qt::WA_TranslucentBackground);

    // 默认尺寸 900×600（含阴影边距）
    setMinimumSize(700 + SHADOW_MARGIN * 2, 450 + SHADOW_MARGIN * 2);
    resize(900 + SHADOW_MARGIN * 2, 600 + SHADOW_MARGIN * 2);

    initUI();
    initConnections();
    applyStyle();

    // 安装边缘缩放辅助
    m_winHelper = new WindowHelper(this, this);

    // 恢复上次窗口状态
    restoreWindowState();
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

    // 内容容器（圆角背景绘制在此 widget 上）
    QWidget* contentWidget = new QWidget(this);
    contentWidget->setObjectName("contentWidget");
    outerLayout->addWidget(contentWidget);

    // 内容容器主布局（垂直：标题栏 + 主体区域）
    QVBoxLayout* contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);

    // 标题栏
    m_titleBar = new TitleBar(contentWidget);
    contentLayout->addWidget(m_titleBar);

    // 主体区域（水平：侧边栏 + 内容区）
    QWidget* bodyWidget = new QWidget(contentWidget);
    bodyWidget->setObjectName("bodyWidget");
    QHBoxLayout* bodyLayout = new QHBoxLayout(bodyWidget);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(0);

    // 侧边栏
    m_sidebar = new Sidebar(bodyWidget);
    bodyLayout->addWidget(m_sidebar);

    // 内容区（一期占位：文本编辑框，二期替换为便签列表）
    m_textEdit = new QTextEdit(bodyWidget);
    m_textEdit->setObjectName("textEdit");
    m_textEdit->setPlaceholderText("便签内容区域（二期实现便签列表）...");
    m_textEdit->setFrameShape(QFrame::NoFrame);
    bodyLayout->addWidget(m_textEdit, 1);

    contentLayout->addWidget(bodyWidget, 1);
}

void MainWindow::initConnections()
{
    connect(m_titleBar, &TitleBar::minimizeRequested, this, &MainWindow::showMinimized);
    connect(m_titleBar, &TitleBar::closeRequested,    this, &MainWindow::close);
    connect(m_titleBar, &TitleBar::maximizeRequested, this, [this]() {
        if (isMaximized())
            showNormal();
        else
            showMaximized();
    });
}

void MainWindow::applyStyle()
{
    setStyleSheet(R"(
        QWidget#contentWidget {
            background-color: #F5F6FA;
            border-radius: 8px;
        }

        QWidget#bodyWidget {
            background-color: #F5F6FA;
            border-bottom-left-radius: 8px;
            border-bottom-right-radius: 8px;
        }

        QTextEdit#textEdit {
            background-color: #ffffff;
            border: none;
            padding: 12px;
            font-size: 13px;
            color: #333;
            border-bottom-right-radius: 8px;
        }
    )");
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
        // 首次启动：居中显示
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

    // 最大化时不绘制阴影和圆角
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
        bool maximized = isMaximized();
        m_titleBar->updateMaxButton(maximized);

        // 最大化时去掉阴影边距
        QVBoxLayout* outerLayout = qobject_cast<QVBoxLayout*>(layout());
        if (outerLayout) {
            int margin = maximized ? 0 : SHADOW_MARGIN;
            outerLayout->setContentsMargins(margin, margin, margin, margin);
        }
        update();
    }
    QWidget::changeEvent(event);
}