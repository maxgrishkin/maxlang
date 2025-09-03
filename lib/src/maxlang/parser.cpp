#include "parser.h"
#include "expression.h"
#include "fmt/format.h"
#include "util.h"
#include <stdexcept>
#include <cassert>

std::unique_ptr<maxlang::expression::Base> maxlang::Parser::parseExpression(int leftBindingPower) {
    if (mTokens.empty()) {
        throw std::runtime_error("Unexpected end of input");
    }

    auto lhs = std::visit(
        match {
          [](token::Integer token) -> std::unique_ptr<expression::Base> {
              return std::make_unique<expression::Constant>(token.value);
          },
          [](token::String token) -> std::unique_ptr<expression::Base> {
              return std::make_unique<expression::Constant>(std::move(token.value));
          },
          [&](token::LPar token) -> std::unique_ptr<expression::Base> {
              auto lhs = parseExpression(0);
              auto n = take();
              if (!std::holds_alternative<token::RPar>(n.first)) {
                  throw std::runtime_error(fmt::format("Expected ')' to close '(', got {}, at line {}", tokenToString(n.first),n.second));
              }
              return lhs;
          },
          [&](token::Identifier identifier) -> std::unique_ptr<expression::Base> {
    // 1. variable reference
    // 2. function call
    if (std::holds_alternative<token::LPar>(peek().first)) {
        take();
        std::vector<std::unique_ptr<expression::Base>> args;
        for (;;) {
            auto n = peek();
            if (std::holds_alternative<token::RPar>(n.first)) {
                take();
                if (!args.empty()) {
                    throw std::runtime_error(fmt::format("Unexpected ')' after ',', at line {}",n.second));
                }
                break;
            }
            args.push_back(parseExpression());
            if (std::holds_alternative<token::Comma>(peek().first)) {
                take();
                continue;
            }
            if (std::holds_alternative<token::RPar>(peek().first)) {
                take();
                break;
            }
            throw std::runtime_error(
                fmt::format("Expected ',' or ')' to close argument list, got {}, at line {}", typeid(n).name(),n.second));
        }
        return std::make_unique<expression::FunctionCall>(std::move(identifier.value), std::move(args));
    }

    auto variableRef = std::make_unique<expression::VariableReference>(std::move(identifier.value));

    // Проверяем индексацию массива
    if (std::holds_alternative<token::LSquareBracket>(peek().first)) {
    take();

    auto index = parseExpression();

    if (mTokens.empty()) {
        throw std::runtime_error(fmt::format("Unexpected end of input after array index",peek().second));
    }

    if (!std::holds_alternative<token::RSquareBracket>(peek().first)) {
        throw std::runtime_error(fmt::format("Expected ']' after array index, got {}, at line {}",
            tokenToString(peek().first),peek().second));
    }
    take();

    return std::make_unique<expression::ArrayIndex>(std::move(variableRef), std::move(index));
}

    return variableRef;
},

            [&](token::LSquareBracket token) -> std::unique_ptr<expression::Base> {
    std::vector<std::unique_ptr<expression::Base>> elements;

    if (std::holds_alternative<token::RSquareBracket>(peek().first)) {
        take();
        return std::make_unique<expression::ArrayCreation>(std::move(elements), "");
    }

    while (true) {
        elements.push_back(parseExpression());

        if (mTokens.empty()) {
            throw std::runtime_error("Unexpected end of input in array literal");
        }

        if (std::holds_alternative<token::RSquareBracket>(peek().first)) {
            take();
            break;
        }

        if (std::holds_alternative<token::Comma>(peek().first)) {
            take();
            continue;
        }

        throw std::runtime_error(fmt::format("Expected ',' or ']' in array literal, got {}, at line {}",
            tokenToString(peek().first),peek().second));
    }

    return std::make_unique<expression::ArrayCreation>(std::move(elements), "");
},

          [&](auto&& token) -> std::unique_ptr<expression::Base> {
              throw std::runtime_error(fmt::format("Unexpected token: {}, at line {}", tokenToString(peek().first),peek().second));
          },
        },
        take().first);
    if (std::holds_alternative<token::LSquareBracket>(peek().first)) {
        take(); // consume '['
        auto index = parseExpression();
        auto n = take();
        if (!std::holds_alternative<token::RSquareBracket>(n.first)) {
            throw std::runtime_error(fmt::format("Expected ']' after index",n.second));
        }

        // Проверяем, является ли это присваиванием
        if (std::holds_alternative<token::Equal>(peek().first)) {
            take(); // consume '='
            auto value = parseExpression();
            return std::make_unique<expression::ArrayAssignment>(
                std::move(lhs), std::move(index), std::move(value));
        }

        return std::make_unique<expression::ArrayIndex>(std::move(lhs), std::move(index));
    }
    if (!mTokens.empty()) {
        if (std::holds_alternative<token::PlusPlus>(peek().first)) {
            take(); // consume '++'
            lhs = std::make_unique<expression::PostfixIncrement>(std::move(lhs));
        }
        if (std::holds_alternative<token::MinusMinus>(peek().first)) {
            take(); // consume '--'
            lhs = std::make_unique<expression::PostfixDecrement>(std::move(lhs));
        }
    }
    for (;;) {
        if (mTokens.empty()) {
            break;
        }
        if (std::holds_alternative<token::RSquareBracket>(peek().first)) {
            break;
        }
        if (std::holds_alternative<token::RPar>(peek().first)) {
            break;
        }
        if (std::holds_alternative<token::Semicolon>(peek().first)) {
            break;
        }
        if (std::holds_alternative<token::Comma>(peek().first)) {
            break;
        }
        if (std::holds_alternative<token::Equal>(peek().first)) {
            take();
            auto rhs = parseExpression(0);

            // Проверяем, является ли левая часть переменной
            if (auto variableReference = dynamic_cast<expression::VariableReference*>(lhs.get())) {
                lhs = std::make_unique<expression::VariableAssignment>(
                    std::move(variableReference->name), std::move(rhs));
                break;
            }

            // Проверяем, является ли левая часть доступом к массиву
            if (auto arrayIndex = dynamic_cast<expression::ArrayIndex*>(lhs.get())) {
                lhs = std::make_unique<expression::ArrayAssignment>(
                    std::move(arrayIndex->array),
                    std::move(arrayIndex->index),
                    std::move(rhs));
                break;
            }

            throw std::runtime_error(fmt::format("Expected variable or array element on left side of assignment, at line {}",peek().second));
        }

        int rightBindingPower = std::visit(
    match {
      [](token::Equal2 token) { return 0; }, // ==
      [](token::NoEqual token) { return 0; },// !=
      [](token::LAngleBracket token) { return 0; },      // <
      [](token::RAngleBracket token) { return 0; },      // >
      [](token::LAngleBracketEqual token) { return 0; }, // <=
      [](token::RAngleBracketEqual token) { return 0; }, // >=
      [](token::Plus token) { return 1; },
      [](token::Minus token) { return 1; },
      [](token::Asterisk token) { return 2; },
      [](token::Slash token) { return 2; },
      [](token::LSquareBracket token) { return 10; },
      [&](auto&& token) -> int {
          throw std::runtime_error(fmt::format("Unexpected token: {}, at line {}", tokenToString(peek().first),peek().second));
      },
    },
    peek().first);

        if (rightBindingPower < leftBindingPower) {
            return lhs;
        }

        auto opToken = take();
auto rhs = parseExpression(rightBindingPower);

lhs = std::visit(
    match {
      [&](token::Equal2 token) -> std::unique_ptr<expression::Base> {
          return std::make_unique<expression::Binary<std::equal_to<>>>(std::move(lhs), std::move(rhs));
      },
        [&](token::NoEqual token) -> std::unique_ptr<expression::Base> {
          return std::make_unique<expression::Binary<std::not_equal_to<>>>(std::move(lhs), std::move(rhs));
      },
      [&](token::LAngleBracket token) -> std::unique_ptr<expression::Base> {          // <
          return std::make_unique<expression::Binary<std::less<>>>(std::move(lhs), std::move(rhs));
      },
      [&](token::RAngleBracket token) -> std::unique_ptr<expression::Base> {          // >
          return std::make_unique<expression::Binary<std::greater<>>>(std::move(lhs), std::move(rhs));
      },
      [&](token::LAngleBracketEqual token) -> std::unique_ptr<expression::Base> {     // <=
          return std::make_unique<expression::Binary<std::less_equal<>>>(std::move(lhs), std::move(rhs));
      },
      [&](token::RAngleBracketEqual token) -> std::unique_ptr<expression::Base> {     // >=
          return std::make_unique<expression::Binary<std::greater_equal<>>>(std::move(lhs), std::move(rhs));
      },
      [&](token::Plus token) -> std::unique_ptr<expression::Base> {
          return std::make_unique<expression::Binary<std::plus<>>>(std::move(lhs), std::move(rhs));
      },
      [&](token::Minus token) -> std::unique_ptr<expression::Base> {
          return std::make_unique<expression::Binary<std::minus<>>>(std::move(lhs), std::move(rhs));
      },
      [&](token::Asterisk token) -> std::unique_ptr<expression::Base> {
          return std::make_unique<expression::Binary<std::multiplies<>>>(std::move(lhs), std::move(rhs));
      },
      [&](token::Slash token) -> std::unique_ptr<expression::Base> {
          return std::make_unique<expression::Binary<std::divides<>>>(std::move(lhs), std::move(rhs));
      },
      [&](auto&& token) -> std::unique_ptr<expression::Base> {
          throw std::runtime_error(fmt::format("Unexpected token: {}, at line {}", tokenToString(opToken.first),peek().second));
      },
    },
    opToken.first);
    }

    return lhs;
}

