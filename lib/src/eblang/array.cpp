#include "array.h"
#include "util.h"
#include <iostream>
/*werwerwer*/
namespace maxlang {

    namespace {
        // Вспомогательная функция для вывода Value
        void printValue(std::ostream& os, const Value& value) {
            std::visit(
                match {
                    [&](std::monostate) { os << "<void>"; },
                    [&](int v) { os << v; },
                    [&](const std::string& v) { os << v; },
                },
                value);
        }

        void impl_array(std::ostream& os, const Array& array) {
            os << "[";
            for (size_t i = 0; i < array.elements.size(); ++i) {
                printValue(os, array.elements[i]); // Используем нашу функцию
                if (i != array.elements.size() - 1) {
                    os << ", ";
                }
            }
            os << "]";
        }
    }   // namespace

    std::ostream& operator<<(std::ostream& os, const Array& array) {
        impl_array(os, array);
        return os;
    }

    void printArray(std::ostream& os, const Array& array) {
        os << "Array '" << array.name << "': [";
        for (size_t i = 0; i < array.elements.size(); ++i) {
            printValue(os, array.elements[i]); // Используем нашу функцию
            if (i != array.elements.size() - 1) {
                os << ", ";
            }
        }
        os << "]";
    }
}