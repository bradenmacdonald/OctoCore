/**
 * Exception classes used by OctoCore
 * Part of OctoCore by Braden MacDonald
 */
#pragma once

#include <exception>
#include <string>

namespace Octo {

/** Base class for all exceptions thrown by OctoCore */
class Exception : public std::exception {
public:
    virtual const char* what() const noexcept { return "Unknown Exception in OctoCore."; }
};



/** Base class for an State error */
class StateException : public Exception {
    const char* m_error_reason;
public:
    StateException(const char* errorReason) : m_error_reason(errorReason) {}
    virtual const char* what() const noexcept { return m_error_reason; }
};

class InapplicableCommandException : public StateException {
public:
    InapplicableCommandException() : StateException("That Octo::Command is not compatible with that Octo::State.") {}
};


/** Base class for an error in creating/executing a command */
class CommandException : public Exception {};


/** Error when trying to initialize a command that is incompatible with the current State */
class CommandWillNotApplyException : public CommandException {
    const char* m_error_reason;
public:
    CommandWillNotApplyException(const char* errorReason) : m_error_reason(errorReason) {}
    virtual const char* what() const noexcept { return m_error_reason; }
};

/** Error when trying to initialize a command that is incompatible with the current State */
class CommandResultMisuseException : public CommandException {
public:
    CommandResultMisuseException() {}
    virtual const char* what() const noexcept {
        return "Result data should only be modified on the first call to forward()";
    }
};

} // namespace Octo
