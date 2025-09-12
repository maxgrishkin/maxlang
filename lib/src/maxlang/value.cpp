#include <iostream>
#include "value.h"
#include "util.h"

namespace {
    void impl(std::ostream& os, const maxlang::Value& value) {
        std::visit(
            maxlang::match {
              [&](std::monostate) { os << "<void>"; },
              [&](int v) { os << v; },
              [&](const std::string& v) { os << v; },
                [&](char c) { os << c; },
              // Array больше не обрабатываем здесь
            },
            value);
    }
}   // namespace

std::ostream& maxlang::operator<<(std::ostream& os, const maxlang::Value& value) {
    impl(os, value);
    return os;
}