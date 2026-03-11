#include "changecategorycommand.h"
#include "../notemanager.h"

ChangeCategoryCommand::ChangeCategoryCommand(const QString& note_id,
                                             const QString& old_category,
                                             const QString& new_category)
    : note_id_(note_id)
    , old_category_(old_category)
    , new_category_(new_category)
{
}

void ChangeCategoryCommand::execute() {
    NoteManager::instance()->updateNoteCategory(note_id_, new_category_);
}

void ChangeCategoryCommand::undo() {
    NoteManager::instance()->updateNoteCategory(note_id_, old_category_);
}
