#pragma once
#include <QStringList>
#include <QDir>


// 便签背景色色板
inline const QStringList NOTE_COLORS = {
    "#FFEAA7",  // 黄
    "#81ECEC",  // 青
    "#DFE6E9",  // 灰
    "#FAB1A0",  // 红
    "#A29BFE"   // 紫
};

// 内置分类（不可删除）
inline const QStringList BUILTIN_CATEGORIES = {
    "全部", "工作", "生活", "学习"
};

// 数据文件路径
inline QString notesFilePath() {
    return QDir::homePath() + "/.stickynote/notes.json";
}

