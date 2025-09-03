#include "util.h"
#include "value.h"

namespace maxlang {

    int getIntFromValue(const Value& value, const std::string& context) {
        return std::visit(
            match {
                [](int v) { return v; },
                [&](const std::string&) -> int {
                    throw std::runtime_error("Ожидалось целое число" +
                        (context.empty() ? "" : " в " + context));
                },
                [&](std::monostate) -> int {
                    throw std::runtime_error("Ожидалось целое число" +
                        (context.empty() ? "" : " в " + context));
                }
            },
            value
        );
    }

}