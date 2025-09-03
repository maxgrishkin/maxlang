#pragma once

#include <utility>
#include <memory>
#include <map>
#include <functional>
#include "array.h"
#include "context.h"
#include "fmt/format.h"
#include "util.h"

namespace maxlang::expression {
    struct ArrayIndex;
    struct ArrayCreation;
    struct While;

    struct Base {
        virtual ~Base() = default;

        virtual Value evaluate(Context& context) = 0;
    };

    using CommandSequence = std::vector<std::unique_ptr<maxlang::expression::Base>>;

    void execute(const CommandSequence& commands, Context& context);

    struct Constant : Base {
        explicit Constant(Value value) : value(std::move(value)) {}
        ~Constant() override = default;

        Value value;
        Value evaluate(Context& context) override { return value; }
    };

    template <std::invocable<int, int> Op>
    struct Binary : Base {
        Binary(std::unique_ptr<expression::Base> lhs, std::unique_ptr<expression::Base> rhs)
          : lhs(std::move(lhs)), rhs(std::move(rhs)) {}
        ~Binary() override = default;

        std::unique_ptr<expression::Base> lhs;
        std::unique_ptr<expression::Base> rhs;

        Value evaluate(Context& context) override {
            return std::visit(
                maxlang::match {
                  [](auto&& lhs, auto&& rhs) -> Value {
                      if constexpr (requires { Op{}(lhs, rhs); }) {
                          return Op{}(lhs, rhs);
                      }
                      throw std::runtime_error(fmt::format("Can't perform {} on {} and {}", typeid(Op).name(), typeid(lhs).name(), typeid(rhs).name()));
                  },
                },
                lhs->evaluate(context), rhs->evaluate(context));
        }
    };

    struct VariableAssignment : Base {
        VariableAssignment(std::string name, std::unique_ptr<expression::Base> value)
          : name(std::move(name)), value(std::move(value)) {}
        ~VariableAssignment() override = default;
        std::string name;
        std::unique_ptr<expression::Base> value;

        Value evaluate(Context& context) override { return context.variables[name] = value->evaluate(context); }
    };

    struct FunctionCall : Base {
        FunctionCall(std::string name, std::vector<std::unique_ptr<expression::Base>> args)
          : name(std::move(name)), args(std::move(args)) {}
        ~FunctionCall() override = default;
        std::string name;
        std::vector<std::unique_ptr<expression::Base>> args;

        Value evaluate(Context& context) override;
    };

    struct VariableReference : Base {
        explicit VariableReference(std::string name) : name(std::move(name)) {}
        ~VariableReference() override = default;
        std::string name;
        Value evaluate(Context& context) override {
            auto it = context.variables.find(name);
            if (it == context.variables.end()) {
                throw std::runtime_error("Variable not found: " + name);
            }
            return it->second;
        }
    };

    struct If : Base {
        If(std::unique_ptr<expression::Base> condition, CommandSequence body)
          : condition(std::move(condition)), body(std::move(body)) {}
        ~If() override = default;

        std::unique_ptr<expression::Base> condition;
        CommandSequence body;

        Value evaluate(Context& context) override {
            if (getIntFromValue(condition->evaluate(context), "if condition") != 0) {
                expression::execute(body, context);
            }
            return std::monostate {};
        }
    };

    struct Return : Base {
        explicit Return(std::unique_ptr<expression::Base> expression) : expression(std::move(expression)) {}
        ~Return() override = default;

        std::unique_ptr<expression::Base> expression;

        Value evaluate(Context& context) override {
            context.returnValue = expression != nullptr ? expression->evaluate(context) : std::monostate {};
            context.shouldReturn = true;
            return std::monostate {};
        }
    };

    struct While : maxlang::expression::Base {
        While(std::unique_ptr<Base> condition, maxlang::expression::CommandSequence body)
          : condition(std::move(condition)), body(std::move(body)) {}
        ~While() override = default;

        std::unique_ptr<Base> condition;
        maxlang::expression::CommandSequence body;

