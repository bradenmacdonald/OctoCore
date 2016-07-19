#pragma once
#include <atomic>
#include <deque>
#include <string>
#include <vector>
#include "Command.h"
#include "Exception.h"

namespace Octo {

class State {
public:
    using SessionId = uint16_t;
    using ObjectId = int64_t;

    /** Destructor for this state manager. */
    virtual ~State();
    
    enum class InternalKey; // Keys to the internal session-specific key-value store table. Internal use only.

    SessionId sessionId() const { return m_session_id; }
    
    /** getNextObjectId: Get a unique ID for a new object.
     *
     *  The returned ID is guaranteed not to conflict with object IDs created in other sessions
     *  (because the object ID includes the sessionId).
     *
     *  Object IDs are never re-used. Each sesion will be limited to approximately 10^14 IDs
     *  over all time.
     */
    ObjectId getNextObjectId();

    /** Run a command, and optionally add it to the undo queue. */
    template<class CommandType>
    typename CommandType::Result runCommand(const CommandType& command, bool allowUndo = true) {
        return typename CommandType::Result {_runCommand(command, allowUndo)};
    }
    /** Is there a command in the undo queue that we can undo? */
    bool canUndo() const { return (not m_undo.empty()); }
    /** Undo the last command */
    void undo();
    /** Is there a command in the redo queue that we can replay? */
    bool canRedo() const { return (not m_redo.empty()); }
    /** Redo the last command */
    void redo();

protected:
    /** Construct a state manager.
     * It will either keep its data in memory or load/save it to the given file path.
     *
     * sessionId: This must be a unique 16-bit integer to represent this session/client.
     *     For instance, in a web application, each web page concurrently working on the
     *     same document *must* have a different sessionId assigned by the server.
     *     In the case of an installed application or mobile app, sessionId can represent
     *     that particular app and be re-used.
     */
    State(SessionId sessionId);

    /** Get the command registry used for this state.
     *  This method should return the same thing as the static method YourSubclass::getCommandRegistry()
     *  If you include OCTO_STATE_DEFAULTS in your class declaration this is implemented for you.
     */
    virtual CommandRegistry* _getCommandRegistry() const;
    
private:
    /** Run a command, and optionally add it to the undo queue. */
    std::shared_ptr<const Map> _runCommand(const CommandBase& command, bool allowUndo);

    // Data:
protected:
    const SessionId m_session_id;

private:
    struct CommandRecord {
        const int32_t command_id;
        const std::shared_ptr<const Map> args;
        const std::shared_ptr<const Map> result;

        CommandRecord(int32_t commandId, std::shared_ptr<const Map> args, std::shared_ptr<const Map> result):
            command_id(commandId), args(args), result(result) {}
        CommandRecord() : command_id(0), args(nullptr), result(nullptr) {}
        // Disable copying but allow moves:
        CommandRecord(const CommandRecord&) = delete;
        CommandRecord& operator=(const CommandRecord&) = delete;
        CommandRecord(CommandRecord&&) = default;
        CommandRecord& operator=(CommandRecord&&) = default;
    };
    std::deque<CommandRecord> m_undo;
    std::deque<CommandRecord> m_redo;
    #ifdef EMSCRIPTEN
    ObjectId m_next_object_id; // 64-bit atomics don't work properly in Emscripten :-/
    # else
    std::atomic<ObjectId> m_next_object_id; // Next object ID (threadsafe). Does not include the session identifier part
    #endif
};

} // namespace Octo


/** Helper macro - put this inside the class declaration for most states, unless you
 *  want to be able to have commands that work on multiple states. */
#define OCTO_STATE_DEFAULTS \
    public: \
    static Octo::CommandRegistry* getCommandRegistry() { \
        static Octo::CommandRegistry registry; \
        return &registry; \
    } \
    virtual Octo::CommandRegistry* _getCommandRegistry() const override { \
        return std::remove_reference<decltype(*this)>::type::getCommandRegistry(); \
    }


/** Helper macro - put this inside the class declaration for states that want to share
 *  commands. See State_test.cpp for an example. */
#define OCTO_STATE_COMMAND_REGISTRY(getter) \
    public: \
    static Octo::CommandRegistry* getCommandRegistry() { return getter; } \
    virtual Octo::CommandRegistry* _getCommandRegistry() const override { return getter; }
