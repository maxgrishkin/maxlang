#include "expression.h"
#include <ranges>

maxlang::Value maxlang::expression::FunctionCall::evaluate(Context& context) {
    auto it = context.functions.find(name);
    if (it == context.functions.end()) {
        throw std::runtime_error(fmt::format("Function not found: {}", name));
    }
    /*
    if (it->second.args.size() != args.size()) {
        throw std::runtime_error(fmt::format(
            "Function {} expects {} arguments, but {} were provided", name, it->second.args.size(), args.size()));
    }
    auto prevVariables = std::move(context.variables);
    for (std::size_t i = 0; i < args.size(); ++i) {
        context.variables[it->second.args[i].name] = args[i]->evaluate(context);
    }
    */
    auto evaluatedArgs = args | std::views::transform([&](auto& arg) { return arg->evaluate(context); });
    auto result = it->second.value(context, {evaluatedArgs.begin(), evaluatedArgs.end()});
//        context.variables = std::move(prevVariables);
    return result;
}

void maxlang::expression::execute(const CommandSequence& commands, Context& context) {
    for (const auto& command : commands) {
        command->evaluate(context);

        // Проверяем флаги управления потоком
        if (context.returnValue.has_value()) return;
        if (context.shouldReturn) break;
        if (context.shouldBreak) break;
        if (context.shouldContinue) continue; // continue тоже прерывает выполнение блока
    }
}