maxlang::expression::CommandSequence maxlang::Parser::parseCommandSequence() {
    std::vector<std::unique_ptr<expression::Base>> expressions;

    while (!mTokens.empty()) {
        auto current = peek();

        // Проверяем, является ли текущий токен ключевым словом
        if (auto keyword = std::get_if<token::Keyword>(&current.first)) {
            switch (*keyword) {
                case token::Keyword::IF:
                    expressions.push_back(parseIfStatement());
                    continue; // Уже обработали, переходим к следующему токену
                case token::Keyword::RETURN:
                    expressions.push_back(parseReturnStatement());
                    continue;
                case token::Keyword::FOR:
                    expressions.push_back(parseForStatement());
                    continue;
                case token::Keyword::WHILE:
                    expressions.push_back(parseWhileStatement());
                    continue;
                case token::Keyword::FOREACH:
                    expressions.push_back(parseForEachStatement());
                    continue;
                case token::Keyword::BREAK:
                    expressions.push_back(std::make_unique<expression::Break>());
                    take();
                    continue;
                case token::Keyword::CONTINUE:
                    expressions.push_back(std::make_unique<expression::Continue>());
                    take();
                    continue;
                case token::Keyword::FN:
                    expressions.push_back(parseFunctionDeclaration());
                    continue;
                case token::Keyword::ELSE:
                    // Обработка else должна быть в parseIfStatement
                    break;
            }
        }

        // Если это не ключевое слово, парсим как выражение
        if (std::holds_alternative<token::RCurlyBracket>(peek().first)) {
            break;
        }
        if (std::holds_alternative<token::Semicolon>(peek().first)) {
            take();
            continue;
        }

        expressions.push_back(parseExpression());
    }
    return expressions;
}