        Value evaluate(Context& context) override {
            context.resetLoopControls();

            while (true) {
                // Проверяем break ДО условия
                if (context.shouldBreak) break;

                // Проверяем continue - пропускаем проверку условия и тело
                if (context.shouldContinue) {
                    context.shouldContinue = false;
                    continue; // Переходим к следующей итерации
                }

                Value condValue = condition->evaluate(context);
                if (getIntFromValue(condValue, "условии while") == 0) {
                    break;
                }

                execute(body, context);

                if (context.shouldReturn) break;
                if (context.shouldBreak) break;
            }

            context.resetLoopControls();
            return std::monostate{};
        }
    };
    struct For : Base {
        For(std::unique_ptr<Base> initialization,
            std::unique_ptr<Base> condition,
            std::unique_ptr<Base> increment,
            CommandSequence body)
            : initialization(std::move(initialization)),
              condition(std::move(condition)),
              increment(std::move(increment)),
              body(std::move(body)) {}
        ~For() override = default;

        std::unique_ptr<Base> initialization;
        std::unique_ptr<Base> condition;
        std::unique_ptr<Base> increment;
        CommandSequence body;

        Value evaluate(Context& context) override {
            context.resetLoopControls();

            if (initialization) {
                initialization->evaluate(context);
            }

            while (true) {
                // Проверяем break ДО условия
                if (context.shouldBreak) break;

                // Проверяем continue - пропускаем оставшуюся часть итерации
                if (context.shouldContinue) {
                    context.shouldContinue = false;
                    goto increment; // Переходим прямо к инкременту
                }

                if (condition) {
                    Value condValue = condition->evaluate(context);
                    if (getIntFromValue(condValue, "условии for") == 0) {
                        break;
                    }
                }

                execute(body, context);

                if (context.shouldReturn) break;
                if (context.shouldBreak) break;

                increment:
                    if (increment) {
                        increment->evaluate(context);
                    }
            }

            context.resetLoopControls();
            return std::monostate{};
        }
    };

    struct ArrayCreation : Base {
        ArrayCreation(std::vector<std::unique_ptr<expression::Base>> elements, std::string arrayName = "")
            : elements(std::move(elements)), arrayName(std::move(arrayName)) {}
        ~ArrayCreation() override = default;

        std::vector<std::unique_ptr<expression::Base>> elements;
        std::string arrayName;

        Value evaluate(Context& context) override {
            auto array = std::make_shared<Array>();
            for (const auto& element : elements) {
                array->elements.push_back(element->evaluate(context));
            }

            if (arrayName.empty()) {
                arrayName = "__array_" + std::to_string(reinterpret_cast<uintptr_t>(array.get()));
            }

            context.arrays[arrayName] = array;

            return arrayName; // Возвращаем имя массива как строку
        }
    };

    struct ArrayIndex : Base {
        ArrayIndex(std::unique_ptr<expression::Base> array, std::unique_ptr<expression::Base> index)
            : array(std::move(array)), index(std::move(index)) {}
        ~ArrayIndex() override = default;

        std::unique_ptr<expression::Base> array;
        std::unique_ptr<expression::Base> index;

        Value evaluate(Context& context) override {
            Value arrayNameValue = array->evaluate(context);
            Value indexValue = index->evaluate(context);

            if (!std::holds_alternative<std::string>(arrayNameValue)) {
                throw std::runtime_error("Expected array name (string) for indexing");
            }
            if (!std::holds_alternative<int>(indexValue)) {
                throw std::runtime_error("Expected integer index");
            }

            std::string arrayName = std::get<std::string>(arrayNameValue);
            int idx = std::get<int>(indexValue);

            auto it = context.arrays.find(arrayName);
            if (it == context.arrays.end()) {
                throw std::runtime_error("Array not found: " + arrayName);
            }

            const auto& arr = *it->second;

            if (idx < 0 || idx >= static_cast<int>(arr.elements.size())) {
                throw std::runtime_error(fmt::format("Array index {} out of bounds [0, {})",
                    idx, arr.elements.size()));
            }

            return arr.elements[idx];
        }
    };

    struct ArrayAssignment : Base {
        ArrayAssignment(std::unique_ptr<expression::Base> array,
                       std::unique_ptr<expression::Base> index,
                       std::unique_ptr<expression::Base> value)
            : array(std::move(array)), index(std::move(index)), value(std::move(value)) {}
        ~ArrayAssignment() override = default;

        std::unique_ptr<expression::Base> array;
        std::unique_ptr<expression::Base> index;
        std::unique_ptr<expression::Base> value;

