#include <map>
#include <string>
#include <memory>
#include "value.h"
#include "function.h"
#include <optional>

namespace maxlang {
    struct Array; // Предварительное объявление

    struct Context {
        std::map<std::string, Function> functions;
        std::map<std::string, Value> variables;
        std::map<std::string, std::shared_ptr<Array>> arrays; // Хранилище массивов
        std::optional<Value> returnValue;
        bool shouldReturn = false;
        bool shouldBreak = false;
        bool shouldContinue = false;

        void resetReturn() {
            shouldReturn = false;
            returnValue = std::monostate{};
        }

        void resetLoopControls() {   // ← добавить
            shouldBreak = false;
            shouldContinue = false;
        }
    };
}