#include "pinnotecommand.h"
#include "../notemanager.h"

PinNoteCommand::PinNoteCommand(const QString& note_id, bool old_pinned)
    : note_id_(note_id)
    , old_pinned_(old_pinned)
{
}

void PinNoteCommand::execute() {
    // togglePin 会将当前状态取反
    NoteManager::instance()->togglePin(note_id_);
}

void PinNoteCommand::undo() {
    // 再次 toggle 恢复原状态
    NoteManager::instance()->togglePin(note_id_);
}
