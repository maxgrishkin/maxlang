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
              if (!std::holds_alternative<token::RPar>(n)) {
                  throw std::runtime_error(fmt::format("Expected ')' to close '(', got {}", tokenToString(n)));
              }
              return lhs;
          },
          [&](token::Identifier identifier) -> std::unique_ptr<expression::Base> {
    // 1. variable reference
    // 2. function call
    if (std::holds_alternative<token::LPar>(peek())) {
        take();
        std::vector<std::unique_ptr<expression::Base>> args;
        for (;;) {
            auto n = peek();
            if (std::holds_alternative<token::RPar>(n)) {
                take();
                if (!args.empty()) {
                    throw std::runtime_error("Unexpected ')' after ','");
                }
                break;
            }
            args.push_back(parseExpression());
            if (std::holds_alternative<token::Comma>(peek())) {
                take();
                continue;
            }
            if (std::holds_alternative<token::RPar>(peek())) {
                take();
                break;
            }
            throw std::runtime_error(
                fmt::format("Expected ',' or ')' to close argument list, got {}", typeid(n).name()));
        }
        return std::make_unique<expression::FunctionCall>(std::move(identifier.value), std::move(args));
    }

    auto variableRef = std::make_unique<expression::VariableReference>(std::move(identifier.value));

    // Проверяем индексацию массива
    if (std::holds_alternative<token::LSquareBracket>(peek())) {
    take();

    auto index = parseExpression();

    if (mTokens.empty()) {
        throw std::runtime_error("Unexpected end of input after array index");
    }

    if (!std::holds_alternative<token::RSquareBracket>(peek())) {
        throw std::runtime_error(fmt::format("Expected ']' after array index, got {}",
            tokenToString(peek())));
    }
    take();

    return std::make_unique<expression::ArrayIndex>(std::move(variableRef), std::move(index));
}

    return variableRef;
},

            [&](token::LSquareBracket token) -> std::unique_ptr<expression::Base> {
    std::vector<std::unique_ptr<expression::Base>> elements;

    if (std::holds_alternative<token::RSquareBracket>(peek())) {
        take();
        return std::make_unique<expression::ArrayCreation>(std::move(elements), "");
    }

    while (true) {
        elements.push_back(parseExpression());

        if (mTokens.empty()) {
            throw std::runtime_error("Unexpected end of input in array literal");
        }

        if (std::holds_alternative<token::RSquareBracket>(peek())) {
            take();
            break;
        }

        if (std::holds_alternative<token::Comma>(peek())) {
            take();
            continue;
        }

        throw std::runtime_error(fmt::format("Expected ',' or ']' in array literal, got {}",
            tokenToString(peek())));
    }

    return std::make_unique<expression::ArrayCreation>(std::move(elements), "");
},

          [&](auto&& token) -> std::unique_ptr<expression::Base> {
              throw std::runtime_error(fmt::format("Unexpected token: {}", tokenToString(peek())));
          },
        },
        take());
    if (std::holds_alternative<token::LSquareBracket>(peek())) {
        take(); // consume '['
        auto index = parseExpression();
        if (!std::holds_alternative<token::RSquareBracket>(take())) {
            throw std::runtime_error("Expected ']' after index");
        }

        // Проверяем, является ли это присваиванием
        if (std::holds_alternative<token::Equal>(peek())) {
            take(); // consume '='
            auto value = parseExpression();
            return std::make_unique<expression::ArrayAssignment>(
                std::move(lhs), std::move(index), std::move(value));
        }

        return std::make_unique<expression::ArrayIndex>(std::move(lhs), std::move(index));
    }
    if (!mTokens.empty()) {
        if (std::holds_alternative<token::PlusPlus>(peek())) {
            take(); // consume '++'
            lhs = std::make_unique<expression::PostfixIncrement>(std::move(lhs));
        }
        if (std::holds_alternative<token::MinusMinus>(peek())) {
            take(); // consume '--'
            lhs = std::make_unique<expression::PostfixDecrement>(std::move(lhs));
        }
    }
    for (;;) {
        if (mTokens.empty()) {
            break;
        }
        if (std::holds_alternative<token::RSquareBracket>(peek())) {
            break;
        }
        if (std::holds_alternative<token::RPar>(peek())) {
            break;
        }
        if (std::holds_alternative<token::Semicolon>(peek())) {
            break;
        }
        if (std::holds_alternative<token::Comma>(peek())) {
            break;
        }
        if (std::holds_alternative<token::Equal>(peek())) {
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

            throw std::runtime_error("Expected variable or array element on left side of assignment");
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
          throw std::runtime_error(fmt::format("Unexpected token: {}", tokenToString(peek())));
      },
    },
    peek());

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
          throw std::runtime_error(fmt::format("Unexpected token: {}", tokenToString(opToken)));
      },
    },
    opToken);
    }

    return lhs;
}

maxlang::expression::CommandSequence maxlang::Parser::parseCommandSequence() {
    std::vector<std::unique_ptr<expression::Base>> expressions;
    out:
    while (!mTokens.empty()) {
        if (auto keyword = std::get_if<token::Keyword>(&peek())) {
            switch (*keyword) {
                case token::Keyword::IF:
                    expressions.push_back(parseIfStatement());
                    goto out;
                case token::Keyword::RETURN:
                    expressions.push_back(parseReturnStatement());
                    goto out;
                case token::Keyword::FOR:
                    expressions.push_back(parseForStatement());
                    goto out;
                case token::Keyword::WHILE:
                    expressions.push_back(parseWhileStatement());
                    goto out;
                case token::Keyword::FOREACH:
                    expressions.push_back(parseForEachStatement());
                    goto out;
                case token::Keyword::BREAK:
                    expressions.push_back(std::make_unique<expression::Break>());
                    take();
                    goto out;
                case token::Keyword::CONTINUE:
                    expressions.push_back(std::make_unique<expression::Continue>());
                    take();
                    goto out;
            }
        }
        if (std::holds_alternative<token::RCurlyBracket>(peek())) {
            break;
        }
        if (std::holds_alternative<token::Semicolon>(peek())) {
            take();
            continue;
        }
        expressions.push_back(parseExpression());
    }
    return expressions;
}