std::unique_ptr<maxlang::expression::Base> maxlang::Parser::parseIfStatement() {
    // Убедимся, что это действительно IF
    auto ifToken = take();
    if (!std::holds_alternative<token::Keyword>(ifToken.first) ||
        std::get<token::Keyword>(ifToken.first) != token::Keyword::IF) {
        throw std::runtime_error("Internal error: parseIfStatement called without IF token");
    }

    // Проверяем открывающую скобку с помощью peek
    auto lparToken = peek();
    if (!std::holds_alternative<token::LPar>(lparToken.first)) {
        throw std::runtime_error(fmt::format("Expected '(' after 'if', at line {}", lparToken.second));
    }
    take(); // consume '('

    // Парсим условие
    auto condition = parseExpression();

    // Проверяем закрывающую скобку
    auto rparToken = take();
    if (!std::holds_alternative<token::RPar>(rparToken.first)) {
        throw std::runtime_error(fmt::format("Expected ')' after condition, at line {}", rparToken.second));
    }

    // Проверяем открывающую фигурную скобку
    auto lcurlyToken = peek();
    if (!std::holds_alternative<token::LCurlyBracket>(lcurlyToken.first)) {
        throw std::runtime_error(fmt::format("Expected '{{' after ')', at line {}", lcurlyToken.second));
    }

    auto ifBody = parseCommandBlock();

    // Проверяем наличие else
    if (!mTokens.empty() &&
        std::holds_alternative<token::Keyword>(peek().first) &&
        std::get<token::Keyword>(peek().first) == token::Keyword::ELSE) {

        take(); // consume 'else'

        if (!std::holds_alternative<token::LCurlyBracket>(peek().first)) {
            throw std::runtime_error(fmt::format("Expected '{{' after 'else', at line {}", peek().second));
        }

        auto elseBody = parseCommandBlock();
        return std::make_unique<maxlang::expression::IfElse>(
            std::move(condition), std::move(ifBody), std::move(elseBody));
    }

    return std::make_unique<maxlang::expression::If>(std::move(condition), std::move(ifBody));
}


