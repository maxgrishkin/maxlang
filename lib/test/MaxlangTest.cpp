#include "maxlang/state.h"
#include <gtest/gtest.h>

TEST(Eblang, Math1) {
    maxlang::State g;
    EXPECT_EQ(std::get<int>(g.evaluate("1 + 2 * 3")), 7);
}

TEST(Eblang, Math2) {
    maxlang::State g;
    EXPECT_EQ(std::get<int>(g.evaluate("2 * 3 + 1")), 7);
}

TEST(Eblang, Math3) {
    maxlang::State g;
    EXPECT_EQ(std::get<int>(g.evaluate("(1 + 2) * 3")), 9);
}

TEST(Eblang, Math4) {
    maxlang::State g;
    EXPECT_EQ(std::get<int>(g.evaluate("(2 * 3) + 1")), 7);
}

TEST(Eblang, Math5) {
    maxlang::State g;
    EXPECT_EQ(std::get<int>(g.evaluate("3 * (1 + 2)")), 9);
}

TEST(Eblang, Math6) {
    maxlang::State g;
    EXPECT_EQ(std::get<int>(g.evaluate("(3 * 1) + 2")), 5);
}

TEST(Eblang, FunctionCall1) {
    maxlang::State g;
    bool called = false;
    g.context().functions["func"] = { [&](maxlang::Context& context, std::vector<maxlang::Value> args) -> maxlang::Value {
        EXPECT_TRUE(args.empty());
        called = true;
        return std::monostate {};
    } };
    EXPECT_TRUE(std::holds_alternative<std::monostate>(g.evaluate("func()")));
    EXPECT_TRUE(called);
}

TEST(Eblang, FunctionCall2) {
    maxlang::State g;
    bool called = false;
    g.context().functions["func"] = {
        .value = [&](maxlang::Context& context, std::vector<maxlang::Value> args) -> maxlang::Value {
            EXPECT_TRUE(args.empty());
            called = true;
            return 228;
        },
    };
    EXPECT_EQ(std::get<int>(g.evaluate("func()")), 228);
    EXPECT_TRUE(called);
}

TEST(Eblang, FunctionCall3) {
    maxlang::State g;
    bool called = false;
    g.context().functions["func"] = {
        .value = [&](maxlang::Context& context, std::vector<maxlang::Value> args) -> maxlang::Value {
            if (args.size() != 1) {
                throw std::runtime_error("");
            }
            called = true;
            EXPECT_EQ(std::get<int>(args[0]), 228);
            return std::monostate {};
        },
    };
    EXPECT_TRUE(std::holds_alternative<std::monostate>(g.evaluate("func(228)")));
    EXPECT_THROW(std::holds_alternative<std::monostate>(g.evaluate("func()")), std::runtime_error);
    EXPECT_THROW(std::holds_alternative<std::monostate>(g.evaluate("func(228, 322)")), std::runtime_error);
    EXPECT_TRUE(called);
}

TEST(Eblang, FunctionCall4) {
    maxlang::State g;
    bool called = false;
    g.context().functions["func"] = {
        .value = [&](maxlang::Context& context, std::vector<maxlang::Value> args) -> maxlang::Value {
            if (args.size() != 2) {
                throw std::runtime_error("");
            }
            called = true;
            EXPECT_EQ(std::get<int>(args[0]), 228);
            EXPECT_EQ(std::get<std::string>(args[1]), "322");
            return std::monostate {};
        },
    };
    EXPECT_TRUE(std::holds_alternative<std::monostate>(g.evaluate("func(228, \"322\")")));
    EXPECT_THROW(std::holds_alternative<std::monostate>(g.evaluate("func()")), std::runtime_error);
    EXPECT_THROW(std::holds_alternative<std::monostate>(g.evaluate("func(228)")), std::runtime_error);
    EXPECT_TRUE(called);
}

TEST(Eblang, FunctionCall5) {
    maxlang::State g;
    int call = 0;
    g.context().functions["println"] = {
        .value = [&](maxlang::Context& context, std::vector<maxlang::Value> args) -> maxlang::Value {
            switch (call++) {
                case 0: // first call
                    EXPECT_EQ(args.size(), 1);
                    EXPECT_EQ(std::get<int>(args[0]), 228);
                    break;
                case 1: // second call
                    EXPECT_EQ(args.size(), 2);
                    EXPECT_EQ(std::get<int>(args[0]), 228);
                    EXPECT_EQ(std::get<std::string>(args[1]), "322");
                    break;
            }
            return std::monostate {};
        },
    };
    EXPECT_TRUE(std::holds_alternative<std::monostate>(g.evaluate("println(228)")));
    EXPECT_TRUE(std::holds_alternative<std::monostate>(g.evaluate("println(228, \"322\")")));
    EXPECT_EQ(call, 2);
}

TEST(Eblang, VariableAssignment) {
    maxlang::State g;
    g.evaluate("a = 123");
    EXPECT_EQ(std::get<int>(g.context().variables["a"]), 123);
}

TEST(Eblang, RunProgram) {
    maxlang::State g;
    g.run(R"(
a = 123;
b = 456;
)");
    EXPECT_EQ(std::get<int>(g.context().variables["a"]), 123);
    EXPECT_EQ(std::get<int>(g.context().variables["b"]), 456);
}

TEST(Eblang, IfCondition) {
    std::string code = R"(
if (a == 1) {
  positive();
}
)";
    maxlang::State g;
    bool called = false;
    g.context().functions["positive"] = {
        .value = [&](maxlang::Context& context, std::vector<maxlang::Value> args) -> maxlang::Value {
            EXPECT_EQ(args.size(), 0);
            called = true;
            return std::monostate {};
        },
    };
    g.context().variables["a"] = 1;
    g.run(code);
    EXPECT_TRUE(called);

    called = false;

    g.context().variables["a"] = 2;
    g.run(code);
    EXPECT_FALSE(called);
}

TEST(Eblang, Return) {
    maxlang::State g;
    g.context().functions["reallybad"] = {
        .value = [&](maxlang::Context& context, std::vector<maxlang::Value> args) -> maxlang::Value {
            throw std::runtime_error("should not be called");
        },
    };
    g.run(R"(
a = 123;
return;
reallybad();
)");
    EXPECT_EQ(std::get<int>(g.context().variables["a"]), 123);
}

TEST(Eblang, IntEqual) {
    maxlang::State g;
    EXPECT_EQ(std::get<int>(g.evaluate("1 == 1")), true);
    EXPECT_EQ(std::get<int>(g.evaluate("0 == 1")), false);
}

TEST(Eblang, StringEqual) {
    maxlang::State g;
    EXPECT_EQ(std::get<int>(g.evaluate("\"a\" == \"a\"")), true);
    EXPECT_EQ(std::get<int>(g.evaluate("\"a\" == \"b\"")), false);
}

TEST(Eblang, StringConcat) {
    maxlang::State g;
    EXPECT_EQ(std::get<std::string>(g.evaluate("\"a\" + \"b\"")), "ab");
}
TEST(Eblang, While) {
    maxlang::State g;
    EXPECT_EQ(std::get<std::string>(g.evaluate("a = '';while (a != 'aaaaaaaaaa'){a = a + 'a'}")), "aaaaaaaaaa");
}
