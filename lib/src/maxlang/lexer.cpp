#include "lexer.h"
#include "fmt/format.h"
#include <charconv>
#include <iostream>
#include <map>
#include <ranges>
#include <stdexcept>

using namespace maxlang;
using namespace maxlang::token;

std::vector<std::pair<token::Any,int>> maxlang::lexer::process(std::string_view code) {
    int line = 1;
    bool singleLineComment = false;
    bool multiLineComment = false;
    std::vector<std::pair<token::Any,int>> result;

    for (auto it = code.begin(); it != code.end(); ++it) {
        auto remainingString = std::ranges::subrange(it, code.end());
        auto findBlank = [&] {
            return std::ranges::find_if(remainingString, [](char c) {
                switch (c) {
                    case ' ':
                    case '\t':
                    case '\n':
                        return true;
                    default:
                        return false;
                }
            });
        };
        if (singleLineComment) {
            if (*it == '\n') {
                singleLineComment = false;
                line++;
            }
            continue;
        }
        else{
            if (multiLineComment) {
                if (*it == '\n') {
                    line++;
                }
                if (std::next(it) != code.end() && *it == '*' && *std::next(it) == '/') {
                    it++; // Пропускаем '/'
                    multiLineComment = false;
                }
                continue;
            }
            else {
                switch (*it) {
                    default:
                        if (std::isalpha(*it)) {
                            auto blank = std::ranges::find_if(remainingString, [](char c) {
                                return !std::isalnum(c) && c != '_';
                            });
                            std::string_view valueString(it, blank);
                            it = std::prev(blank);

                            static std::map<std::string_view, Keyword> keywords = {
                                {"return", Keyword::RETURN},
                                {"fn", Keyword::FN},
                                {"if", Keyword::IF},
                                {"else", Keyword::ELSE},
                                {"for", Keyword::FOR},
                                {"while", Keyword::WHILE},
                                {"foreach", Keyword::FOREACH},
                                {"in", Keyword::IN},
                                {"break", Keyword::BREAK},
                                {"continue", Keyword::CONTINUE},
                        };

                            if (auto keyword = keywords.find(valueString); keyword != keywords.end()) {
                                result.push_back(std::make_pair(keyword->second,line));
                                break;
                            }
                            result.push_back(std::make_pair(Identifier{.value = std::string(valueString)},line));
                            break;
                        }

                        if (std::isdigit(*it)) {
                            auto end = std::ranges::find_if(remainingString, [](char c) {
                                return !std::isdigit(c);
                            });
                            std::string_view digitString(it, end);
                            int integer = 0;
                            std::from_chars(digitString.data(), digitString.data() + digitString.size(), integer);
                            result.push_back(std::make_pair(Integer{.value = integer},line));
                            it = std::prev(end);
                            break;
                        }

                        throw std::runtime_error(fmt::format("Unexpected character: '{}', at line {}", *it, line));

                    case ' ':
                    case '\r':
                    case '\t':
                        break;

                    case '\n':
                        ++line;
                        break;

                    case '(':
                        result.push_back(std::make_pair(LPar{},line));
                        break;
                    case ')':
                        result.push_back(std::make_pair(RPar{},line));
                        break;
                    case '{':
                        result.push_back(std::make_pair(LCurlyBracket{},line));
                        break;
                    case '}':
                        result.push_back(std::make_pair(RCurlyBracket{},line));
                        break;
                    case '[':
                        result.push_back(std::make_pair(LSquareBracket{},line));
                        break;
                    case ']':
                        result.push_back(std::make_pair(RSquareBracket{},line));
                        break;
                    case '<':
                        if ((std::next(it) != code.end())) {
                            if (*std::next(it) == '=') {
                                it++;
                                result.push_back(std::make_pair(LAngleBracketEqual{},line));
                                break;
                            }
                        }
                        result.push_back(std::make_pair(LAngleBracket{},line));
                        break;
                    case '>':
                        if ((std::next(it) != code.end())) {
                            if (*std::next(it) == '=') {
                                it++;
                                result.push_back(std::make_pair(RAngleBracketEqual{},line));
                                break;
                            }
                        }
                        result.push_back(std::make_pair(RAngleBracket{},line));
                        break;
                    case ';':
                        result.push_back(std::make_pair(Semicolon{},line));
                        break;
                    case '+':
                        if ((std::next(it) != code.end())) {
                            if (*std::next(it) == '+') {
                                it++;
                                result.push_back(std::make_pair(PlusPlus{},line));
                                break;
                            }
                        }
                        result.push_back(std::make_pair(Plus{},line));
                        break;
                    case '-':
                        if ((std::next(it) != code.end())) {
                            if (*std::next(it) == '-') {
                                it++;
                                result.push_back(std::make_pair(MinusMinus{},line));
                                break;
                            }
                        }
                        result.push_back(std::make_pair(Minus{},line));
                        break;
                    case '*':
                        result.push_back(std::make_pair(Asterisk{},line));
                        break;
                    case '/':
                        if ((std::next(it) != code.end())) {
                            if (*std::next(it) == '/') {
                                it++;
                                singleLineComment = true;
                                break;
                            }
                            if (*std::next(it) == '*') {
                                it++;
                                multiLineComment = true;
                                break;
                            }
                        }
                        result.push_back(std::make_pair(Slash{},line));
                        break;
                    case ',':
                        result.push_back(std::make_pair(Comma{},line));
                        break;

                    case '=':
                        if (std::next(it) != code.end()) {
                            if (*std::next(it) == '=') {
                                it++;
                                result.push_back(std::make_pair(Equal2{},line));
                                break;
                            }
                        }
                        result.push_back(std::make_pair(Equal{},line));
                        break;
                    case '!':
                        if (std::next(it) != code.end()) {
                            if (*std::next(it) == '=') {
                                it++;
                                result.push_back(std::make_pair(NoEqual{},line));
                                break;
                            }
                        }
                        break;
                    case '\'': {
                        auto string_end = std::ranges::find(std::ranges::subrange(std::next(remainingString.begin()), remainingString.end()), '\'');
                        if (string_end == remainingString.end()) {
                            throw std::runtime_error(fmt::format("String literal is not finished, at line {}", line));
                        }
                        result.push_back(std::make_pair(String{.value = std::string(std::next(it), string_end)},line));
                        it = string_end;
                        break;
                    }

                    case '"': {
                        auto string_end = std::ranges::find(std::ranges::subrange(std::next(remainingString.begin()), remainingString.end()), '"');
                        if (string_end == remainingString.end()) {
                            throw std::runtime_error(fmt::format("String literal is not finished, at line {}", line));
                        }
                        result.push_back(std::make_pair(String{.value = std::string(std::next(it), string_end)},line));
                        it = string_end;
                        break;
                    }
                }
                if (it == code.end()) {
                    break;
                }
            }
        }
    }
    return result;
}