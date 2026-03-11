#include "addnotecommand.h"
#include "../notemanager.h"

AddNoteCommand::AddNoteCommand(const NoteData& note)
    : note_(note)
{
}

void AddNoteCommand::execute() {
    NoteManager::instance()->addNote(note_);
}

void AddNoteCommand::undo() {
    NoteManager::instance()->removeNote(note_.id);
}
