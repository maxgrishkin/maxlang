#include <iostream>
#include "stdlib.h"
#include <cstdlib>
#include "value.h"
#include "util.h"

using namespace maxlang;

namespace {
    maxlang::Value println(maxlang::Context& state, const std::vector<maxlang::Value>& args) {
        for (auto& arg : args) {
            std::cout << arg;
        }
        std::cout << std::endl;
        return std::monostate();
    }
    maxlang::Value print(maxlang::Context& state, const std::vector<maxlang::Value>& args) {
        for (auto& arg : args) {
            std::cout << arg;
        }
        return std::monostate();
    }
    maxlang::Value input(maxlang::Context& state, const std::vector<maxlang::Value>& args) {
        std::string str;
        std::cin >> str;
        return str;
    }

    maxlang::Value toint(maxlang::Context& state, const std::vector<maxlang::Value>& args) {
        if (args.size() != 1) {
            throw std::runtime_error("toint expects 1 argument");
        }
        return std::visit(
            maxlang::match {
              [](auto&& v) -> int { return int(v); },
              [](const std::string& s) -> int {
                  return std::stoi(s);
              },
              [](std::monostate) -> int { throw std::runtime_error("toint: can't convert void to int"); },
            },
            args[0]);
    }
    maxlang::Value array_length(maxlang::Context& state, const std::vector<maxlang::Value>& args) {
        if (args.size() != 1) {
            throw std::runtime_error("array_length expects 1 argument");
        }

        if (!std::holds_alternative<std::string>(args[0])) {
            throw std::runtime_error("array_length: expected array name (string)");
        }

        std::string arrayName = std::get<std::string>(args[0]);
        auto it = state.arrays.find(arrayName);
        if (it == state.arrays.end()) {
            throw std::runtime_error("Array not found: " + arrayName);
        }

        return static_cast<int>(it->second->elements.size());
    }

    maxlang::Value array_push(maxlang::Context& state, const std::vector<maxlang::Value>& args) {
        if (args.size() < 2) {
            throw std::runtime_error("array_push expects at least 2 arguments");
        }

        if (!std::holds_alternative<std::string>(args[0])) {
            throw std::runtime_error("array_push: expected array name (string) as first argument");
        }

        std::string arrayName = std::get<std::string>(args[0]);
        auto it = state.arrays.find(arrayName);
        if (it == state.arrays.end()) {
            throw std::runtime_error("Array not found: " + arrayName);
        }

        for (size_t i = 1; i < args.size(); ++i) {
            it->second->elements.push_back(args[i]);
        }

        return static_cast<int>(it->second->elements.size());
    }

    maxlang::Value array_pop(maxlang::Context& state, const std::vector<maxlang::Value>& args) {
        if (args.size() != 1) {
            throw std::runtime_error("array_pop expects 1 argument");
        }

        if (!std::holds_alternative<std::string>(args[0])) {
            throw std::runtime_error("array_pop: expected array name (string)");
        }

        std::string arrayName = std::get<std::string>(args[0]);
        auto it = state.arrays.find(arrayName);
        if (it == state.arrays.end()) {
            throw std::runtime_error("Array not found: " + arrayName);
        }

        if (it->second->elements.empty()) {
            throw std::runtime_error("array_pop: cannot pop from empty array");
        }

        auto value = it->second->elements.back();
        it->second->elements.pop_back();
        return value;
    }
    maxlang::Value endl = "\n";
}

void maxlang::stdlib::init(maxlang::State& state) {
#define DEFINE(name) state.context().functions[#name] = { name }

    DEFINE(println);
    DEFINE(print);
    DEFINE(input);
    DEFINE(toint);
    DEFINE(array_length);
    DEFINE(array_push);
    DEFINE(array_pop);
    state.context().variables["endl"] = { endl };
}
