#include "editnotecommand.h"
#include "../notemanager.h"

EditNoteCommand::EditNoteCommand(const NoteData& old_data, const NoteData& new_data)
    : old_data_(old_data)
    , new_data_(new_data)
{
}

void EditNoteCommand::execute() {
    NoteManager::instance()->updateNote(new_data_);
}

void EditNoteCommand::undo() {
    NoteManager::instance()->updateNote(old_data_);
}
