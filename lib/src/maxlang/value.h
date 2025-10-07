#include <variant>
#include <string>
#include "array.h"

namespace maxlang {
    using Value = std::variant<std::monostate /* aka void */, int, std::string, char>;
    std::ostream& operator<<(std::ostream& os, const Value& value);
}