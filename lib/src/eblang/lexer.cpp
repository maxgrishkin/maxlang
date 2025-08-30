#include "lexer.h"
#include "fmt/format.h"
#include <charconv>
#include <iostream>
#include <map>
#include <ranges>
#include <stdexcept>

using namespace maxlang;
using namespace maxlang::token;

std::vector<token::Any> maxlang::lexer::process(std::string_view code) {
    int line = 1;
    bool singleLineComment = false;
    bool multiLineComment = false;
    std::vector<token::Any> result;

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
                                {"for", Keyword::FOR},
                                {"while", Keyword::WHILE},
                                {"foreach", Keyword::FOREACH},
                                {"in", Keyword::IN},
                                {"break", Keyword::BREAK},
                                {"continue", Keyword::CONTINUE},
                        };

                            if (auto keyword = keywords.find(valueString); keyword != keywords.end()) {
                                result.push_back(keyword->second);
                                break;
                            }
                            result.push_back(Identifier{.value = std::string(valueString)});
                            break;
                        }

                        if (std::isdigit(*it)) {
                            auto end = std::ranges::find_if(remainingString, [](char c) {
                                return !std::isdigit(c);
                            });
                            std::string_view digitString(it, end);
                            int integer = 0;
                            std::from_chars(digitString.data(), digitString.data() + digitString.size(), integer);
                            result.push_back(Integer{.value = integer});
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
                        result.push_back(LPar{});
                        break;
                    case ')':
                        result.push_back(RPar{});
                        break;
                    case '{':
                        result.push_back(LCurlyBracket{});
                        break;
                    case '}':
                        result.push_back(RCurlyBracket{});
                        break;
                    case '[':
                        result.push_back(LSquareBracket{});
                        break;
                    case ']':
                        result.push_back(RSquareBracket{});
                        break;
                    case '<':
                        if ((std::next(it) != code.end())) {
                            if (*std::next(it) == '=') {
                                it++;
                                result.push_back(LAngleBracketEqual{});
                                break;
                            }
                        }
                        result.push_back(LAngleBracket{});
                        break;
                    case '>':
                        if ((std::next(it) != code.end())) {
                            if (*std::next(it) == '=') {
                                it++;
                                result.push_back(RAngleBracketEqual{});
                                break;
                            }
                        }
                        result.push_back(RAngleBracket{});
                        break;
                    case ';':
                        result.push_back(Semicolon{});
                        break;
                    case '+':
                        if ((std::next(it) != code.end())) {
                            if (*std::next(it) == '+') {
                                it++;
                                result.push_back(PlusPlus{});
                                break;
                            }
                        }
                        result.push_back(Plus{});
                        break;
                    case '-':
                        if ((std::next(it) != code.end())) {
                            if (*std::next(it) == '-') {
                                it++;
                                result.push_back(MinusMinus{});
                                break;
                            }
                        }
                        result.push_back(Minus{});
                        break;
                    case '*':
                        result.push_back(Asterisk{});
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
                        result.push_back(Slash{});
                        break;
                    case ',':
                        result.push_back(Comma{});
                        break;

                    case '=':
                        if (std::next(it) != code.end()) {
                            if (*std::next(it) == '=') {
                                it++;
                                result.push_back(Equal2{});
                                break;
                            }
                        }
                        result.push_back(Equal{});
                        break;
                    case '!':
                        if (std::next(it) != code.end()) {
                            if (*std::next(it) == '=') {
                                it++;
                                result.push_back(NoEqual{});
                                break;
                            }
                        }
                        break;
                    case '\'': {
                        auto string_end = std::ranges::find(std::ranges::subrange(std::next(remainingString.begin()), remainingString.end()), '\'');
                        if (string_end == remainingString.end()) {
                            throw std::runtime_error(fmt::format("String literal is not finished, at line {}", line));
                        }
                        result.push_back(String{.value = std::string(std::next(it), string_end)});
                        it = string_end;
                        break;
                    }

                    case '"': {
                        auto string_end = std::ranges::find(std::ranges::subrange(std::next(remainingString.begin()), remainingString.end()), '"');
                        if (string_end == remainingString.end()) {
                            throw std::runtime_error(fmt::format("String literal is not finished, at line {}", line));
                        }
                        result.push_back(String{.value = std::string(std::next(it), string_end)});
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