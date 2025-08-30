#include "expression.h"
#include "function.h"
#include <ranges>

namespace maxlang::expression {

    Value FunctionDeclaration::evaluate(Context& context) {
        // Используем shared_ptr для разделяемого владения
        auto params_ptr = std::make_shared<std::vector<std::string>>(std::move(parameters));
        auto body_ptr = std::make_shared<CommandSequence>(std::move(body));

        auto wrapper = [params_ptr, body_ptr]
                       (Context& ctx, std::vector<Value> args) -> Value {
            if (args.size() != params_ptr->size()) {
                throw std::runtime_error(fmt::format(
                    "Function expects {} arguments, got {}", params_ptr->size(), args.size()));
            }

            // Сохраняем текущие переменные
            auto oldVariables = std::move(ctx.variables);
            ctx.variables.clear();

            // Устанавливаем аргументы
            for (size_t i = 0; i < params_ptr->size(); ++i) {
                ctx.variables[(*params_ptr)[i]] = args[i];
            }

            // Выполняем тело функции
            ctx.resetReturn();
            execute(*body_ptr, ctx);

            // Восстанавливаем переменные
            auto result = ctx.returnValue.value_or(std::monostate{});
            ctx.variables = std::move(oldVariables);
            ctx.resetReturn();

            return result;
        };

        context.functions[name] = ::maxlang::Function(std::move(wrapper));
        return std::monostate{};
    }

Value FunctionCall::evaluate(Context& context) {
    auto it = context.functions.find(name);
    if (it == context.functions.end()) {
        throw std::runtime_error(fmt::format("Function not found: {}", name));
    }

    std::vector<Value> evaluatedArgs;
    for (const auto& arg : args) {
        evaluatedArgs.push_back(arg->evaluate(context));
    }

    return it->second.nativeFunction(context, std::move(evaluatedArgs));
}

// Убедитесь, что остальной код соответствует новой реализации
void execute(const CommandSequence& commands, Context& context) {
    for (const auto& command : commands) {
        command->evaluate(context);

        if (context.returnValue.has_value()) return;
        if (context.shouldReturn) break;
        if (context.shouldBreak) break;
        if (context.shouldContinue) continue;
    }
}

}