        Value evaluate(Context& context) override {
            Value arrayNameValue = array->evaluate(context);
            Value indexValue = index->evaluate(context);
            Value newValue = value->evaluate(context);

            if (!std::holds_alternative<std::string>(arrayNameValue)) {
                throw std::runtime_error("Expected array name (string) for indexing");
            }
            if (!std::holds_alternative<int>(indexValue)) {
                throw std::runtime_error("Expected integer index");
            }

            std::string arrayName = std::get<std::string>(arrayNameValue);
            int idx = std::get<int>(indexValue);

            // Находим массив в контексте
            auto it = context.arrays.find(arrayName);
            if (it == context.arrays.end()) {
                throw std::runtime_error("Array not found: " + arrayName);
            }

            auto& arr = *it->second;
            if (idx < 0 || idx >= static_cast<int>(arr.elements.size())) {
                throw std::runtime_error(fmt::format("Array index {} out of bounds [0, {})", idx, arr.elements.size()));
            }

            arr.elements[idx] = newValue;
            return newValue;
        }
    };
    struct PostfixIncrement : Base {
    PostfixIncrement(std::unique_ptr<expression::Base> operand)
        : operand(std::move(operand)) {}
    ~PostfixIncrement() override = default;

    std::unique_ptr<expression::Base> operand;

    Value evaluate(Context& context) override {
        // Получаем текущее значение переменной или элемента массива
        Value currentValue = operand->evaluate(context);

        if (!std::holds_alternative<int>(currentValue)) {
            throw std::runtime_error("Postfix increment can only be applied to integers");
        }

        int currentInt = std::get<int>(currentValue);
        int newValue = currentInt + 1;

        // Обновляем значение в контексте
        if (auto variableRef = dynamic_cast<VariableReference*>(operand.get())) {
            // Для переменной
            context.variables[variableRef->name] = newValue;
        }
        else if (auto arrayIndex = dynamic_cast<ArrayIndex*>(operand.get())) {
            // Для элемента массива
            Value arrayNameValue = arrayIndex->array->evaluate(context);
            Value indexValue = arrayIndex->index->evaluate(context);

            if (!std::holds_alternative<std::string>(arrayNameValue)) {
                throw std::runtime_error("Expected array name for array element increment");
            }
            if (!std::holds_alternative<int>(indexValue)) {
                throw std::runtime_error("Expected integer index for array element increment");
            }

            std::string arrayName = std::get<std::string>(arrayNameValue);
            int idx = std::get<int>(indexValue);

            auto it = context.arrays.find(arrayName);
            if (it == context.arrays.end()) {
                throw std::runtime_error("Array not found: " + arrayName);
            }

            it->second->elements[idx] = newValue;
        }
        else {
            throw std::runtime_error("Postfix increment can only be applied to variables or array elements");
        }

        // Возвращаем старое значение (постфиксная семантика)
        return currentInt;
    }
    };
    struct PostfixDecrement : Base {
        PostfixDecrement(std::unique_ptr<expression::Base> operand)
            : operand(std::move(operand)) {}
        ~PostfixDecrement() override = default;

        std::unique_ptr<expression::Base> operand;

        Value evaluate(Context& context) override {
            // Получаем текущее значение переменной или элемента массива
            Value currentValue = operand->evaluate(context);

            if (!std::holds_alternative<int>(currentValue)) {
                throw std::runtime_error("Postfix increment can only be applied to integers");
            }

            int currentInt = std::get<int>(currentValue);
            int newValue = currentInt - 1;

            // Обновляем значение в контексте
            if (auto variableRef = dynamic_cast<VariableReference*>(operand.get())) {
                // Для переменной
                context.variables[variableRef->name] = newValue;
            }
            else if (auto arrayIndex = dynamic_cast<ArrayIndex*>(operand.get())) {
                // Для элемента массива
                Value arrayNameValue = arrayIndex->array->evaluate(context);
                Value indexValue = arrayIndex->index->evaluate(context);

                if (!std::holds_alternative<std::string>(arrayNameValue)) {
                    throw std::runtime_error("Expected array name for array element increment");
                }
                if (!std::holds_alternative<int>(indexValue)) {
                    throw std::runtime_error("Expected integer index for array element increment");
                }

                std::string arrayName = std::get<std::string>(arrayNameValue);
                int idx = std::get<int>(indexValue);

                auto it = context.arrays.find(arrayName);
                if (it == context.arrays.end()) {
                    throw std::runtime_error("Array not found: " + arrayName);
                }

                it->second->elements[idx] = newValue;
            }
            else {
                throw std::runtime_error("Postfix increment can only be applied to variables or array elements");
            }

            // Возвращаем старое значение (постфиксная семантика)
            return currentInt;
        }
    };
    struct ForEach : Base {
        ForEach(std::string variableName,
                std::unique_ptr<Base> collection,
                CommandSequence body)
            : variableName(std::move(variableName)),
              collection(std::move(collection)),
              body(std::move(body)) {}
        ~ForEach() override = default;

