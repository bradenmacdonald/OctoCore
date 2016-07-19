# OctoCore

C++ library for modelling data that is modified using the Command Pattern.

Designed so that multiple clients can be editing the state at the same time.
For example, the data ("State") could be an online document that multiple people are editing
simultaneously - like Google Docs.

Commands can be easily serialized using protocol buffers.

Implements an `ObjectId` type that will ensure commands from different clients don't conflict.

Includes undo/redo functionality.

Emcripten compatible.

There are six fundamental data types that can be used to model the state and command parameters:
* `bool`
* `int32`
* `int64`
* `double`
* `string`
* `blob`

There are also five container types:
* `List`    - A list of GenericValues
* `IntList` - A list of 64-bit integers (or ObjectIds)
* `StrList` - A list of strings
* `Map`     - A map where the keys are 32-bit integers (FieldId) and the values are generic
* `StrMap`  - A map where the keys are strings and the values are generic

Then changes to the state are defined by implementing `Command` classes that have a `forward()`
and `backward()` method.

## Compilation and Testing

To build, make sure you have CMake installed, and simply run `make`.

To run the tests, run `make test`.

## Example: Employee State

```c++
#include "OctoCore/Exception.h"
#include "OctoCore/State.h"

using Octo::State;
using Octo::Command;


// State definition:
struct Employee {
    std::string name;
    int start_date;
};
class BasicState : public State {
public:
    BasicState() : State(10) {}
    bool hasName(const char* name) {
        for (auto employee : m_employees) {
            if (employee.second.name == name)
                return true;
        }
        return false;
    }
    OCTO_STATE_DEFAULTS;
    std::map<ObjectId, Employee> m_employees;
};

// Commands:
struct InsertEmployeesCommand : public Command<BasicState, 1> {
    // Arguments that this command accepts:
    OCTO_ARG(StrList, names); // A list of names to add as new employees
    // Result data created by this command:
    OCTO_RESULTS(
        OCTO_RESULT(IntList, employee_ids); // The resulting employee IDs
    )

    InsertEmployeesCommand() {}
    InsertEmployeesCommand& addName(const char* name) {
        *(*names()).Add() = name;
        return *this;
    }
    using Command::Command;
    void forward(State* state, Result& result) const {
        // Generate ObjectIds, if needed:
        if (!result.has_employee_ids()) { // Compute new IDs when this command first runs, but not during redo()
            IntList new_ids;
            for (int i = 0; i < names().size(); i++) { new_ids.Add( state->getNextObjectId() ); }
            result.set_employee_ids(new_ids);
        }
        // Insert the new employees
        auto id_iterator = result.employee_ids().begin();
        for (auto name : names()) {
            const ObjectId id = *(id_iterator++);
            state->m_employees[id] = Employee { name, 0 };
        }
        
    }
    void backward(State* state, const Result& result) const {
        // Delete the new employees
        for (auto id : result.employee_ids()) {
            state->m_employees.erase(id);
        }
    }
};
REGISTER_OCTO_COMMAND(InsertEmployeesCommand);

// Usage example:
BasicState state;
state.runCommand(InsertEmployeesCommand().addName("alice").addName("bob"));
// state.hasName("alice") == true
// state.hasName("bob") == true
// state.hasName("cameron") == false
state.undo();
// state.hasName("alice") == false
// state.hasName("bob") == false
// state.hasName("cameron") == false
state.redo();
// state.hasName("alice") == true
// state.hasName("bob") == true
// state.hasName("cameron") == false
```
