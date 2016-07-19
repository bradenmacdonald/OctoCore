#pragma once
#include <climits>
#include <memory>
#include <string>
#include <unordered_map>

#include "DataTypes.h"
#include "FieldHash.h"
#include "Exception.h"

namespace Octo {

class State;

/** Base class for Command.
 *  You should never need to use this class directly.
 */
class CommandBase {
public:
    // Convenience typedefs for subclasses:
    using string = string;
    using FieldId = FieldId;
    using CommandId = int32_t;
    using ObjectId = ObjectId;
    using List = List;
    using IntList = IntList;
    using StrList = StrList;
    using Map = Map;
    using StrMap = StrMap;
    /** Normal constructor for use by derived classes */
    CommandBase(CommandId commandId) : m_command_id(commandId), m_args(std::make_shared<Map>()) {}
protected:
    /** Internal constructor that points to existing arguments.
     *  This is used to re-create an instance of a command subclass just prior to running it
     *  on an Octo::State.
     */
    CommandBase(CommandId commandId, const std::shared_ptr<Map>& args) : m_command_id(commandId), m_args(args) {}
    /** protected non-virtual destructor to handle memory allocation correctly. */
    ~CommandBase() {}

    CommandBase(const CommandBase&) = delete; // No copy constructor
    CommandBase& operator=(const CommandBase& r) = delete; // No assignment operator
public:
    /** args(): Get this command's arguments (read-only).
     *  The result of this call can be stored indefinitely and is guaranteed to never change.
     */
    std::shared_ptr<const Map> args() const { return m_args; }
    
    /** Get the ID of this command. The ID is unique within the CommandRegistry. */
    CommandId commandId() const { return m_command_id; }
protected:
    /** Get a raw constant pointer to arguments. Used by ArgField only. */
    const Map* argsReadOnly() const { return m_args.get(); }
    /** Get a raw mutable pointer to arguments. Used by ArgField only. */
    Map* argsMutable() {
        // This implements copy-on-write: If some other object has stored/copied a pointer to
        // our arguments, we will re-create them and work with a new copy.
        if (m_args.use_count() > 1) {
            const Map* old_args = m_args.get();
            m_args = std::make_shared<Map>(*old_args);
        }
        return m_args.get();
    }

