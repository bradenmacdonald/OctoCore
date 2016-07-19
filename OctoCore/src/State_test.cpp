#include "OctoCore/Exception.h"
#include "OctoCore/State.h"

#include <gtest/gtest.h>
    using ::testing::Test;

using Octo::State;
using Octo::Command;

// A Trivial State for Food Orders - has one command, and no models. Used for basic tests.
class FoodOrdersState : public State {
public:
    FoodOrdersState() : State(1), m_orders(0) {}
    int m_orders = 1;
    OCTO_STATE_DEFAULTS; // Use default command registry
};

struct PlaceOrder : public Command<FoodOrdersState, 1> {
    using Command::Command;
    OCTO_RESULTS()
    void forward(State* state, Result& result) const { state->m_orders++; }
    void backward(State* state, Result) const { state->m_orders--; }
};

REGISTER_OCTO_COMMAND(PlaceOrder);


namespace testing {

    TEST(StateTest, create_state) {
        FoodOrdersState state;
        EXPECT_EQ(0, state.m_orders);
    }

    TEST(StateTest, test_simple_undo_redo) {
        FoodOrdersState state;
        EXPECT_EQ(0, state.m_orders);

        state.runCommand(PlaceOrder());
        EXPECT_EQ(1, state.m_orders);
        EXPECT_TRUE(state.canUndo());
        EXPECT_FALSE(state.canRedo());

        state.undo();
        EXPECT_EQ(0, state.m_orders);
        EXPECT_FALSE(state.canUndo());
        EXPECT_TRUE(state.canRedo());

        state.redo();
        EXPECT_EQ(1, state.m_orders);
        EXPECT_TRUE(state.canUndo());
        EXPECT_FALSE(state.canRedo());
    }
}

// BasicState: State for basic tests that involve the OctoStore
namespace {
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
    struct InsertEmployeesCommand : public Command<BasicState, 1> {
        OCTO_ARG(StrList, names);
        OCTO_RESULTS(
            OCTO_RESULT(IntList, employee_ids);
        )

        InsertEmployeesCommand() {}
        InsertEmployeesCommand& addName(const char* name) {
            *(*names()).Add() = name;
            return *this;
        }
        using Command::Command;
        void forward(State* state, Result& result) const {
            for (auto name : names()) {
                if (state->hasName(name.c_str())) {
                    throw Octo::CommandWillNotApplyException((string("Name already exists: ") + name).c_str());
                }
            }
            IntList new_ids;
            for (auto name : names()) {
                ObjectId obj_id = state->getNextObjectId();
                new_ids.Add(obj_id);
                state->m_employees[obj_id] = Employee { name, 0 };
            }
            result.set_employee_ids(new_ids);
        }
        void backward(State* state, Result) const { throw Octo::StateException("Not implemented"); }
    };
    REGISTER_OCTO_COMMAND(InsertEmployeesCommand);
}

namespace testing {
    
    TEST(BasicStateTest, test_get_next_object_id) {
        BasicState state;
        
        // We expect the first object ID from this state to be 1, but with the session ID (also 1) 48 bits to the left.
        const State::ObjectId id1 = 10ull << 48 | 1;
        EXPECT_EQ(state.getNextObjectId(), id1);
        
        // Then Object IDs should increment
        const State::ObjectId id2 = id1 + 1;
        EXPECT_EQ(state.getNextObjectId(), id2);
    }
    
    TEST(BasicStateTest, test_transaction_atomicity) {
        BasicState state;
        EXPECT_EQ(state.hasName("alice"), false);
        EXPECT_EQ(state.hasName("bob"), false);
        EXPECT_EQ(state.hasName("cameron"), false);
        state.runCommand(InsertEmployeesCommand().addName("alice").addName("bob"));
        EXPECT_EQ(state.hasName("alice"), true);
        EXPECT_EQ(state.hasName("bob"), true);
        EXPECT_EQ(state.hasName("cameron"), false);
        
        // Now if we try to insert two names and the second fails,
        // the whole command should fail atomically.
        // This is not automatic in any way - each command must be carefully implemented to be
        // atomic.
        EXPECT_THROW(
            state.runCommand(InsertEmployeesCommand().addName("cameron").addName("bob")),
            Octo::CommandException
        );
        EXPECT_EQ(state.hasName("cameron"), false);
    }
}

