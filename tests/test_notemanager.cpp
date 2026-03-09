#include <QtTest/QtTest>
#include <QApplication>

#include "core/notemanager.h"
#include "common/notedata.h"

// ================================================================
// 测试类：NoteManagerTest
// 测试便签的新增、修改、删除功能
// ================================================================
class NoteManagerTest : public QObject
{
    Q_OBJECT

private slots:
    // ---- 测试夹具 ----
    void init();     // 每个测试用例执行前调用，清空数据
    void cleanup();  // 每个测试用例执行后调用，清空数据

    // ---- 新增测试 ----
    void test_addNote_increasesCount();    // 新增后数量 +1
    void test_addNote_contentIsCorrect();  // 新增后内容正确
    void test_addNote_idIsUnique();        // 两条便签 id 不重复

    // ---- 修改测试 ----
    void test_updateNote_titleChanged();    // 修改标题后生效
    void test_updateNote_contentChanged();  // 修改内容后生效
    void test_updateNote_categoryChanged(); // 修改分类后生效
    void test_updateNote_countUnchanged();  // 修改后数量不变
    void test_updateNote_nonExistId();      // 修改不存在的 id，不崩溃

    // ---- 删除测试 ----
    void test_removeNote_decreasesCount();     // 删除后数量 -1
    void test_removeNote_cannotFindById();     // 删除后 findById 返回 nullptr
    void test_removeNote_nonExistId();         // 删除不存在的 id，不崩溃
    void test_removeNote_correctItemRemoved(); // 只删指定项，其他便签完好

    // ---- 批量删除测试（3.5 新增）----
    void test_removeNotes_allDeleted();          // 批量删除后全部消失
    void test_removeNotes_countDecreases();      // 批量删除后数量正确减少
    void test_removeNotes_othersUnaffected();    // 批量删除只删指定项，其余完好
    void test_removeNotes_emptyList();           // 传入空列表，不崩溃，数量不变
    void test_removeNotes_partialInvalidIds();   // 列表中含不存在的 id，不崩溃，有效项被删

    // ---- 置顶切换测试（3.8 新增）----
    void test_togglePin_falseToTrue();           // 未置顶 → 置顶
    void test_togglePin_trueToFalse();           // 已置顶 → 取消置顶
    void test_togglePin_twiceRestoresOriginal(); // 连续切换两次，回到原始状态
    void test_togglePin_nonExistId();            // 不存在的 id，不崩溃

    // ---- 更改分类测试（3.8 新增）----
    void test_updateNoteCategory_changed();      // 分类成功更改
    void test_updateNoteCategory_countUnchanged(); // 更改分类后数量不变
    void test_updateNoteCategory_nonExistId();   // 不存在的 id，不崩溃

    // ---- 更改颜色测试（3.8 新增）----
    void test_updateNoteColor_changed();         // 颜色成功更改
    void test_updateNoteColor_countUnchanged();  // 更改颜色后数量不变
    void test_updateNoteColor_nonExistId();      // 不存在的 id，不崩溃
};


// ----------------------------------------------------------------
// 测试夹具：每个测试用例执行前清空 NoteManager，保证测试隔离
// ----------------------------------------------------------------
void NoteManagerTest::init()
{
    // NoteManager 是单例，直接清空其数据
    const QList<NoteData> notes = NoteManager::instance()->notes();
    for (const NoteData& note : notes) {
        NoteManager::instance()->removeNote(note.id);
    }
    QCOMPARE(NoteManager::instance()->notes().size(), 0);
}

void NoteManagerTest::cleanup()
{
    const QList<NoteData> notes = NoteManager::instance()->notes();
    for (const NoteData& note : notes) {
        NoteManager::instance()->removeNote(note.id);
    }
}

// ================================================================
// 新增测试
// ================================================================

// 新增一条便签后，总数应从 0 变为 1
void NoteManagerTest::test_addNote_increasesCount()
{
    QCOMPARE(NoteManager::instance()->notes().size(), 0);

    NoteManager::instance()->addNote(
        NoteData::createNew("测试标题", "测试内容", "工作", "#FFEAA7"));

    QCOMPARE(NoteManager::instance()->notes().size(), 1);
}

// 新增后，通过 findById 能取回完整数据，且内容与写入一致
void NoteManagerTest::test_addNote_contentIsCorrect()
{
    NoteData note = NoteData::createNew("Qt 学习", "信号与槽机制", "学习", "#A29BFE");
    NoteManager::instance()->addNote(note);

    NoteData* found = NoteManager::instance()->findById(note.id);

    QVERIFY(found != nullptr);
    QCOMPARE(found->title,    QString("Qt 学习"));
    QCOMPARE(found->content,  QString("信号与槽机制"));
    QCOMPARE(found->category, QString("学习"));
    QCOMPARE(found->color,    QString("#A29BFE"));
}