maxlang::expression::CommandSequence maxlang::Parser::parseCommandBlock() {
    auto openBrace = take();
    if (!std::holds_alternative<token::LCurlyBracket>(openBrace.first)) {
        throw std::runtime_error(fmt::format("Expected '{{', got {}, at line {}",
            tokenToString(openBrace.first), openBrace.second));
    }

    auto result = parseCommandSequence();

    if (mTokens.empty()) {
        throw std::runtime_error("Unexpected end of input in command block");
    }

    auto closeBrace = take();
    if (!std::holds_alternative<token::RCurlyBracket>(closeBrace.first)) {
        throw std::runtime_error(fmt::format("Expected '}}' to close command block, got {}, at line {}",
            tokenToString(closeBrace.first), closeBrace.second));
    }

    return result;
}

std::unique_ptr<maxlang::expression::Return> maxlang::Parser::parseReturnStatement() {
    assert(std::get<token::Keyword>(peek().first) == token::Keyword::RETURN);
    take();
    if (std::holds_alternative<token::Semicolon>(peek().first)) {
        // return;
        return std::make_unique<expression::Return>(nullptr);
    }

    // return 228;
    return std::make_unique<expression::Return>(parseExpression());
}

std::unique_ptr<maxlang::expression::For> maxlang::Parser::parseForStatement() {
    assert(std::get<token::Keyword>(peek().first) == token::Keyword::FOR);
    take(); // consume 'for'
    auto n = take();
    if (!std::holds_alternative<token::LPar>(n.first)) {
        throw std::runtime_error(fmt::format("Expected '(' after 'for', at line {}",n.second));
    }

    // Парсим инициализацию (может быть пустой)
    std::unique_ptr<expression::Base> initialization = nullptr;
    if (!std::holds_alternative<token::Semicolon>(peek().first)) {
        initialization = parseExpression();
    }
    n = take();
    if (!std::holds_alternative<token::Semicolon>(take().first)) {
        throw std::runtime_error(fmt::format("Expected ';' after for initialization at line {}",n.second));
    }

    // Парсим условие (может быть пустым)
    std::unique_ptr<expression::Base> condition = nullptr;
    if (!std::holds_alternative<token::Semicolon>(peek().first)) {
        condition = parseExpression();
    }
    n = take();
    if (!std::holds_alternative<token::Semicolon>(take().first)) {
        throw std::runtime_error(fmt::format("Expected ';' after for condition, at line {}",n.second));
    }

    // Парсим инкремент (может быть пустым)
    std::unique_ptr<expression::Base> increment = nullptr;
    if (!std::holds_alternative<token::RPar>(peek().first)) {
        increment = parseExpression();
    }
    n = take();
    if (!std::holds_alternative<token::RPar>(take().first)) {
        throw std::runtime_error(fmt::format("Expected ')' after for increment, at line {}",n.second));
    }

    // Парсим тело цикла
    if (!std::holds_alternative<token::LCurlyBracket>(peek().first)) {
        throw std::runtime_error(fmt::format("Expected '{}' after for statement","{",peek().second));
    }
    auto body = parseCommandBlock();

    return std::make_unique<maxlang::expression::For>(
        std::move(initialization),
        std::move(condition),
        std::move(increment),
        std::move(body)
    );
}

