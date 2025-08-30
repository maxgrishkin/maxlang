#pragma once

#include <functional>
#include <vector>
#include <string>
#include "value.h"

namespace maxlang {

    struct Context;

    struct Function {
        std::function<Value(Context& context, std::vector<Value> args)> nativeFunction;

        Function() = default;

        Function(std::function<Value(Context&, std::vector<Value>)> func)
            : nativeFunction(std::move(func)) {}

        Value operator()(Context& context, std::vector<Value> args) {
            return nativeFunction(context, std::move(args));
        }
    };
}