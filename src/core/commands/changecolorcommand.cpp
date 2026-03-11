#include "changecolorcommand.h"
#include "../notemanager.h"

ChangeColorCommand::ChangeColorCommand(const QString& note_id,
                                       const QString& old_color,
                                       const QString& new_color)
    : note_id_(note_id)
    , old_color_(old_color)
    , new_color_(new_color)
{
}

void ChangeColorCommand::execute() {
    NoteManager::instance()->updateNoteColor(note_id_, new_color_);
}

void ChangeColorCommand::undo() {
    NoteManager::instance()->updateNoteColor(note_id_, old_color_);
}