// DataTypesState: State for testing all supported datatypes
namespace {
    class DataTypesState : public State {
    public:
        DataTypesState() : State(10) {}
        Octo::GenericValue getValue(const char* key) {
            if (m_key_value_store.count(key)) {
                return m_key_value_store[key];
            }
            return Octo::GenericValue();
        }
        OCTO_STATE_DEFAULTS;
        std::map<std::string, Octo::GenericValue> m_key_value_store;
    };
    struct SetValueCommand : public Command<DataTypesState, 1> {
        using Command::Command;
        // Declare an argument of each possible type. Note that these may be null/unset
        OCTO_ARG(bool, bool_arg);
        OCTO_ARG(int32_t, int32_arg);
        OCTO_ARG(int64_t, int64_arg);
        OCTO_ARG(double, double_arg);
        OCTO_ARG(string, string_arg);
        OCTO_ARG(List, list_arg);
        OCTO_ARG(IntList, int_list_arg);
        OCTO_ARG(StrList, str_list_arg);
        OCTO_ARG(Map, map_arg);
        OCTO_ARG(StrMap, str_map_arg);

        // Declare a result variable of each type to hold the previous values
        OCTO_RESULTS(
            OCTO_RESULT(bool, bool_prev_value);
            OCTO_RESULT(int32_t, int32_prev_value);
            OCTO_RESULT(int64_t, int64_prev_value);
            OCTO_RESULT(double, double_prev_value);
            OCTO_RESULT(string, string_prev_value);
            OCTO_RESULT(List, list_prev_value);
            OCTO_RESULT(IntList, int_list_prev_value);
            OCTO_RESULT(StrList, str_list_prev_value);
            OCTO_RESULT(Map, map_prev_value);
            OCTO_RESULT(StrMap, str_map_prev_value);
        )

        void forward(State* state, Result& result) const {
            if (has_bool_arg()) {
                if (state->m_key_value_store.count("bool")) {
                    result.set_bool_prev_value(state->m_key_value_store["bool"].boolean());
                }
                state->m_key_value_store["bool"] = Octo::wrap(bool_arg());
            }
            if (has_int32_arg()) {
                if (state->m_key_value_store.count("int32")) {
                    result.set_int32_prev_value(state->m_key_value_store["int32"].int32());
                }
                state->m_key_value_store["int32"] = Octo::wrap(int32_arg());
            }
            if (has_int64_arg()) {
                if (state->m_key_value_store.count("int64")) {
                    result.set_int64_prev_value(state->m_key_value_store["int64"].int64());
                }
                state->m_key_value_store["int64"] = Octo::wrap(int64_arg());
            }
            if (has_double_arg()) {
                if (state->m_key_value_store.count("double")) {
                    result.set_double_prev_value(state->m_key_value_store["double"].real());
                }
                state->m_key_value_store["double"] = Octo::wrap(double_arg());
            }
            if (has_string_arg()) {
                if (state->m_key_value_store.count("string")) {
                    result.set_string_prev_value(state->m_key_value_store["string"].string());
                }
                state->m_key_value_store["string"] = Octo::wrap(string_arg());
            }
        }
        void backward(State* state, Result result) const {
            if (has_bool_arg()) {
                if (result.has_bool_prev_value()) {
                    state->m_key_value_store["bool"] = Octo::wrap(result.bool_prev_value());
                } else { state->m_key_value_store.erase("bool"); }
            }
            if (has_int32_arg()) {
                if (result.has_int32_prev_value()) {
                    state->m_key_value_store["int32"] = Octo::wrap(result.int32_prev_value());
                } else { state->m_key_value_store.erase("int32"); }
            }
            if (has_int64_arg()) {
                if (result.has_int64_prev_value()) {
                    state->m_key_value_store["int64"] = Octo::wrap(result.int64_prev_value());
                } else { state->m_key_value_store.erase("int64"); }
            }
            if (has_double_arg()) {
                if (result.has_double_prev_value()) {
                    state->m_key_value_store["double"] = Octo::wrap(result.double_prev_value());
                } else { state->m_key_value_store.erase("double"); }
            }
            if (has_string_arg()) {
                if (result.has_string_prev_value()) {
                    state->m_key_value_store["string"] = Octo::wrap(result.string_prev_value());
                } else { state->m_key_value_store.erase("string"); }
            }
        }
    };
    REGISTER_OCTO_COMMAND(SetValueCommand);
}

