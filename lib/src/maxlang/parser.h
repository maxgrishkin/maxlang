#pragma once


#include "maxlang/token.h"
#include "expression.h"
#include <memory>
#include <span>

namespace maxlang {

    class Parser {
    public:
        explicit Parser(std::span<std::pair<token::Any,int>> tokens) : mTokens(tokens) {}

        std::unique_ptr<maxlang::expression::Base> parseExpression() {
            return parseExpression(0);
        }

        std::vector<std::unique_ptr<expression::Base>> parseCommandSequence();

        std::string tokenToString(const token::Any & any);

    private:
        std::span<std::pair<maxlang::token::Any,int>> mTokens;

        std::unique_ptr<maxlang::expression::Base> parseExpression(int leftBindingPower);

        const std::pair<token::Any,int> peek() const {
            return mTokens.front();
        }

        std::pair<token::Any,int> take() {
            auto token = std::move(mTokens.front());
            mTokens = mTokens.subspan(1);
            return token;
        }
        std::unique_ptr<maxlang::expression::Base> parseIfStatement();
        std::unique_ptr<maxlang::expression::Return> parseReturnStatement();
        std::unique_ptr<maxlang::expression::For> parseForStatement();
        std::unique_ptr<maxlang::expression::While> parseWhileStatement();
        std::unique_ptr<maxlang::expression::ForEach> parseForEachStatement();
        std::unique_ptr<maxlang::expression::FunctionDeclaration> parseFunctionDeclaration();

        /**
         * @brief Like parseCommandSequence but also consumes '{' and '}'.
         */
        expression::CommandSequence parseCommandBlock();
    };

}