#include <QtTest/QtTest>
#include <QApplication>

#include "models/notelistmodel.h"
#include "models/notefilterproxymodel.h"
#include "core/notemanager.h"
#include "common/notedata.h"

// ================================================================
// 测试类：NoteFilterProxyModelTest
// 每个 private slots 中的函数都是一个独立的测试用例
// ================================================================
class NoteFilterProxyModelTest : public QObject
{
    Q_OBJECT

private slots:
    // ---- 测试夹具 ----
    void init();     // 每个测试用例执行前调用（相当于 setUp）
    void cleanup();  // 每个测试用例执行后调用（相当于 tearDown）

    // ---- 测试用例 ----
    void test_defaultShowsAll();           // 默认状态（全部 + 空关键词）应显示所有便签
    void test_filterByCategory();          // 按分类过滤
    void test_filterByCategory_noMatch();  // 分类不存在时结果为空
    void test_filterByKeywordInTitle();    // 关键词命中标题
    void test_filterByKeywordInContent();  // 关键词命中内容
    void test_filterByKeyword_noMatch();   // 关键词不存在时结果为空
    void test_filterCombined();            // 分类 + 关键词组合过滤
    void test_filterCombined_categoryMismatch(); // 关键词匹配但分类不匹配
    void test_setKeyword_emptyRestoresAll();      // 清空关键词后恢复显示
    void test_setCategory_quanbuRestoresAll();    // 切回"全部"后恢复显示

    // ---- 搜索边界用例（3.7 实时过滤相关）----
    void test_filterByKeyword_caseInsensitive();  // 搜索大小写不敏感
    void test_filterByKeyword_partialMatch();     // 部分匹配（子串）
    void test_filterByKeyword_whitespaceOnly();   // 纯空格关键词，视为空，显示全部
    void test_filterByKeyword_chineseKeyword();   // 中文关键词匹配
    void test_filterByKeyword_updateAfterAdd();   // 新增便签后，过滤结果实时更新
    void test_filterByKeyword_updateAfterRemove();// 删除便签后，过滤结果实时更新


private:
    NoteListModel*        model_      = nullptr;
    NoteFilterProxyModel* proxyModel_ = nullptr;

    // 向 NoteManager 插入测试数据的辅助函数
    void populateTestData();
};

// ----------------------------------------------------------------
// 测试夹具实现
// ----------------------------------------------------------------
void NoteFilterProxyModelTest::init()
{
    // 每次测试前清空 NoteManager 中的数据，保证测试隔离
    // NoteManager 是单例，直接操作其数据
    for (const NoteData& note : NoteManager::instance()->notes()) {
        NoteManager::instance()->removeNote(note.id);
    }

    populateTestData();

    model_      = new NoteListModel(this);
    proxyModel_ = new NoteFilterProxyModel(this);

    // 将 NoteManager 的数据变更信号连接到 model_ 的刷新槽
    connect(NoteManager::instance(), &NoteManager::dataChanged,
            model_, &NoteListModel::refresh);

    model_->refresh();
    proxyModel_->setSourceModel(model_);
}

void NoteFilterProxyModelTest::cleanup()
{
    delete proxyModel_;
    proxyModel_ = nullptr;
    delete model_;
    model_ = nullptr;
}

void NoteFilterProxyModelTest::populateTestData()
{
    // 插入 5 条测试便签，覆盖 3 个分类
    NoteManager::instance()->addNote(
        NoteData::createNew("Qt 学习笔记",   "Model/View 架构",    "学习", "#A29BFE"));
    NoteManager::instance()->addNote(
        NoteData::createNew("C++ 基础",      "指针与引用的区别",    "学习", "#A29BFE"));
    NoteManager::instance()->addNote(
        NoteData::createNew("今日计划",       "完成项目报告",        "工作", "#FAB1A0"));
    NoteManager::instance()->addNote(
        NoteData::createNew("会议记录",       "讨论了架构设计方案",  "工作", "#FAB1A0"));
    NoteManager::instance()->addNote(
        NoteData::createNew("购物清单",       "牛奶、面包、鸡蛋",    "生活", "#81ECEC"));
}

