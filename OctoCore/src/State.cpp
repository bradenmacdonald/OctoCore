#include "State.h"

using namespace Octo;

State::State(SessionId sessionId) :
    m_session_id(sessionId),
    m_next_object_id(((int64_t)sessionId << 48) | 1) // see getNextObjectId()
{
    if (m_session_id & 4<<14) {
        throw StateException("Invalid session ID. Session ID must be 14 bytes or smaller.");
    }
}

State::~State() {
}

ObjectId State::getNextObjectId() {
    // Atomically fetch and increment the value of m_next_object_id.
    // Object ID format is: 2bits that are always zero, 14 bits session ID, 48 bits incremented IDs
    const ObjectId object_id = m_next_object_id++;
    if ((object_id >> 48) != m_session_id) {
        // We've reached the limit of the number of object IDs supported in a single session.
        throw StateException("Reached limit of available object IDs for this session.");
    }
    return object_id;
}

CommandRegistry* State::_getCommandRegistry() const {
    throw StateException("_getCommandRegistry not implemented. Add OCTO_STATE_DEFAULTS to use commands.");
}

std::shared_ptr<const Map> State::_runCommand(const CommandBase& command, bool allowUndo) {
    auto wrapped_command = _getCommandRegistry()->getCommand(command.commandId());
    if (wrapped_command == nullptr) {
        throw InapplicableCommandException();
    }
    auto result = std::make_shared<Map>();
    wrapped_command->forward(this, command.args(), result, true);
    if (allowUndo) {
        CommandRecord r{ command.commandId(), command.args(), result };
        m_undo.push_back(std::move(r));
        if (not m_redo.empty()) {
            m_redo.clear();
        }
    }
    return result;
}
void State::undo() {
    if (canUndo()) {
        CommandRecord r = std::move(m_undo.back());
        m_undo.pop_back();
        auto wrapped_command = _getCommandRegistry()->getCommand(r.command_id);
        wrapped_command->backward(this, r.args, r.result);
        m_redo.push_back(std::move(r));
    }
}
void State::redo() {
    if (canRedo()) {
        CommandRecord r = std::move(m_redo.back());
        m_redo.pop_back();
        auto wrapped_command = _getCommandRegistry()->getCommand(r.command_id);
        // Unfortunately we need to do a const_cast<> here. But guarantees are in place
        // that 'result' won't be modified since we're passing mutable_result = false
        std::shared_ptr<Map> mutable_result = std::const_pointer_cast<Map>(r.result);
        wrapped_command->forward(this, r.args, mutable_result, false);
        m_undo.push_back(std::move(r));
    }
}