std::unique_ptr<maxlang::expression::If> maxlang::Parser::parseIfStatement() {
    assert(std::get<token::Keyword>(peek()) == token::Keyword::IF);
    take();
    if (!std::holds_alternative<token::LPar>(take())) {
        throw std::runtime_error("Expected '(' after 'if'");
    }
    auto condition = parseExpression();
    if (!std::holds_alternative<token::RPar>(take())) {
        throw std::runtime_error("Expected ')' after condition");
    }
    if (!std::holds_alternative<token::LCurlyBracket>(peek())) {
        throw std::runtime_error("Expected '{' after ')'");
    }
    auto body = parseCommandBlock();
    return std::make_unique<maxlang::expression::If>(std::move(condition), std::move(body));
}

maxlang::expression::CommandSequence maxlang::Parser::parseCommandBlock() {
    auto f1 = take();
    assert(std::holds_alternative<token::LCurlyBracket>(f1));
    auto result = parseCommandSequence();
    auto f2 = take();
    if (!std::holds_alternative<token::RCurlyBracket>(f2)) {
        throw std::runtime_error("Expected '}' that closes the command block");
    }
    return result;
}

std::unique_ptr<maxlang::expression::Return> maxlang::Parser::parseReturnStatement() {
    assert(std::get<token::Keyword>(peek()) == token::Keyword::RETURN);
    take();
    if (std::holds_alternative<token::Semicolon>(peek())) {
        // return;
        return std::make_unique<expression::Return>(nullptr);
    }

    // return 228;
    return std::make_unique<expression::Return>(parseExpression());
}

std::unique_ptr<maxlang::expression::For> maxlang::Parser::parseForStatement() {
    assert(std::get<token::Keyword>(peek()) == token::Keyword::FOR);
    take(); // consume 'for'

    if (!std::holds_alternative<token::LPar>(take())) {
        throw std::runtime_error("Expected '(' after 'for'");
    }

    // Парсим инициализацию (может быть пустой)
    std::unique_ptr<expression::Base> initialization = nullptr;
    if (!std::holds_alternative<token::Semicolon>(peek())) {
        initialization = parseExpression();
    }
    if (!std::holds_alternative<token::Semicolon>(take())) {
        throw std::runtime_error("Expected ';' after for initialization");
    }

    // Парсим условие (может быть пустым)
    std::unique_ptr<expression::Base> condition = nullptr;
    if (!std::holds_alternative<token::Semicolon>(peek())) {
        condition = parseExpression();
    }
    if (!std::holds_alternative<token::Semicolon>(take())) {
        throw std::runtime_error("Expected ';' after for condition");
    }

    // Парсим инкремент (может быть пустым)
    std::unique_ptr<expression::Base> increment = nullptr;
    if (!std::holds_alternative<token::RPar>(peek())) {
        increment = parseExpression();
    }
    if (!std::holds_alternative<token::RPar>(take())) {
        throw std::runtime_error("Expected ')' after for increment");
    }

    // Парсим тело цикла
    if (!std::holds_alternative<token::LCurlyBracket>(peek())) {
        throw std::runtime_error("Expected '{' after for statement");
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
    assert(std::get<token::Keyword>(peek()) == token::Keyword::WHILE);
    take();
    if (!std::holds_alternative<token::LPar>(take())) {
        throw std::runtime_error("Expected '(' after 'while'");
    }
    auto condition = parseExpression();
    if (!std::holds_alternative<token::RPar>(take())) {
        throw std::runtime_error("Expected ')' after condition");
    }
    if (!std::holds_alternative<token::LCurlyBracket>(peek())) {
        throw std::runtime_error("Expected '{' after ')'");
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
    assert(std::get<token::Keyword>(peek()) == token::Keyword::FOREACH);
    take(); // consume 'foreach'

    if (!std::holds_alternative<token::LPar>(take())) {
        throw std::runtime_error("Expected '(' after 'foreach'");
    }

    // Парсим имя переменной
    if (!std::holds_alternative<token::Identifier>(peek())) {
        throw std::runtime_error("Expected variable name in foreach");
    }
    std::string variableName = std::get<token::Identifier>(take()).value;

    if (!std::holds_alternative<token::Keyword>(peek()) ||
        std::get<token::Keyword>(peek()) != token::Keyword::IN) {
        throw std::runtime_error("Expected 'in' after variable name");
        }
    take(); // consume 'in'

    // Парсим коллекцию (массив)
    auto collection = parseExpression();

    if (!std::holds_alternative<token::RPar>(take())) {
        throw std::runtime_error("Expected ')' after collection");
    }

    // Парсим тело цикла
    if (!std::holds_alternative<token::LCurlyBracket>(peek())) {
        throw std::runtime_error("Expected '{' after foreach statement");
    }
    auto body = parseCommandBlock();

    return std::make_unique<maxlang::expression::ForEach>(
        std::move(variableName),
        std::move(collection),
        std::move(body)
    );
}