std::unique_ptr<maxlang::expression::While> maxlang::Parser::parseWhileStatement() {
    assert(std::get<token::Keyword>(peek().first) == token::Keyword::WHILE);
    take();

    // ИСПРАВЬТЕ ТАКЖЕ ЗДЕСЬ:
    auto n = peek();  // используем peek() вместо take()
    if (!std::holds_alternative<token::LPar>(n.first)) {
        throw std::runtime_error(fmt::format("Expected '(' after 'while', at line {}", n.second));
    }
    take(); // consume '('

    auto condition = parseExpression();

    n = peek();  // используем peek() для проверки ')'
    if (!std::holds_alternative<token::RPar>(n.first)) {
        throw std::runtime_error(fmt::format("Expected ')' after condition, at line {}", n.second));
    }
    take();
    if (!std::holds_alternative<token::LCurlyBracket>(peek().first)) {
        throw std::runtime_error(fmt::format("Expected '{}' after ')', at line {}","{",peek().second));
    }
    auto body = parseCommandBlock();
    return std::make_unique<maxlang::expression::While>(std::move(condition), std::move(body));
}

std::string maxlang::Parser::tokenToString(const maxlang::token::Any& token) {
    return std::visit(
        maxlang::match {
            [](maxlang::token::Keyword kw) -> std::string {
                switch (kw) {
                    case maxlang::token::Keyword::FN: return "fn";
                    case maxlang::token::Keyword::RETURN: return "return";
                    case maxlang::token::Keyword::IF: return "if";
                    case maxlang::token::Keyword::ELSE: return "else";
                    case maxlang::token::Keyword::FOR: return "for";
                    case maxlang::token::Keyword::WHILE: return "while";
                    case maxlang::token::Keyword::FOREACH: return "foreach";
                    case maxlang::token::Keyword::BREAK: return "break";
                    case maxlang::token::Keyword::CONTINUE: return "continue";
                }
                return "unknown keyword";
            },
            [](maxlang::token::LPar)-> std::string { return "("; },
            [](maxlang::token::RPar)-> std::string  { return ")"; },
            [](maxlang::token::LCurlyBracket)-> std::string  { return "{"; },
            [](maxlang::token::RCurlyBracket)-> std::string  { return "}"; },
            [](maxlang::token::LSquareBracket)-> std::string  { return "["; },
            [](maxlang::token::RSquareBracket)-> std::string  { return "]"; },
            [](maxlang::token::Semicolon)-> std::string  { return ";"; },
            [](maxlang::token::Plus)-> std::string  { return "+"; },
            [](maxlang::token::Minus)-> std::string  { return "-"; },
            [](maxlang::token::Asterisk)-> std::string  { return "*"; },
            [](maxlang::token::Slash)-> std::string  { return "/"; },
            [](maxlang::token::Comma)-> std::string  { return ","; },
            [](maxlang::token::Equal)-> std::string  { return "="; },
            [](maxlang::token::Equal2)-> std::string  { return "=="; },
            [](maxlang::token::NoEqual)-> std::string  { return "!="; },
            [](maxlang::token::LAngleBracket)-> std::string  { return "<"; },
            [](maxlang::token::RAngleBracket)-> std::string  { return ">"; },
            [](maxlang::token::LAngleBracketEqual)-> std::string  { return "<="; },
            [](maxlang::token::RAngleBracketEqual)-> std::string  { return ">="; },
            [](maxlang::token::PlusPlus)-> std::string  { return "++"; },
            [](maxlang::token::MinusMinus)-> std::string  { return "--"; },
            [](const maxlang::token::Identifier& id)-> std::string  { return "identifier:" + id.value; },
            [](const maxlang::token::String& s)-> std::string  { return "string:\"" + s.value + "\""; },
            [](const maxlang::token::Integer& i)-> std::string  { return "integer:" + std::to_string(i.value); },
            [](auto&&) { return "unknown token"; },
        },
        token);
}
std::unique_ptr<maxlang::expression::ForEach> maxlang::Parser::parseForEachStatement() {
    assert(std::get<token::Keyword>(peek().first) == token::Keyword::FOREACH);
    take(); // consume 'foreach'
    auto n = take();
    if (!std::holds_alternative<token::LPar>(n.first)) {
        throw std::runtime_error(fmt::format("Expected '(' after 'foreach', at line {}",n.second));
    }

    // Парсим имя переменной
    if (!std::holds_alternative<token::Identifier>(peek().first)) {
        throw std::runtime_error(fmt::format("Expected variable name in foreach, at line {}",peek().second));
    }
    std::string variableName = std::get<token::Identifier>(take().first).value;

    if (!std::holds_alternative<token::Keyword>(peek().first) ||
        std::get<token::Keyword>(peek().first) != token::Keyword::IN) {
        throw std::runtime_error(fmt::format("Expected 'in' after variable name, at line {}",peek().second));
        }
    take(); // consume 'in'

    // Парсим коллекцию (массив)
    auto collection = parseExpression();

    if (!std::holds_alternative<token::RPar>(take().first)) {
        throw std::runtime_error(fmt::format("Expected ')' after collection, at line {}",peek().second));
    }

    // Парсим тело цикла
    if (!std::holds_alternative<token::LCurlyBracket>(peek().first)) {
        throw std::runtime_error(fmt::format("Expected '{}' after foreach statement, at line {}","{",peek().second));
    }
    auto body = parseCommandBlock();

    return std::make_unique<maxlang::expression::ForEach>(
        std::move(variableName),
        std::move(collection),
        std::move(body)
    );
}
std::unique_ptr<maxlang::expression::FunctionDeclaration> maxlang::Parser::parseFunctionDeclaration() {
    assert(std::get<token::Keyword>(peek().first) == token::Keyword::FN);
    take(); // consume 'function'

    // Парсим имя функции
    if (!std::holds_alternative<token::Identifier>(peek().first)) {
        throw std::runtime_error(fmt::format("Expected function name, at line {}",peek().second));
    }
    std::string functionName = std::get<token::Identifier>(take().first).value;

    auto n = take();
    // Парсим параметры
    if (!std::holds_alternative<token::LPar>(n.first)) {
        throw std::runtime_error(fmt::format("Expected '(' after function name, at line {}",n.second));
    }

    std::vector<std::string> parameters;
    while (!std::holds_alternative<token::RPar>(peek().first)) {
        if (!std::holds_alternative<token::Identifier>(peek().first)) {
            throw std::runtime_error(fmt::format("Expected parameter name, at line {}",peek().second));
        }
        parameters.push_back(std::get<token::Identifier>(take().first).value);

        if (std::holds_alternative<token::Comma>(peek().first)) {
            take(); // consume ','
        } else if (!std::holds_alternative<token::RPar>(peek().first)) {
            throw std::runtime_error(fmt::format("Expected ',' or ')' in parameter list, at line {}",peek().second));
        }
    }
    take(); // consume ')'

    // Парсим тело функции
    if (!std::holds_alternative<token::LCurlyBracket>(peek().first)) {
        throw std::runtime_error(fmt::format("Expected '{}' after function parameters, at line {}","{",peek().second));
    }
    auto body = parseCommandBlock();

    return std::make_unique<expression::FunctionDeclaration>(
        std::move(functionName), std::move(parameters), std::move(body));
}