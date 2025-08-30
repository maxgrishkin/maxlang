#pragma once

#include <functional>
#include "value.h"

namespace maxlang {

struct Context;

struct Function {
    std::function<Value(Context& context, std::vector<Value> args)> value;
};
}