#include "deletemultiplenotescommand.h"
#include "../notemanager.h"

DeleteMultipleNotesCommand::DeleteMultipleNotesCommand(const QList<NoteData>& notes)
    : notes_(notes)
{
}

void DeleteMultipleNotesCommand::execute() {
    QStringList ids;
    for (const NoteData& note : notes_) {
        ids.append(note.id);
    }
    NoteManager::instance()->removeNotes(ids);
}

void DeleteMultipleNotesCommand::undo() {
    // 逐条恢复（按原顺序）
    for (const NoteData& note : notes_) {
        NoteManager::instance()->addNote(note);
    }
}
