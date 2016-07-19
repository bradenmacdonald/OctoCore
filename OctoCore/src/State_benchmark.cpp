#include "OctoCore/Exception.h"
#include "OctoCore/State.h"

#include <gtest/gtest.h>
    using ::testing::Test;

#define SQL(a) #a

using Octo::State;
using Octo::Command;
using Octo::wrap;
using std::string;

/** Inventory Management tests: Benchmark OctoState performance
 *  and test integration of various classes
 */
class InventoryState : public Octo::State {
public:
    InventoryState(SessionId sessionId) : State(sessionId) {}

    // A simple model:
    class Transaction {
    public:
        Transaction() : m_date(0), m_amount(0), m_description() {}
        Transaction(uint64_t date, double amount, string description) :
            m_date(date), m_amount(amount), m_description(description)
        {}
        auto date() const { return m_date; }
        auto amount() const { return m_amount; }
        auto description() const { return m_description; }
    private:
        uint64_t m_date;
        double m_amount;
        string m_description;
    };

    std::map<string, double> m_inventory; // Map of item_name -> quantity
    std::map<ObjectId, Transaction> m_ledger;

    double getAccountBalance() {
        double balance = 0.0;
        for (auto it : m_ledger) {
            balance += it.second.amount();
        }
        return balance;
    }

    double checkInventoryOf(const char* item) {
        if (m_inventory.count(item)) {
            return m_inventory[item];
        }
        return 0;
    }
    
    OCTO_STATE_DEFAULTS; // Use default command registry
};

namespace {
    const char* const EGGS = "eggs";
    const char* const FLOUR = "flour";
    const char* const CAKES = "cakes";
    const char* const LOAVES = "loaves";
}

/** FundCompanyCommand */
struct FundCompanyCommand : public Command<InventoryState, 17> {
    FundCompanyCommand(double _amount) {
        amount() = _amount;
    }
    using Command::Command; // Inherit required base class constructor
    OCTO_ARG(double, amount);
    OCTO_RESULTS(
        OCTO_RESULT(ObjectId, new_ledger_entry_id);
    );

    void forward(State* state, Result& result) const {
        if (not result.has_new_ledger_entry_id()) {
            result.set_new_ledger_entry_id(state->getNextObjectId());
        }
        state->m_ledger[result.new_ledger_entry_id()] = InventoryState::Transaction(0, amount(), "Funded Company");
    }
    void backward(State* state, const Result result) const {
        state->m_ledger.erase(result.new_ledger_entry_id());
    }
};

REGISTER_OCTO_COMMAND(FundCompanyCommand);

/** PurchaseCommand */
struct PurchaseCommand : public Command<InventoryState, 37> {
    using Command::Command;
    
    OCTO_ARG(std::string, item);
    OCTO_ARG(double, unit_price);
    OCTO_ARG(double, qty);
    OCTO_RESULTS(
        OCTO_RESULT(ObjectId, new_ledger_entry_id);
    )

    PurchaseCommand(std::string&& _item, double _unitPrice, double _qty = 1) {
        item() = _item;
        unit_price() = _unitPrice;
        qty() = _qty;
    }

    void forward(State* state, Result& result) const {
        if (not result.has_new_ledger_entry_id()) {
            result.set_new_ledger_entry_id(state->getNextObjectId());
        }
        const std::string description = std::string("Purchased ") + item();
        const double cost = qty() * unit_price();
        state->m_inventory[item()] += qty();
        state->m_ledger[result.new_ledger_entry_id()] = InventoryState::Transaction(0, -cost, description);
    }

    void backward(State* state, const Result result) const {
        state->m_inventory[item()] -= qty();
        state->m_ledger.erase(result.new_ledger_entry_id());
    }
};
REGISTER_OCTO_COMMAND(PurchaseCommand);

/** BakeCommand */
struct BakeCommand : public Command<InventoryState, 19> {
    using Command::Command;
    
    OCTO_ARG(std::string, item);
    OCTO_ARG(StrMap, items_needed);
    OCTO_ARG(double, qty);
    OCTO_RESULTS()

    BakeCommand(std::string&& _item, double _qty = 1) {
        item() = _item;
        qty() = _qty;
        if (_item == CAKES) {
            (*items_needed())[EGGS] = wrap(4 * _qty);
            (*items_needed())[FLOUR] = wrap(6 * _qty);
        } else if (_item == LOAVES) {
            (*items_needed())[EGGS] = wrap(1.5 * _qty);
            (*items_needed())[FLOUR] = wrap(5 * _qty);
        }
    }
    void run(State* state, bool forward) const {
        for (auto& needed : items_needed()) {
            // needed is a pair of item, qty
            const std::string& needed_item = needed.first;
            const double needed_qty = needed.second.real();
            state->m_inventory[needed_item] += forward ? -needed_qty : needed_qty;
        }
        state->m_inventory[item()] += forward ? qty() : -qty();
    }

