#include "OctoCore/Command.h"
#include "OctoCore/State.h"
#include <gtest/gtest.h>
    using ::testing::Test;

using Octo::Command;
using Octo::Map;

namespace {
    class SimpleState : public Octo::State {
    public:
        SimpleState() : State(10) {}
        OCTO_STATE_DEFAULTS;
    };
}


/** TestCommand */
struct TestCommand : public Command<SimpleState, 3000> {
    using Command::Command; // Inherit required base class constructor
    OCTO_ARG(bool, bool_arg);
    OCTO_ARG(int64_t, int_arg);
    OCTO_RESULTS(
        OCTO_RESULT(ObjectId, oid_result);
    );

    void forward(State* state, Result& result) const {}
    void backward(State* state, const Result result) const {}
};

REGISTER_OCTO_COMMAND(TestCommand);

namespace testing {

    TEST(CommandTest, test_args_guarantee) {
        // The .args() method of CommandBase guarantees that the returned
        // object will never be modified in the future. Test that.
        TestCommand tc;
        tc.bool_arg() = true;
        tc.int_arg() = 42;
        auto args1 = tc.args();
        tc.bool_arg() = false;
        tc.int_arg() = -50;
        auto args2 = tc.args();
        EXPECT_EQ(args2.use_count(), 2);
        tc.int_arg() = 0;
        EXPECT_EQ(args2.use_count(), 1);

        Octo::FieldId fid = tc.bool_arg_field_id;
        EXPECT_EQ(args1->at(fid).boolean(), true);
        EXPECT_EQ(args1->at(tc.int_arg_field_id).int64(), 42);
        EXPECT_EQ(args1.use_count(), 1);

        EXPECT_EQ(args2->at(fid).boolean(), false);
        EXPECT_EQ(args2->at(tc.int_arg_field_id).int64(), -50);
    }
}