namespace testing {
    TEST(DataTypesStateTest, test_data_types) {
        DataTypesState state;

        // Set a bool and an int
        SetValueCommand cmd {};
        cmd.bool_arg() = true;
        cmd.int32_arg() = 15;
        state.runCommand(cmd);
        ASSERT_EQ(state.getValue("bool").boolean(), true);
        ASSERT_EQ(state.getValue("int32").int32(), 15);
        ASSERT_EQ(state.getValue("int64").has_int64(), false);
        
        // Set a bunch of values
        SetValueCommand cmd2 {};
        cmd2.bool_arg() = false;
        cmd2.int32_arg() = 0;
        cmd2.int64_arg() = -72036854775807;
        cmd2.double_arg() = 3.40;
        cmd2.string_arg() = "So say we all.";
        state.runCommand(cmd2);
        ASSERT_EQ(state.getValue("bool").boolean(), false);
        ASSERT_EQ(state.getValue("int32").int32(), 0);
        ASSERT_EQ(state.getValue("int64").int64(), -72036854775807);
        ASSERT_EQ(state.getValue("double").real(), 3.40);
        ASSERT_EQ(state.getValue("string").string(), "So say we all.");
        
        // Undo:
        state.undo();
        ASSERT_EQ(state.getValue("bool").boolean(), true);
        ASSERT_EQ(state.getValue("int32").int32(), 15);
        ASSERT_EQ(state.getValue("int64").has_int64(), false);
        ASSERT_EQ(state.getValue("double").has_real(), false);
        ASSERT_EQ(state.getValue("string").has_string(), false);
    }
    TEST(DataTypesStateTest, test_container_types) {
        // The container types cannot easily be stored in the DB at the moment, so we test them as follows:
        SetValueCommand cmd {};
        *cmd.list_arg()->Add() = Octo::wrap("list element 1");
        *cmd.list_arg()->Add() = Octo::wrap(2.0); // This mixed list holds a string and a double
        cmd.int_list_arg()->Add(1);
        cmd.int_list_arg()->Add(2);
        *cmd.str_list_arg()->Add() = "list element 1";
        *cmd.str_list_arg()->Add() = "list element 2";
        (*cmd.map_arg())[100] = Octo::wrap("one hundred");
        ASSERT_EQ(cmd.map_arg()->count(100), 1);
        ASSERT_EQ(cmd.map_arg()->at(100).value_case(), Octo::GenericValue::ValueCase::kString);
        ASSERT_EQ(cmd.map_arg()->at(100).has_string(), true);
        ASSERT_EQ(cmd.map_arg()->at(100).string(), "one hundred");
        cmd.map_arg()[200] = Octo::wrap(200.0);
        cmd.str_map_arg()["alpha"] = Octo::wrap("α");
        cmd.str_map_arg()["beta"] = Octo::wrap("β");

        // Copy the command:
        auto args = std::make_shared<Octo::Map>(*cmd.args());
        SetValueCommand cmd_copy{ args };
        ASSERT_EQ(cmd_copy.int_list_arg()->size(), 2);
        ASSERT_EQ(cmd_copy.int_list_arg()->Get(0), 1);
        ASSERT_EQ(cmd_copy.int_list_arg()->Get(1), 2);

        ASSERT_EQ(cmd_copy.str_list_arg()->size(), 2);
        ASSERT_EQ(cmd_copy.str_list_arg()->Get(0), "list element 1");
        ASSERT_EQ(cmd_copy.str_list_arg()->Get(1), "list element 2");
        
        ASSERT_EQ(cmd_copy.map_arg()->size(), 2);
        ASSERT_EQ(cmd_copy.map_arg()->at(100).string(), "one hundred");
        ASSERT_EQ(cmd_copy.map_arg()[200].real(), 200.0);
        
        ASSERT_EQ(cmd_copy.str_map_arg()->size(), 2);
        ASSERT_EQ(cmd_copy.str_map_arg()["alpha"].string(), "α");
        ASSERT_EQ(cmd_copy.str_map_arg()["beta"].string(), "β");
    }
}

