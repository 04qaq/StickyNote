#include "deletenotecommand.h"
#include "../notemanager.h"

DeleteNoteCommand::DeleteNoteCommand(const NoteData& note)
    : note_(note)
{
}

void DeleteNoteCommand::execute() {
    NoteManager::instance()->removeNote(note_.id);
}

void DeleteNoteCommand::undo() {
    NoteManager::instance()->addNote(note_);
}
