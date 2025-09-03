#pragma once
#include "value.h"
#include <string>

namespace maxlang {

    template <typename... Lambdas>
    struct match : Lambdas... {
        using Lambdas::operator()...;
    };

    template <typename... Lambdas>
    match(Lambdas...) -> match<Lambdas...>;

    // Только объявление
    int getIntFromValue(const Value& value, const std::string& context = "");

}