        std::string variableName;
        std::unique_ptr<Base> collection;
        CommandSequence body;

        Value evaluate(Context& context) override {
            context.resetLoopControls();

            Value collectionValue = collection->evaluate(context);

            if (!std::holds_alternative<std::string>(collectionValue)) {
                throw std::runtime_error("Foreach expects array name");
            }

            std::string arrayName = std::get<std::string>(collectionValue);
            auto it = context.arrays.find(arrayName);
            if (it == context.arrays.end()) {
                throw std::runtime_error("Array not found: " + arrayName);
            }

            for (const auto& element : it->second->elements) {
                // Проверяем break ДО обработки элемента
                if (context.shouldBreak) break;

                // Проверяем continue - пропускаем этот элемент
                if (context.shouldContinue) {
                    context.shouldContinue = false;
                    continue; // Переходим к следующему элементу
                }

                context.variables[variableName] = element;
                execute(body, context);

                if (context.shouldReturn) break;
                if (context.shouldBreak) break;
            }

            context.resetLoopControls();
            return std::monostate{};
        }
    };
    struct Break : Base {
        Break() = default;
        ~Break() override = default;

        Value evaluate(Context& context) override {
            context.shouldBreak = true;
            return std::monostate{};
        }
    };

    struct Continue : Base {
        Continue() = default;
        ~Continue() override = default;

        Value evaluate(Context& context) override {
            context.shouldContinue = true;
            return std::monostate{};
        }
    };
    struct Function {
        std::vector<std::string> parameters; // Имена параметров
        expression::CommandSequence body;    // Тело функции
        std::function<Value(Context& context, std::vector<Value> args)> value;

        // Конструктор для пользовательских функций
        Function(std::vector<std::string> params, expression::CommandSequence body)
            : parameters(std::move(params)), body(std::move(body)) {
            value = [&](Context& context, std::vector<Value> args) -> Value {
                if (args.size() != parameters.size()) {
                    throw std::runtime_error(fmt::format(
                        "Function expects {} arguments, got {}", parameters.size(), args.size()));
                }

                // Сохраняем текущие переменные
                auto oldVariables = std::move(context.variables);
                context.variables.clear();

                // Устанавливаем аргументы
                for (size_t i = 0; i < parameters.size(); ++i) {
                    context.variables[parameters[i]] = args[i];
                }

                // Выполняем тело функции
                context.resetReturn();
                expression::execute(body, context);

                // Восстанавливаем переменные
                auto result = context.returnValue.value_or(std::monostate{});
                context.variables = std::move(oldVariables);
                context.resetReturn();

                return result;
            };
        }

        // Конструктор для встроенных функций
        Function(std::function<Value(Context&, std::vector<Value>)> func)
            : value(std::move(func)) {}
    };
    struct FunctionDeclaration : Base {
        FunctionDeclaration(std::string name,
                           std::vector<std::string> parameters,
                           CommandSequence body)
            : name(std::move(name)),
              parameters(std::move(parameters)),
              body(std::move(body)) {}

        ~FunctionDeclaration() override = default;

        std::string name;
        std::vector<std::string> parameters;
        CommandSequence body;

        Value evaluate(Context& context) override;
    };
    struct IfElse : Base {
        IfElse(std::unique_ptr<expression::Base> condition,
               CommandSequence ifBody,
               CommandSequence elseBody)
            : condition(std::move(condition)),
              ifBody(std::move(ifBody)),
              elseBody(std::move(elseBody)) {}
        ~IfElse() override = default;

        std::unique_ptr<expression::Base> condition;
        CommandSequence ifBody;
        CommandSequence elseBody;

        Value evaluate(Context& context) override {
            if (getIntFromValue(condition->evaluate(context), "if-else condition") != 0) {
                expression::execute(ifBody, context);
            } else {
                expression::execute(elseBody, context);
            }
            return std::monostate {};
        }
    };

}