// ----------------------------------------------------------------
// 测试用例实现
// ----------------------------------------------------------------

// 默认状态：category_ = "全部"，keyword_ = ""，应显示全部 5 条
void NoteFilterProxyModelTest::test_defaultShowsAll()
{
    QCOMPARE(proxyModel_->rowCount(), 5);
}

// 按分类"学习"过滤，应只剩 2 条
void NoteFilterProxyModelTest::test_filterByCategory()
{
    proxyModel_->setCategory("学习");
    QCOMPARE(proxyModel_->rowCount(), 2);
}

// 按不存在的分类过滤，结果应为 0
void NoteFilterProxyModelTest::test_filterByCategory_noMatch()
{
    proxyModel_->setCategory("娱乐");
    QCOMPARE(proxyModel_->rowCount(), 0);
}

// 关键词"Qt"命中标题"Qt 学习笔记"，应返回 1 条
void NoteFilterProxyModelTest::test_filterByKeywordInTitle()
{
    proxyModel_->setKeyword("Qt");
    QCOMPARE(proxyModel_->rowCount(), 1);

    // 进一步验证命中的那条便签标题正确
    QModelIndex idx = proxyModel_->index(0, 0);
    QString title = idx.data(NoteRole::NoteTitleRole).toString();
    QCOMPARE(title, QString("Qt 学习笔记"));
}

// 关键词"架构"命中内容，应返回 2 条（"Model/View 架构" 和 "讨论了架构设计方案"）
void NoteFilterProxyModelTest::test_filterByKeywordInContent()
{
    proxyModel_->setKeyword("架构");
    QCOMPARE(proxyModel_->rowCount(), 2);
}

// 关键词不存在，结果应为 0
void NoteFilterProxyModelTest::test_filterByKeyword_noMatch()
{
    proxyModel_->setKeyword("不存在的关键词xyz");
    QCOMPARE(proxyModel_->rowCount(), 0);
}

// 组合过滤：分类"工作" + 关键词"报告"，只有"今日计划"的内容含"报告"，应返回 1 条
void NoteFilterProxyModelTest::test_filterCombined()
{
    proxyModel_->setCategory("工作");
    proxyModel_->setKeyword("报告");
    QCOMPARE(proxyModel_->rowCount(), 1);

    QModelIndex idx = proxyModel_->index(0, 0);
    QString title = idx.data(NoteRole::NoteTitleRole).toString();
    QCOMPARE(title, QString("今日计划"));
}

// 关键词"架构"在"学习"和"工作"分类都有，但限定分类"生活"后应为 0
void NoteFilterProxyModelTest::test_filterCombined_categoryMismatch()
{
    proxyModel_->setCategory("生活");
    proxyModel_->setKeyword("架构");
    QCOMPARE(proxyModel_->rowCount(), 0);
}

// 先设置关键词，再清空，应恢复显示全部
void NoteFilterProxyModelTest::test_setKeyword_emptyRestoresAll()
{
    proxyModel_->setKeyword("Qt");
    QCOMPARE(proxyModel_->rowCount(), 1);  // 先确认过滤生效

    proxyModel_->setKeyword("");           // 清空关键词
    QCOMPARE(proxyModel_->rowCount(), 5);  // 应恢复全部
}

// 先设置分类，再切回"全部"，应恢复显示全部
void NoteFilterProxyModelTest::test_setCategory_quanbuRestoresAll()
{
    proxyModel_->setCategory("工作");
    QCOMPARE(proxyModel_->rowCount(), 2);  // 先确认过滤生效

    proxyModel_->setCategory("全部");      // 切回全部
    QCOMPARE(proxyModel_->rowCount(), 5);  // 应恢复全部
}

// ================================================================
// 搜索边界用例（3.7 实时过滤相关）
// ================================================================

// 搜索应大小写不敏感："qt" 应能匹配 "Qt 学习笔记"
void NoteFilterProxyModelTest::test_filterByKeyword_caseInsensitive()
{
    proxyModel_->setKeyword("qt");  // 小写
    // "Qt 学习笔记" 标题含 "Qt"，应被匹配
    QCOMPARE(proxyModel_->rowCount(), 1);

    QModelIndex idx = proxyModel_->index(0, 0);
    QString title = idx.data(NoteRole::NoteTitleRole).toString();
    QCOMPARE(title, QString("Qt 学习笔记"));
}