// 连续新增两条便签，它们的 id 不能相同
void NoteManagerTest::test_addNote_idIsUnique()
{
    NoteData note1 = NoteData::createNew("便签1", "", "工作");
    NoteData note2 = NoteData::createNew("便签2", "", "工作");

    NoteManager::instance()->addNote(note1);
    NoteManager::instance()->addNote(note2);

    QVERIFY(note1.id != note2.id);
}

// ================================================================
// 修改测试
// ================================================================

// 修改标题后，findById 返回的数据标题应已更新
void NoteManagerTest::test_updateNote_titleChanged()
{
    NoteData note = NoteData::createNew("旧标题", "内容", "工作");
    NoteManager::instance()->addNote(note);

    note.title = "新标题";
    NoteManager::instance()->updateNote(note);

    NoteData* found = NoteManager::instance()->findById(note.id);
    QVERIFY(found != nullptr);
    QCOMPARE(found->title, QString("新标题"));
}

// 修改内容后，findById 返回的数据内容应已更新
void NoteManagerTest::test_updateNote_contentChanged()
{
    NoteData note = NoteData::createNew("标题", "旧内容", "生活");
    NoteManager::instance()->addNote(note);

    note.content = "新内容";
    NoteManager::instance()->updateNote(note);

    NoteData* found = NoteManager::instance()->findById(note.id);
    QVERIFY(found != nullptr);
    QCOMPARE(found->content, QString("新内容"));
}

// 修改分类后，findById 返回的数据分类应已更新
void NoteManagerTest::test_updateNote_categoryChanged()
{
    NoteData note = NoteData::createNew("标题", "内容", "工作");
    NoteManager::instance()->addNote(note);

    note.category = "学习";
    NoteManager::instance()->updateNote(note);

    NoteData* found = NoteManager::instance()->findById(note.id);
    QVERIFY(found != nullptr);
    QCOMPARE(found->category, QString("学习"));
}

// 修改便签后，总数量不应发生变化（不能变成 2 条）
void NoteManagerTest::test_updateNote_countUnchanged()
{
    NoteData note = NoteData::createNew("标题", "内容", "工作");
    NoteManager::instance()->addNote(note);
    QCOMPARE(NoteManager::instance()->notes().size(), 1);

    note.title = "修改后的标题";
    NoteManager::instance()->updateNote(note);

    QCOMPARE(NoteManager::instance()->notes().size(), 1);
}

// 用不存在的 id 调用 updateNote，不应崩溃，数量不变
void NoteManagerTest::test_updateNote_nonExistId()
{
    NoteData note = NoteData::createNew("标题", "内容", "工作");
    NoteManager::instance()->addNote(note);

    NoteData fakeNote = NoteData::createNew("假便签", "", "工作");
    fakeNote.id = "non-exist-id-12345";
    NoteManager::instance()->updateNote(fakeNote);  // 不应崩溃

    QCOMPARE(NoteManager::instance()->notes().size(), 1);
}

// ================================================================
// 删除测试
// ================================================================

// 删除一条便签后，总数应从 1 变为 0
void NoteManagerTest::test_removeNote_decreasesCount()
{
    NoteData note = NoteData::createNew("待删除", "内容", "工作");
    NoteManager::instance()->addNote(note);
    QCOMPARE(NoteManager::instance()->notes().size(), 1);

    NoteManager::instance()->removeNote(note.id);

    QCOMPARE(NoteManager::instance()->notes().size(), 0);
}

// 删除后，findById 应返回 nullptr
void NoteManagerTest::test_removeNote_cannotFindById()
{
    NoteData note = NoteData::createNew("待删除", "内容", "工作");
    NoteManager::instance()->addNote(note);

    NoteManager::instance()->removeNote(note.id);

    QVERIFY(NoteManager::instance()->findById(note.id) == nullptr);
}

// 用不存在的 id 调用 removeNote，不应崩溃，数量不变
void NoteManagerTest::test_removeNote_nonExistId()
{
    NoteData note = NoteData::createNew("标题", "内容", "工作");
    NoteManager::instance()->addNote(note);

    NoteManager::instance()->removeNote("non-exist-id-99999");  // 不应崩溃

    QCOMPARE(NoteManager::instance()->notes().size(), 1);
}

// 有两条便签时，删除其中一条，另一条应完好无损
void NoteManagerTest::test_removeNote_correctItemRemoved()
{
    NoteData note1 = NoteData::createNew("便签A", "内容A", "工作");
    NoteData note2 = NoteData::createNew("便签B", "内容B", "学习");
    NoteManager::instance()->addNote(note1);
    NoteManager::instance()->addNote(note2);

    NoteManager::instance()->removeNote(note1.id);

    // note1 应已消失
    QVERIFY(NoteManager::instance()->findById(note1.id) == nullptr);

    // note2 应仍然存在，且内容完好
    NoteData* found = NoteManager::instance()->findById(note2.id);
    QVERIFY(found != nullptr);
    QCOMPARE(found->title,    QString("便签B"));
    QCOMPARE(found->content,  QString("内容B"));
    QCOMPARE(found->category, QString("学习"));
}

