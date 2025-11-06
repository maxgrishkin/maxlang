#include <iostream>
#include "value.h"
#include "util.h"

namespace {
    void impl(std::ostream& os, const maxlang::Value& value) {
        std::visit(
            maxlang::match {
                [&](std::monostate) { os << "<void>"; },
                [&](int v) { os << v; },
                [&](double v) { os << v; },
                [&](const std::string& v) { os << v; },
                [&](char c) { os << c; },
            },
            value);
    }
}   // namespace

std::ostream& maxlang::operator<<(std::ostream& os, const maxlang::Value& value) {
    impl(os, value);
    return os;
}

// Реализация оператора == для Value
bool maxlang::operator==(const Value& a, const Value& b) {
    if (a.index() != b.index()) {
        return false;
    }

    return std::visit(
        maxlang::match {
            [](std::monostate, std::monostate) -> bool { return true; },
            [](int a, int b) -> bool { return a == b; },
            [](double a, double b) -> bool { return a == b; },
            [](const std::string& a, const std::string& b) -> bool { return a == b; },
            [](char a, char b) -> bool { return a == b; },
            [](auto&&, auto&&) -> bool { return false; } // разные типы
        },
        a, b);
}

// Реализация оператора != для Value
bool maxlang::operator!=(const Value& a, const Value& b) {
    return !(a == b);
}