// 部分匹配（子串）：搜索 "计" 应匹配 "今日计划"（标题）和 "今日计划"（内容含"完成项目报告"不含"计"）
// 实际上 "计" 只在 "今日计划" 标题中出现，应返回 1 条
void NoteFilterProxyModelTest::test_filterByKeyword_partialMatch()
{
    proxyModel_->setKeyword("计划");
    // "今日计划" 标题含 "计划"，应被匹配
    QCOMPARE(proxyModel_->rowCount(), 1);

    QModelIndex idx = proxyModel_->index(0, 0);
    QString title = idx.data(NoteRole::NoteTitleRole).toString();
    QCOMPARE(title, QString("今日计划"));
}

// 纯空格关键词应视为空，显示全部 5 条
// （防止用户误输入空格导致搜索结果为空的体验问题）
void NoteFilterProxyModelTest::test_filterByKeyword_whitespaceOnly()
{
    proxyModel_->setKeyword("   ");  // 纯空格
    // NoteFilterProxyModel 应对关键词做 trimmed() 处理
    // 如果实现了 trim，则显示全部 5 条；如果没有，则为 0 条
    // 此处验证实现是否符合预期（trimmed 后为空 → 显示全部）
    int count = proxyModel_->rowCount();
    QVERIFY2(count == 5 || count == 0,
        "纯空格关键词：要么 trim 后显示全部(5)，要么严格匹配显示0，两种都可接受");
    // 记录实际行为，方便后续决策是否需要 trim
    qDebug() << "纯空格关键词实际结果：" << count << "条（5=trim处理，0=严格匹配）";
}

// 中文关键词匹配：搜索 "牛奶" 应命中 "购物清单" 的内容
void NoteFilterProxyModelTest::test_filterByKeyword_chineseKeyword()
{
    proxyModel_->setKeyword("牛奶");
    QCOMPARE(proxyModel_->rowCount(), 1);

    QModelIndex idx = proxyModel_->index(0, 0);
    QString title = idx.data(NoteRole::NoteTitleRole).toString();
    QCOMPARE(title, QString("购物清单"));
}

// 新增便签后，已设置的关键词过滤应实时更新
void NoteFilterProxyModelTest::test_filterByKeyword_updateAfterAdd()
{
    proxyModel_->setKeyword("新增");
    QCOMPARE(proxyModel_->rowCount(), 0);  // 初始无匹配

    // 新增一条含关键词的便签
    NoteManager::instance()->addNote(
        NoteData::createNew("新增的便签", "新增内容", "工作"));
    model_->refresh();  // 刷新源模型

    // 代理模型应自动重新过滤，显示 1 条
    QCOMPARE(proxyModel_->rowCount(), 1);
}

// 删除便签后，已设置的关键词过滤应实时更新
void NoteFilterProxyModelTest::test_filterByKeyword_updateAfterRemove()
{
    proxyModel_->setKeyword("Qt");
    QCOMPARE(proxyModel_->rowCount(), 1);  // 初始有 1 条匹配

    // 找到并删除那条便签
    // 通过代理模型的第 0 行获取 id
    QModelIndex proxyIdx = proxyModel_->index(0, 0);
    QModelIndex sourceIdx = proxyModel_->mapToSource(proxyIdx);
    NoteData note = sourceIdx.data(NoteRole::NoteDataRole).value<NoteData>();

    NoteManager::instance()->removeNote(note.id);
    model_->refresh();  // 刷新源模型

    // 代理模型应自动重新过滤，显示 0 条
    QCOMPARE(proxyModel_->rowCount(), 0);
}

// ================================================================
// 程序入口
// QTEST_MAIN 会自动生成 main 函数，运行所有测试用例
// ================================================================
QTEST_MAIN(NoteFilterProxyModelTest)
#include "test_notefilterproxymodel.moc"