// ================================================================
// 批量删除测试（3.5 新增：removeNotes）
// ================================================================

// 批量删除 3 条便签后，全部应消失
void NoteManagerTest::test_removeNotes_allDeleted()
{
    NoteData n1 = NoteData::createNew("便签1", "", "工作");
    NoteData n2 = NoteData::createNew("便签2", "", "工作");
    NoteData n3 = NoteData::createNew("便签3", "", "工作");
    NoteManager::instance()->addNote(n1);
    NoteManager::instance()->addNote(n2);
    NoteManager::instance()->addNote(n3);

    NoteManager::instance()->removeNotes({n1.id, n2.id, n3.id});

    QCOMPARE(NoteManager::instance()->notes().size(), 0);
    QVERIFY(NoteManager::instance()->findById(n1.id) == nullptr);
    QVERIFY(NoteManager::instance()->findById(n2.id) == nullptr);
    QVERIFY(NoteManager::instance()->findById(n3.id) == nullptr);
}

// 有 5 条便签，批量删除其中 3 条，剩余应为 2 条
void NoteManagerTest::test_removeNotes_countDecreases()
{
    NoteData n1 = NoteData::createNew("便签1", "", "工作");
    NoteData n2 = NoteData::createNew("便签2", "", "工作");
    NoteData n3 = NoteData::createNew("便签3", "", "工作");
    NoteData n4 = NoteData::createNew("便签4", "", "学习");
    NoteData n5 = NoteData::createNew("便签5", "", "学习");
    NoteManager::instance()->addNote(n1);
    NoteManager::instance()->addNote(n2);
    NoteManager::instance()->addNote(n3);
    NoteManager::instance()->addNote(n4);
    NoteManager::instance()->addNote(n5);

    // 删除前 3 条
    NoteManager::instance()->removeNotes({n1.id, n2.id, n3.id});

    QCOMPARE(NoteManager::instance()->notes().size(), 2);
}

// 批量删除只删指定项，其余便签内容完好
void NoteManagerTest::test_removeNotes_othersUnaffected()
{
    NoteData keep1 = NoteData::createNew("保留A", "内容A", "学习");
    NoteData keep2 = NoteData::createNew("保留B", "内容B", "生活");
    NoteData del1  = NoteData::createNew("删除X", "内容X", "工作");
    NoteData del2  = NoteData::createNew("删除Y", "内容Y", "工作");
    NoteManager::instance()->addNote(keep1);
    NoteManager::instance()->addNote(keep2);
    NoteManager::instance()->addNote(del1);
    NoteManager::instance()->addNote(del2);

    NoteManager::instance()->removeNotes({del1.id, del2.id});

    // 被删的应消失
    QVERIFY(NoteManager::instance()->findById(del1.id) == nullptr);
    QVERIFY(NoteManager::instance()->findById(del2.id) == nullptr);

    // 保留的应完好
    NoteData* fa = NoteManager::instance()->findById(keep1.id);
    NoteData* fb = NoteManager::instance()->findById(keep2.id);
    QVERIFY(fa != nullptr);
    QVERIFY(fb != nullptr);
    QCOMPARE(fa->title,   QString("保留A"));
    QCOMPARE(fb->content, QString("内容B"));
}

// 传入空列表，不应崩溃，数量不变
void NoteManagerTest::test_removeNotes_emptyList()
{
    NoteData note = NoteData::createNew("便签", "", "工作");
    NoteManager::instance()->addNote(note);

    NoteManager::instance()->removeNotes({});  // 空列表，不应崩溃

    QCOMPARE(NoteManager::instance()->notes().size(), 1);
}

// 列表中含不存在的 id，不应崩溃，有效项被正确删除
void NoteManagerTest::test_removeNotes_partialInvalidIds()
{
    NoteData note = NoteData::createNew("真实便签", "", "工作");
    NoteManager::instance()->addNote(note);

    // 一个真实 id + 一个不存在的 id
    NoteManager::instance()->removeNotes({note.id, "fake-id-000"});

    // 真实便签应被删除
    QVERIFY(NoteManager::instance()->findById(note.id) == nullptr);
    QCOMPARE(NoteManager::instance()->notes().size(), 0);
}

// ================================================================
// 置顶切换测试（3.8 新增：togglePin）
// ================================================================