    void forward(State* state, Result& result) const { run(state, true); }
    void backward(State* state, const Result result) const { run(state, false); }

};

REGISTER_OCTO_COMMAND(BakeCommand);

// Define and register a few more commands to simulate a larger command set
#define ADD_EMPTY_COMMAND(number) \
    struct OtherCommand ## number : public Command<InventoryState, number> { \
        using Command::Command; \
        OCTO_RESULTS() \
        void forward(State* state, Result& result) const {} \
        void backward(State* state, const Result result) const {} \
    }; \
    REGISTER_OCTO_COMMAND(OtherCommand ## number);
ADD_EMPTY_COMMAND(1);
ADD_EMPTY_COMMAND(2);
ADD_EMPTY_COMMAND(3);
ADD_EMPTY_COMMAND(4);
ADD_EMPTY_COMMAND(5);
ADD_EMPTY_COMMAND(6);
ADD_EMPTY_COMMAND(7);
ADD_EMPTY_COMMAND(8);
ADD_EMPTY_COMMAND(9);
ADD_EMPTY_COMMAND(10);
ADD_EMPTY_COMMAND(20);
ADD_EMPTY_COMMAND(30);
ADD_EMPTY_COMMAND(40);
ADD_EMPTY_COMMAND(50);
ADD_EMPTY_COMMAND(200);

namespace testing {

    const int NUM_ITERATIONS = 200;

    TEST(StateBenchmark, benchmark_init) {
        for (uint16_t i = 0; i < NUM_ITERATIONS; i++) {
            InventoryState bakery { i };
            ASSERT_EQ(bakery.getAccountBalance(), 0.0);
        }
    }

    TEST(StateBenchmark, benchmark_scenario) {
        for (uint16_t i = 0; i < NUM_ITERATIONS; i++) {
            InventoryState bakery { i };
            ASSERT_EQ(bakery.getAccountBalance(), 0.0);
            ASSERT_EQ(bakery.checkInventoryOf(EGGS), 0.0);
            
            // Invest $10k money into the company:
            FundCompanyCommand fc {10000.0};
            Octo::ObjectId entry_id = bakery.runCommand(fc).new_ledger_entry_id();
            {
                auto entry = bakery.m_ledger[entry_id];
                ASSERT_EQ(entry.description(), "Funded Company");
            }
            while (bakery.canUndo()) { bakery.undo(); }
            ASSERT_EQ(bakery.m_ledger.count(entry_id), 0);
            while (bakery.canRedo()) { bakery.redo(); }
            {
                auto entry = bakery.m_ledger[entry_id];
                ASSERT_EQ(entry.description(), "Funded Company");
            }

            ASSERT_EQ(bakery.getAccountBalance(), 10000.0);
            
            // Buy 240 eggs at $10 each, in two separate transactions:
            bakery.runCommand(PurchaseCommand(EGGS, 10, 120));
            bakery.runCommand(PurchaseCommand(EGGS, 10, 120));
            while (bakery.canUndo()) { bakery.undo(); }
            while (bakery.canRedo()) { bakery.redo(); }
            ASSERT_EQ(bakery.checkInventoryOf(EGGS), 240);
            ASSERT_EQ(bakery.getAccountBalance(), 10000 - 2400);
            
            // Buy 500 units of flour at $1 each:
            bakery.runCommand(PurchaseCommand(FLOUR, 1, 500));
            while (bakery.canUndo()) { bakery.undo(); }
            while (bakery.canRedo()) { bakery.redo(); }
            ASSERT_EQ(bakery.checkInventoryOf(EGGS), 240);
            ASSERT_EQ(bakery.checkInventoryOf(FLOUR), 500);
            ASSERT_EQ(bakery.getAccountBalance(), 10000 - 2400 - 500);
            
            // Bake some cake and bread:
            bakery.runCommand(BakeCommand(CAKES, 10));
            bakery.runCommand(BakeCommand(LOAVES, 30));
            ASSERT_EQ(bakery.checkInventoryOf(CAKES), 10);
            ASSERT_EQ(bakery.checkInventoryOf(LOAVES), 30);
            ASSERT_EQ(bakery.checkInventoryOf(EGGS), 240 - 10*4 - 30*1.5);
            ASSERT_EQ(bakery.checkInventoryOf(FLOUR), 500 - 10*6 - 30*5);
        }
    }

}
