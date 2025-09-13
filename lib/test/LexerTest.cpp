#include "maxlang/lexer.h"
#include "maxlang/token.h"
#include <gtest/gtest.h>

using namespace maxlang::token;

TEST(Lexer, Case1) {
    auto processed = maxlang::lexer::process(R"(
fn main() {
    return 228 + 322;
}
)");
    std::vector<Any> expected{
        Keyword::FN,
        Identifier { .value = "main" },
        LPar{},
        RPar{},
        LCurlyBracket{},
        Keyword::RETURN,
        Integer { .value = 228 },
        Plus{},
        Integer { .value = 322 },
        Semicolon{},
        RCurlyBracket{},
    };

    // Сравниваем только токены (первые элементы пар), игнорируя номера строк
    ASSERT_EQ(processed.size(), expected.size());
    for (size_t i = 0; i < processed.size(); ++i) {
        EXPECT_EQ(processed[i].first, expected[i]);
    }
}

TEST(Lexer, Case2) {
    auto processed = maxlang::lexer::process(R"(
"hello"
)");
    std::vector<Any> expected{
        String { .value = "hello" },
    };

    // Сравниваем только токены (первые элементы пар), игнорируя номера строк
    ASSERT_EQ(processed.size(), expected.size());
    for (size_t i = 0; i < processed.size(); ++i) {
        EXPECT_EQ(processed[i].first, expected[i]);
    }
}