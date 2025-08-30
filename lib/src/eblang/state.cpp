#include "state.h"
#include "lexer.h"
#include "parser.h"

using namespace maxlang;

maxlang::Value State::evaluate(std::string_view expression) {
    auto tokens = lexer::process(expression);
    Parser parser(tokens);
    return parser.parseExpression()->evaluate(mContext);
}

void State::run(std::string_view code) {
    auto tokens = lexer::process(code);
    Parser parser(tokens);
    expression::execute(parser.parseCommandSequence(), mContext);
}
