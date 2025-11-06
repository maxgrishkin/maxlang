#include "util.h"

#include <stdexcept>

#include "value.h"

namespace maxlang {

    int getIntFromValue(const Value& value, const std::string& context) {
        return std::visit(
            match {
                [](int v) -> int { return v; },
                [](double v) -> int { return static_cast<int>(v); },
                [&](char c) -> int {
                    throw std::runtime_error("Ожидалось целое число" +
                        (context.empty() ? "" : " в " + context));
                },
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

    double getDoubleFromValue(const Value& value, const std::string& context) {
        return std::visit(
            match {
                [](int v) -> double { return static_cast<double>(v); },
                [](double v) -> double { return v; },
                [&](char c) -> double {
                    throw std::runtime_error("Ожидалось число с плавающей точкой" +
                        (context.empty() ? "" : " в " + context));
                },
                [&](const std::string&) -> double {
                    throw std::runtime_error("Ожидалось число с плавающей точкой" +
                        (context.empty() ? "" : " в " + context));
                },
                [&](std::monostate) -> double {
                    throw std::runtime_error("Ожидалось число с плавающей точкой" +
                        (context.empty() ? "" : " в " + context));
                }
            },
            value
        );
    }

}