    /** ArgField: A wrapper used to make arg fields act more like member variables */
    template<FieldId _fieldID, typename T>
    struct ArgField {
        using UnwrappedType = typename unwrap_as<T>::type;
        using UnwrappedTypeMutable = typename mutate_as<T>::type;
        CommandBase* const m_cmd;
        ArgField(CommandBase* c) : m_cmd(c) {}
        inline void operator=(const T& val) { (*m_cmd->argsMutable())[_fieldID] = wrap(val); }
        inline UnwrappedTypeMutable operator*() {
            GenericValue& value = (*m_cmd->argsMutable())[_fieldID];
            return unwrap<UnwrappedTypeMutable>(value);
        }
        inline operator UnwrappedType() {
            return unwrap<UnwrappedType>(m_cmd->argsReadOnly()->at(_fieldID));
        }
        // For container types, define the -> operator for convenience
        inline typename std::remove_reference<UnwrappedTypeMutable>::type* operator->() {
            GenericValue& value = (*m_cmd->argsMutable())[_fieldID];
            return &(unwrap<UnwrappedTypeMutable>(value));
        }
        // Also pass along the [] operator for container types
        template<typename U>
        inline auto& operator[] (U arg) {
            GenericValue& value = (*m_cmd->argsMutable())[_fieldID];
            return unwrap<UnwrappedTypeMutable>(value)[arg];
        }
    };
public:
    /** Result: base class for wrappers used to provider typed access to result data. */
    struct ResultBase {
        ResultBase(const std::shared_ptr<Map>& data, bool isMutable) : m_is_mutable(isMutable), m_data(data) {}
        ResultBase(const std::shared_ptr<const Map>& data) : m_is_mutable(false), m_data(std::const_pointer_cast<Map>(data)) {}
        /** setResultField: Helper method used to modify result fields. Available as set_{field_name}() */
        template<typename T>
        void setResultField(FieldId _fieldID, T&& value) {
            if (not m_is_mutable) {
                throw CommandResultMisuseException();
            }
            (*m_data)[_fieldID] = wrap(std::move(value));
        }
        const Map* data() const { return m_data.get(); }
    private:
        const bool m_is_mutable;
        const std::shared_ptr<Map> m_data;
    };
private:
    /** m_command_id: Every command has a unique ID */
    const CommandId m_command_id;
    /** m_args: ALL data that describes this command must be stored in here */
    std::shared_ptr<Map> m_args;
};

/** OCTO_ARG(fieldType, fieldName): Create an argument "field" on a command.
 *  Used within an Command subclass to add several methods to the class that  load and save
 *  data to the internal m_args member. This "magic" makes the command serializable so we can do
 *  things like mirror the command on a different state instance (which may be on a remote
 *  server).
 *
 *  Arg[ument] fields should only be modified in the constructor and via mutator methods. It is
 *  an error to modify args during forward() or backward().
 */
#define OCTO_ARG(fieldType, fieldName) \
    enum : FieldId { fieldName##_field_id = #fieldName ## _octo_field_name_hash }; \
    ArgField<fieldName##_field_id, fieldType> fieldName() { return this; } \
    Octo::unwrap_as<fieldType>::type fieldName() const { \
        return Octo::unwrap<Octo::unwrap_as<fieldType>::type>( \
            argsReadOnly()->at(fieldName##_field_id) \
        ); \
    } \
    bool has_##fieldName() const { \
        return ( \
            argsReadOnly()->count(fieldName##_field_id) > 0 && \
            Octo::can_unwrap<fieldType>(argsReadOnly()->at(fieldName##_field_id)) \
        ); \
    }

/** OCTO_RESULTS(): Every Command must include this in its definition. It may contain any number
 *  of OCTO_RESULT() fields as well as custom methods, but it must not contain any data.
 */
#define OCTO_RESULTS(result_fields) \
    struct Result : public ::Octo::CommandBase::ResultBase { \
        using ResultBase::ResultBase; \
        result_fields \
    }; \
    static_assert(sizeof(Result) == sizeof(::Octo::CommandBase::ResultBase), "Result struct cannot have member variables.");

/** OCTO_RESULT(fieldType, fieldName): Create an result "field" on a command's Result
 *  structure.
 *  Used within an Command's OCTO_RESULTS() macro to create a 'Result' struct with methods
 *  that load and save data required to make the command consistently replayable. (So we can do
 *  things like mirror the command on a different state instance (which may be on a remote
 *  server).
 */
#define OCTO_RESULT(fieldType, fieldName) \
    enum : FieldId { fieldName##_field_id = #fieldName ## _octo_field_name_hash }; \
    inline void set_##fieldName(fieldType value) { \
        return setResultField(fieldName##_field_id, value); \
    } \
    Octo::unwrap_as<fieldType>::type fieldName() const { \
        return Octo::unwrap<Octo::unwrap_as<fieldType>::type>( \
            data()->at(fieldName##_field_id) \
        ); \
    } \
    bool has_##fieldName() const { \
        return ( \
            data()->count(fieldName##_field_id) > 0 && \
            Octo::can_unwrap<fieldType>(data()->at(fieldName##_field_id)) \
        ); \
    }


/** Command: Base class template for commands that affect an Octo::State.
 *  Each Command subclass describes a type of atomic change made to an Octo::State.
 *  Commands should always be reversible and consistently replayable (undo-redo).
 *  The Command class is mostly a factory class used to generate the command arguments
 *  stored in m_args. The actual code for effecting the action can only access the Octo::State,
 *  the value of m_args, and a 'results' structure. The results structure is where the code must
 *  store any created object IDs, deleted values, etc. so that the command can be reversed
 *  and/or replayed consistently.
 *
 *  Command subclasses must:
 *   * Implement a default constructor.
 *     (Just add 'using Command::Command;' within the class definition to achieve this.
 *     If you add a custom constructor and destructor, just remember that the desctructor will
 *     need to handle deletion of the command whether it was created with the default
 *     constructor or the custom one.)
 *   * Implement 'void forward(State* state, Result& result) const' to run the command on the
 *     given Octo::State.
 *     (State is a typed pointer to the subclass of Octo::State used in the command template.)
 *   * Implement 'void backward(State* state, const Result result) const' to reverse the effects
 *     of the command on the given Octo::State, using the result field data from an earlier call
 *     to forward().
 *   * Not have any member variables.
 *     (The command will be re-created before it is run, and only the 'm_args' data values will
 *     be preserved, so using other member variables makes it easy to introduce bugs that would
 *     be hard to track down.)
 *   * Use the OCTO_ARG(type, name) macro to declare argument fields. These act somewhat like
 *     member variables, though they are accessed through an auto-generated accessor method. The
 *     data values of these fields will be stored in 'm_args' and are available when executing
 *     (and undoing and redoing) the command on a specific Octo::State. These fields may not be
 *     modified when running the command (in the forward() method) nor when undoing it.
 *     You may declare these fields as private, protected, or public.
 *   * Use OCTO_RESULTS( OCTO_RESULT(type, name), ...) to declare a result structure and its
 *     fields. These are used to store whatever information is required to replay the command
 *     (possibly on another state/server) as well as to undo the command. These fields should
 *     only be modified during the forward() method. It is also required that if a command is
 *     run forward() then backward() then forward(), that the second call to forward must not
 *     modify and of the result values. These fields cannot be modified during backward().
 */
template<class _State, int _commandId>
class Command : public CommandBase {
public:
    using State = _State;
    /** Default constructor */
    Command() : CommandBase(_commandId) {}
    /** Constructor for internal use by OctoCore */
    Command(const std::shared_ptr<Map>& args) : CommandBase(_commandId, args) {}
    /** Compile-time constant accessor for the command ID */
    constexpr static int commandId() { return _commandId; }
    /** acceptStatePtr: Convert an Octo::State pointer in order to run this command.
     *  Given a generic Octo::State pointer, this will return a pointer of the required
     *  subclass/interface type, or nullptr if this command cannot be applied to that state.
     *  The memory address returned by this method may be different than the argument given.
     */
    static State* acceptStatePtr(Octo::State* state) { return dynamic_cast<State*>(state); }
};


/** Used for detecting available commands at runtime */
class CommandRegistry {
private:
    using CommandId = CommandBase::CommandId;
    typedef void (*ForwardFn)(State* state, const std::shared_ptr<const Map>& args, const std::shared_ptr<Map>& result, bool mutableResult);
    typedef void (*BackwardFn)(State* state, const std::shared_ptr<const Map>& args, const std::shared_ptr<const Map>& result);
    /** Entry: Represent a pointer to a command class */
    struct Entry {
        ForwardFn forward;
        BackwardFn backward;
    };
    /** m_entries: The internal map of commands. Allows for fast lookups based on command ID */
    std::unordered_map<CommandId, Entry> m_entries;
    /** registerCommand: Internal method to add a new entry to the registry */
    void registerCommand(CommandId commandId, Entry entry) {
        if (m_entries.count(commandId) != 0) {
            throw StateException("Attempted to register the same command ID twice in the same CommandRegistry.");
        }
        m_entries.emplace(std::make_pair(commandId, entry));
    }
public:
    /** getCommand: Given a command ID, get a handle that allows us to run that command.
     *  This method returns a pointer to a wrapper object with forward() and backward() methods.
     *  If the commandId is invalid, it will return nullptr.
     */
    const Entry* getCommand(CommandId cid) const {
        auto it = m_entries.find(cid);
        return (it != m_entries.end()) ? &(it->second) : nullptr;
    }
    template <class CommandSubclass>
    struct Registration {
        using State = typename CommandSubclass::State;
        /** Construct an instance of the command with the given args and run it forward.
         *  If mutableResult is true, then this command is expected to write out any data
         *  required to replay/undo the command to the 'result' map pointer.
         *  Accessing the 'args' and reading/writing 'result' should both be accomplished using
         *  the typesafe field accessors generated by CommandArg and CommandResult.
         */
        static void forward(Octo::State* state, const std::shared_ptr<const Map>& args, const std::shared_ptr<Map>& result, bool mutableResult) {
            if (State* typed_state = CommandSubclass::acceptStatePtr(state)) {
                // The CommandBase constructor requires mutable args, so we const_cast it.
                // The interface of the forward() method and checks within CommandBase
                // ensure that the args will not be modified by the forward() method.
                std::shared_ptr<Map> args_mutable = std::const_pointer_cast<Map>(args);
                const CommandSubclass cmd {args_mutable};
                typename CommandSubclass::Result res {result, mutableResult};
                cmd.forward(typed_state, res);
            } else { throw InapplicableCommandException(); }
        }
        /** Construct an instance of the command with the given args and run it backward.
         *  Accessing the 'args' and reading 'result' should both be accomplished using
         *  the typesafe field accessors generated by CommandArg and CommandResult.
         */
        static void backward(Octo::State* state, const std::shared_ptr<const Map>& args, const std::shared_ptr<const Map>& result) {
            if (State* typed_state = CommandSubclass::acceptStatePtr(state)) {
                // The CommandBase constructor requires mutable args, so we use const_cast.
                // The interface of the backward() method and checks within CommandBase ensure that neither
                // the args nor the result will be modified by calling backward().
                std::shared_ptr<Map> args_mutable = std::const_pointer_cast<Map>(args);
                const CommandSubclass cmd {args_mutable};
                const typename CommandSubclass::Result res {result};
                cmd.backward(typed_state, res);
            } else { throw InapplicableCommandException(); }
        }
        /** Given a subclass of Command, register it with the appropriate CommandRegistry */
        Registration() {
            static_assert(
                sizeof(CommandSubclass) == sizeof(CommandBase),
                "Command subclasses cannot have data members."
            );
            Entry entry { &Registration::forward, &Registration::backward };
            State::getCommandRegistry()->registerCommand(CommandSubclass::commandId(), entry);
        }
    };
};

} // namespace Octo

#define REGISTER_OCTO_COMMAND(cmd_class) \
    static Octo::CommandRegistry::Registration<cmd_class> cmd_class ## _registration;