// 初始 pinned = false，调用 togglePin 后应变为 true
void NoteManagerTest::test_togglePin_falseToTrue()
{
    NoteData note = NoteData::createNew("便签", "", "工作");
    note.pinned = false;
    NoteManager::instance()->addNote(note);

    NoteManager::instance()->togglePin(note.id);

    NoteData* found = NoteManager::instance()->findById(note.id);
    QVERIFY(found != nullptr);
    QCOMPARE(found->pinned, true);
}

// 初始 pinned = true，调用 togglePin 后应变为 false
void NoteManagerTest::test_togglePin_trueToFalse()
{
    NoteData note = NoteData::createNew("便签", "", "工作");
    note.pinned = true;
    NoteManager::instance()->addNote(note);

    NoteManager::instance()->togglePin(note.id);

    NoteData* found = NoteManager::instance()->findById(note.id);
    QVERIFY(found != nullptr);
    QCOMPARE(found->pinned, false);
}

// 连续调用 togglePin 两次，应回到原始状态（幂等性验证）
void NoteManagerTest::test_togglePin_twiceRestoresOriginal()
{
    NoteData note = NoteData::createNew("便签", "", "工作");
    note.pinned = false;
    NoteManager::instance()->addNote(note);

    NoteManager::instance()->togglePin(note.id);  // false → true
    NoteManager::instance()->togglePin(note.id);  // true  → false

    NoteData* found = NoteManager::instance()->findById(note.id);
    QVERIFY(found != nullptr);
    QCOMPARE(found->pinned, false);  // 回到原始状态
}

// 对不存在的 id 调用 togglePin，不应崩溃，数量不变
void NoteManagerTest::test_togglePin_nonExistId()
{
    NoteData note = NoteData::createNew("便签", "", "工作");
    NoteManager::instance()->addNote(note);

    NoteManager::instance()->togglePin("non-exist-id-pin");  // 不应崩溃

    QCOMPARE(NoteManager::instance()->notes().size(), 1);
}

// ================================================================
// 更改分类测试（3.8 新增：updateNoteCategory）
// ================================================================

// 调用 updateNoteCategory 后，分类应成功更改
void NoteManagerTest::test_updateNoteCategory_changed()
{
    NoteData note = NoteData::createNew("便签", "", "工作");
    NoteManager::instance()->addNote(note);

    NoteManager::instance()->updateNoteCategory(note.id, "学习");

    NoteData* found = NoteManager::instance()->findById(note.id);
    QVERIFY(found != nullptr);
    QCOMPARE(found->category, QString("学习"));
}

// 更改分类后，总数量不应变化
void NoteManagerTest::test_updateNoteCategory_countUnchanged()
{
    NoteData note = NoteData::createNew("便签", "", "工作");
    NoteManager::instance()->addNote(note);

    NoteManager::instance()->updateNoteCategory(note.id, "生活");

    QCOMPARE(NoteManager::instance()->notes().size(), 1);
}

// 对不存在的 id 调用 updateNoteCategory，不应崩溃
void NoteManagerTest::test_updateNoteCategory_nonExistId()
{
    NoteData note = NoteData::createNew("便签", "", "工作");
    NoteManager::instance()->addNote(note);

    NoteManager::instance()->updateNoteCategory("non-exist-id-cat", "学习");  // 不应崩溃

    QCOMPARE(NoteManager::instance()->notes().size(), 1);
}

// ================================================================
// 更改颜色测试（3.8 新增：updateNoteColor）
// ================================================================

// 调用 updateNoteColor 后，颜色应成功更改
void NoteManagerTest::test_updateNoteColor_changed()
{
    NoteData note = NoteData::createNew("便签", "", "工作", "#FFEAA7");
    NoteManager::instance()->addNote(note);

    NoteManager::instance()->updateNoteColor(note.id, "#A29BFE");

    NoteData* found = NoteManager::instance()->findById(note.id);
    QVERIFY(found != nullptr);
    QCOMPARE(found->color, QString("#A29BFE"));
}

// 更改颜色后，总数量不应变化
void NoteManagerTest::test_updateNoteColor_countUnchanged()
{
    NoteData note = NoteData::createNew("便签", "", "工作", "#FFEAA7");
    NoteManager::instance()->addNote(note);

    NoteManager::instance()->updateNoteColor(note.id, "#81ECEC");

    QCOMPARE(NoteManager::instance()->notes().size(), 1);
}

// 对不存在的 id 调用 updateNoteColor，不应崩溃
void NoteManagerTest::test_updateNoteColor_nonExistId()
{
    NoteData note = NoteData::createNew("便签", "", "工作", "#FFEAA7");
    NoteManager::instance()->addNote(note);

    NoteManager::instance()->updateNoteColor("non-exist-id-color", "#A29BFE");  // 不应崩溃

    QCOMPARE(NoteManager::instance()->notes().size(), 1);
}

// ================================================================
// 程序入口
// ================================================================
QTEST_MAIN(NoteManagerTest)
#include "test_notemanager.moc"

