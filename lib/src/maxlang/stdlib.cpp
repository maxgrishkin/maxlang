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

    maxlang::Value Round(maxlang::Context& state, const std::vector<maxlang::Value>& args);

    maxlang::Value toInt(maxlang::Context& state, const std::vector<maxlang::Value>& args) {
        if (args.size() != 1) {
            throw std::runtime_error("toint expects 1 argument");
        }
        return std::visit(
            maxlang::match {
                [](const std::string& s) -> int {
                    return std::stoi(s);
                },
                [](const char& c) -> int {
                    return std::stoi(std::to_string(c)) - 48;
                },
                [&](double v) -> int {
                    std::vector<Value> V = {v};
                    return getIntFromValue(Round(state,V));
                },
                [](std::monostate) -> int { throw std::runtime_error("toint: can't convert void to int"); },
                [](auto&& v) -> int { return int(v); },
            },
            args[0]);
    }

    maxlang::Value toDouble(maxlang::Context& state, const std::vector<maxlang::Value>& args) {
        if (args.size() != 1) {
            throw std::runtime_error("todouble expects 1 argument");
        }
        return std::visit(
            maxlang::match {
                [](const std::string& s) -> double {
                    return std::stod(s);
                },
                [](const char& c) -> double {
                    return static_cast<double>(c);
                },
                [](int v) -> double { return static_cast<double>(v); },
                [](std::monostate) -> double { throw std::runtime_error("todouble: can't convert void to double"); },
                [](auto&& v) -> double { return static_cast<double>(v); },
            },
            args[0]);
    }

    maxlang::Value toString(maxlang::Context& state, const std::vector<maxlang::Value>& args) {
        if (args.size() != 1) {
            throw std::runtime_error("tostring expects 1 argument");
        }
        return std::visit(
            maxlang::match {
                [](const std::string& s) -> std::string {
                    return s;
                },
                [](const char& c) -> std::string {
                    return std::string(1, c);
                },
                [](int v) -> std::string { return std::to_string(v); },
                [](double v) -> std::string { return std::to_string(v); },
                [](std::monostate) -> std::string { return "<void>"; },
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


    maxlang::Value e = 2.71828;
    maxlang::Value pi = 3.14159;

    // Ваша реализация Abc
    maxlang::Value Abc(maxlang::Context& state, const std::vector<maxlang::Value>& args) {
        if (args.size() != 1) {
            throw std::runtime_error("Abc expects 1 argument");
        }
        return std::visit(
            maxlang::match {
                [](double d) -> maxlang::Value {
                    if (d < 0) {
                        return -d;
                    }
                    return d;
                },
                [](int i) -> maxlang::Value {
                    if (i < 0) {
                        return -i;
                    }
                    return i;
                },
                [](auto&& other) -> maxlang::Value {
                    throw std::runtime_error("Abc: expected number type");
                },
            },
            args[0]);
    }

    // Ваша реализация Factorial
    maxlang::Value Factorial(maxlang::Context& state, const std::vector<maxlang::Value>& args) {
        if (args.size() != 1) {
            throw std::runtime_error("Factorial expects 1 argument");
        }

        int n = getIntFromValue(args[0], "Factorial");
        if (n < 0) {
            throw std::runtime_error("Factorial: argument must be non-negative");
        }
        if (n == 0 || n == 1) {
            return 1;
        }

        long result = 1;
        for (int i = 2; i <= n; ++i) {
            result *= i;
        }
        return static_cast<int>(result);
    }

    maxlang::Value Ln(maxlang::Context& state, const std::vector<maxlang::Value>& args);

    // Ваша реализация Pow
    maxlang::Value Pow(maxlang::Context& state, const std::vector<maxlang::Value>& args) {
        if (args.size() != 2) {
            throw std::runtime_error("Pow expects 2 arguments");
        }

        double a = getDoubleFromValue(args[0], "Pow base");
        double b = getDoubleFromValue(args[1], "Pow exponent");

        if (b == 0) return 1.0;
        if (a == 0) return 0.0;
        if (a == 1) return 1.0;

        // Для целых положительных степеней
        if (b == static_cast<int>(b) && b > 0) {
            int intB = static_cast<int>(b);
            double r = a;
            for (int i = 1; i < intB; i++) {
                r *= a;
            }
            return r;
        }

        // Для целых отрицательных степеней
        if (b == static_cast<int>(b) && b < 0) {
            int intB = static_cast<int>(b);
            double r = a;
            for (int i = 1; i < -intB; i++) {
                r *= a;
            }
            return 1.0 / r;
        }

        // Для нецелых степеней используем вашу реализацию с Ln
        if (a > 0) {
            double e = 2.71828;
            double ln_result = 0.0;
            double acuracity = 0.000000001;
            int step = 1;
            int result_int = 1;

            // Ваша реализация Ln
            while (getDoubleFromValue(Abc(state, {getDoubleFromValue(Pow(state, {e, static_cast<double>(result_int)}),"Pow") - a})) > acuracity) {
                if (getDoubleFromValue(Pow(state, {getDoubleFromValue(e,"Pow"), static_cast<double>(result_int)}),"Pow") < a) {
                    result_int += step;
                    if (getDoubleFromValue(Pow(state, {getDoubleFromValue(e,"Pow"), static_cast<double>(result_int)}),"Pow") > a) {
                        step /= 2;
                    }
                }
                if (getDoubleFromValue(Pow(state, {getDoubleFromValue(e,"Pow"), static_cast<double>(result_int)}),"Pow") > a) {
                    result_int -= step;
                    if (getDoubleFromValue(Pow(state, {getDoubleFromValue(e,"Pow"), static_cast<double>(result_int)}),"Pow") < a) {
                        step /= 2;
                    }
                }
            }
            ln_result = result_int;

            return Pow(state, {e, b * ln_result});
        }

        // Для отрицательных оснований
        return getDoubleFromValue(Pow(state, {e, b * getDoubleFromValue(Ln(state, {Abc(state, {a})}),"Pow")}),"Pow") * (b == static_cast<int>(b) ? 1 : -1);
    }

    // Ваша реализация Sqr
    maxlang::Value Sqr(maxlang::Context& state, const std::vector<maxlang::Value>& args) {
        if (args.size() != 1) {
            throw std::runtime_error("Sqr expects 1 argument");
        }

        double a = getDoubleFromValue(args[0], "Sqr");
        return a * a;
    }

    // Ваша реализация isSimple
    maxlang::Value isSimple(maxlang::Context& state, const std::vector<maxlang::Value>& args) {
        if (args.size() != 1) {
            throw std::runtime_error("isSimple expects 1 argument");
        }

        long a = getIntFromValue(args[0], "isSimple");
        for (int i = 2; i < a; i++) {
            if ((a % i) == 0) {
                return 0; // false
            }
        }
        return 1; // true
    }

    // Ваша реализация Root
    maxlang::Value Root(maxlang::Context& state, const std::vector<maxlang::Value>& args) {
        if (args.size() != 2) {
            throw std::runtime_error("Root expects 2 arguments");
        }

        double a = getDoubleFromValue(args[0], "Root value");
        int b = getIntFromValue(args[1], "Root degree");

        if (b == 1) {
            return a;
        }

        double acuracity = 0.000000001;
        double step = a / 2;
        double result = step;

        while (getDoubleFromValue(Abc(state, {getDoubleFromValue(Pow(state, {result, static_cast<double>(b)}),"Root") - a}),"Root") > acuracity) {
            if (getDoubleFromValue(Pow(state, {result, static_cast<double>(b)}),"Root") < a) {
                result += step;
                if (getDoubleFromValue(Pow(state, {result, static_cast<double>(b)}),"Root") > a) {
                    step /= 2;
                }
            }
            if (getDoubleFromValue(Pow(state, {result, static_cast<double>(b)}),"Root") > a) {
                result -= step;
                if (getDoubleFromValue(Pow(state, {result, static_cast<double>(b)}),"Root") < a) {
                    step /= 2;
                }
            }
        }
        return result;
    }

    // Ваша реализация Sqrt
    maxlang::Value Sqrt(maxlang::Context& state, const std::vector<maxlang::Value>& args) {
        if (args.size() != 1) {
            throw std::runtime_error("Sqrt expects 1 argument");
        }

        double a = getDoubleFromValue(args[0], "Sqrt");

        double acuracity = 0.000000001;
        double step = a / 2;
        double result = step;

        while (getDoubleFromValue(Abc(state, {getDoubleFromValue(Sqr(state, {result})) - a})) > acuracity) {
            if (getDoubleFromValue(Sqr(state, {result})) < a) {
                result += step;
                if (getDoubleFromValue(Sqr(state, {result})) > a) {
                    step /= 2;
                }
            }
            if (getDoubleFromValue(Sqr(state, {result})) > a) {
                result -= step;
                if (getDoubleFromValue(Sqr(state, {result})) < a) {
                    step /= 2;
                }
            }
        }
        return result;
    }

    // Ваша реализация Log
    maxlang::Value Log(maxlang::Context& state, const std::vector<maxlang::Value>& args) {
        if (args.size() != 2) {
            throw std::runtime_error("Log expects 2 arguments");
        }

        double a = getDoubleFromValue(args[0], "Log value");
        double b = getDoubleFromValue(args[1], "Log base");

        if (b <= 0 || b == 1) {
            throw std::runtime_error("Log: base must be positive and not equal to 1");
        }

        double acuracity = 0.000000001;
        int step = 1;
        int result = 1;

        while (getDoubleFromValue(Abc(state, {getDoubleFromValue(Pow(state, {b, static_cast<double>(result)})) - a})) > acuracity) {
            if (getDoubleFromValue(Pow(state, {b, static_cast<double>(result)})) < a) {
                result += step;
                if (getDoubleFromValue(Pow(state, {b, static_cast<double>(result)})) > a) {
                    step /= 2;
                }
            }
            if (getDoubleFromValue(Pow(state, {b, static_cast<double>(result)})) > a) {
                result -= step;
                if (getDoubleFromValue(Pow(state, {b, static_cast<double>(result)})) < a) {
                    step /= 2;
                }
            }
        }
        return result;
    }

    // Ваша реализация Ln
    maxlang::Value Ln(maxlang::Context& state, const std::vector<maxlang::Value>& args) {
        if (args.size() != 1) {
            throw std::runtime_error("Ln expects 1 argument");
        }

        double a = getDoubleFromValue(args[0], "Ln");
        double e = 2.71828;
        double acuracity = 0.000000001;
        int step = 1;
        int result = 1;

        while (getDoubleFromValue(Abc(state, {getDoubleFromValue(Pow(state, {e, static_cast<double>(result)})) - a})) > acuracity) {
            if (getDoubleFromValue(Pow(state, {e, static_cast<double>(result)})) < a) {
                result += step;
                if (getDoubleFromValue(Pow(state, {e, static_cast<double>(result)})) > a) {
                    step /= 2;
                }
            }
            if (getDoubleFromValue(Pow(state, {e, static_cast<double>(result)})) > a) {
                result -= step;
                if (getDoubleFromValue(Pow(state, {e, static_cast<double>(result)})) < a) {
                    step /= 2;
                }
            }
        }
        return result;
    }

    // Ваша реализация Fibonachi
    maxlang::Value Fibonachi(maxlang::Context& state, const std::vector<maxlang::Value>& args) {
        if (args.size() != 1) {
            throw std::runtime_error("Fibonachi expects 1 argument");
        }

        short num = getIntFromValue(args[0], "Fibonachi");
        if (num == 1 || num == 2) {
            return 1;
        }

        long a = 1;
        long b = 1;
        long c = a + b;
        for (int i = 3; i <= num; i++) {
            c = a + b;
            a = b;
            b = c;
        }
        return static_cast<int>(c);
    }

    // Ваша реализация Round
    maxlang::Value Round(maxlang::Context& state, const std::vector<maxlang::Value>& args) {
        if (args.size() != 1) {
            throw std::runtime_error("Round expects 1 argument");
        }

        float a = getDoubleFromValue(args[0], "Round");
        return std::round(a);
    }

    // Ваша реализация Sigmoid
    maxlang::Value Sigmoid(maxlang::Context& state, const std::vector<maxlang::Value>& args) {
        if (args.size() != 1) {
            throw std::runtime_error("Sigmoid expects 1 argument");
        }

        float a = getDoubleFromValue(args[0], "Sigmoid");
        double e = 2.71828;
        return 1.0 / (1.0 + getDoubleFromValue(Pow(state, {e, a * (-1)})));
    }

    // Константы
    maxlang::Value endl = '\n';
}

void maxlang::stdlib::init(maxlang::State& state) {
#define FUNCTION(name) state.context().functions[#name] = { name }
#define VARIABLE(name) state.context().variables[#name] = { name }

    FUNCTION(println);
    FUNCTION(print);
    FUNCTION(input);
    FUNCTION(toInt);
    FUNCTION(toDouble);
    FUNCTION(toString);

    FUNCTION(array_length);
    FUNCTION(array_push);
    FUNCTION(array_pop);

    FUNCTION(Abc);
    FUNCTION(Factorial);
    FUNCTION(Pow);
    FUNCTION(Sqr);
    FUNCTION(isSimple);
    FUNCTION(Root);
    FUNCTION(Sqrt);
    FUNCTION(Log);
    FUNCTION(Ln);
    FUNCTION(Fibonachi);
    FUNCTION(Round);
    FUNCTION(Sigmoid);

    VARIABLE(endl);
    VARIABLE(e);
    VARIABLE(pi);
    VARIABLE(true);
    VARIABLE(false);
}