// Plant States Test: Test polymorphic states and commands
namespace {
    Octo::CommandRegistry* getSharedCommandRegistry() { static Octo::CommandRegistry registry; return &registry; }
}
/* Interface indicating edibility */
class IEdible {
public:
    int edible_cmd_count = 10;
    static Octo::CommandRegistry* getCommandRegistry() { return getSharedCommandRegistry(); }
};
class PlantState : public State {
public:
    int plant_cmd_count = 0;
    PlantState() : State(1) {}
    OCTO_STATE_COMMAND_REGISTRY(getSharedCommandRegistry());
};
class TreeState : public PlantState {
public:
    int tree_cmd_count = 0;
    TreeState() {}
    OCTO_STATE_COMMAND_REGISTRY(getSharedCommandRegistry());
};
class CedarState : public TreeState {
public:
    CedarState() {}
    OCTO_STATE_COMMAND_REGISTRY(getSharedCommandRegistry());
};
class PotatoState : public PlantState, public IEdible {
public:
    PotatoState() {}
    OCTO_STATE_COMMAND_REGISTRY(getSharedCommandRegistry());
};
struct EdibleCommand : public Command<IEdible, 1> {
    using Command::Command;
    OCTO_RESULTS()
    void forward(State* state, Result& result) const { state->edible_cmd_count++; }
    void backward(State* state, Result) const { state->edible_cmd_count--; }
};
REGISTER_OCTO_COMMAND(EdibleCommand);
struct TreeCommand : public Command<TreeState, 2> {
    using Command::Command;
    OCTO_RESULTS()
    void forward(State* state, Result& result) const { state->tree_cmd_count++; }
    void backward(State* state, Result) const { state->tree_cmd_count--; }
};
REGISTER_OCTO_COMMAND(TreeCommand);
struct PlantCommand : public Command<PlantState, 3> {
    using Command::Command;
    OCTO_RESULTS()
    void forward(State* state, Result& result) const { state->plant_cmd_count++; }
    void backward(State* state, Result) const { state->plant_cmd_count--; }
};
REGISTER_OCTO_COMMAND(PlantCommand);
#include <iostream>
namespace testing {

    TEST(PolymorphismTest, test_applicability) {
        PlantState plant;
        TreeState tree;
        CedarState cedar;
        PotatoState potato;
        
        // Test PlantCommand:
        EXPECT_EQ(plant.plant_cmd_count, 0);
        plant.runCommand(PlantCommand());
        EXPECT_EQ(plant.plant_cmd_count, 1);

        tree.runCommand(PlantCommand());
        EXPECT_EQ(tree.plant_cmd_count, 1);

        cedar.runCommand(PlantCommand());
        EXPECT_EQ(cedar.plant_cmd_count, 1);

        potato.runCommand(PlantCommand());
        EXPECT_EQ(potato.plant_cmd_count, 1);
        
        // Test EdibleCommand:
        EXPECT_THROW(plant.runCommand(EdibleCommand()), Octo::InapplicableCommandException);
        EXPECT_THROW(tree.runCommand(EdibleCommand()), Octo::InapplicableCommandException);
        EXPECT_THROW(cedar.runCommand(EdibleCommand()), Octo::InapplicableCommandException);

        EXPECT_EQ(potato.edible_cmd_count, 10);
        potato.runCommand(EdibleCommand());
        EXPECT_EQ(potato.edible_cmd_count, 11);
        potato.undo();
        EXPECT_EQ(potato.edible_cmd_count, 10);
        potato.redo();
        EXPECT_EQ(potato.edible_cmd_count, 11);
        
        // Test TreeCommand:
        EXPECT_THROW(plant.runCommand(TreeCommand()), Octo::InapplicableCommandException);

        EXPECT_EQ(tree.tree_cmd_count, 0);
        tree.runCommand(TreeCommand());
        EXPECT_EQ(tree.tree_cmd_count, 1);
        
        EXPECT_EQ(cedar.tree_cmd_count, 0);
        cedar.runCommand(TreeCommand());
        EXPECT_EQ(cedar.tree_cmd_count, 1);
        cedar.undo();
        EXPECT_EQ(cedar.tree_cmd_count, 0);
        cedar.redo();
        EXPECT_EQ(cedar.tree_cmd_count, 1);
        
        EXPECT_THROW(potato.runCommand(TreeCommand()), Octo::InapplicableCommandException);
    }
}
