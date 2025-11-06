#pragma once

#include <variant>
#include <string>
// УБЕРИТЕ: #include "array.h"

namespace maxlang {
    // Предварительное объявление вместо включения
    struct Array;

    using Value = std::variant<std::monostate /* aka void */, int, double, std::string, char>;
    std::ostream& operator<<(std::ostream& os, const Value& value);

    // Объявления операторов сравнения
    bool operator==(const Value& a, const Value& b);
    bool operator!=(const Value& a, const Value& b);
}