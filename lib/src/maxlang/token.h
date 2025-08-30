#pragma once

#include <string>
#include <variant>

namespace maxlang::token {
    enum class Keyword {
        FN,
        RETURN,
        IF,
        FOR,
        WHILE,
        FOREACH,
        IN,
        BREAK,
        CONTINUE,
        ELSE,
    };

    struct LPar {
        auto operator<=>(const LPar&) const = default;
    };   // (
    struct RPar {
        auto operator<=>(const RPar&) const = default;
    };   // (
    struct LCurlyBracket {
        auto operator<=>(const LCurlyBracket&) const = default;
    };   // {
    struct RCurlyBracket {
        auto operator<=>(const RCurlyBracket&) const = default;
    };   // {
    struct LSquareBracket {
        auto operator<=>(const LSquareBracket&) const = default;
    };   // [
    struct LAngleBracket {
        auto operator<=>(const LAngleBracket&) const = default;
    };   // <
    struct RAngleBracket {
        auto operator<=>(const RAngleBracket&) const = default;
    };   // <
    struct LAngleBracketEqual {
        auto operator<=>(const LAngleBracketEqual&) const = default;
    };   // <=
    struct RAngleBracketEqual {
        auto operator<=>(const RAngleBracketEqual&) const = default;
    };   // >=
    struct RSquareBracket {
        auto operator<=>(const RSquareBracket&) const = default;
    };   // [
    struct Semicolon {
        auto operator<=>(const Semicolon&) const = default;
    };   // ;
    struct Plus {
        auto operator<=>(const Plus&) const = default;
    };   // +
    struct Minus {
        auto operator<=>(const Minus&) const = default;
    };   // -
    struct Asterisk {
        auto operator<=>(const Asterisk&) const = default;
    };   // *
    struct Slash {
        auto operator<=>(const Slash&) const = default;
    };   // /
    struct Comma {
        auto operator<=>(const Comma&) const = default;
    };   // ,
    struct Equal {
        auto operator<=>(const Equal&) const = default;
    };   // =
    struct NoEqual {
        auto operator<=>(const NoEqual&) const = default;
    };   // =
    struct Equal2 {
        auto operator<=>(const Equal2&) const = default;
    };   // ==
    struct PlusPlus {
        auto operator<=>(const PlusPlus&) const = default;
    };   // ++
    struct MinusMinus {
        auto operator<=>(const MinusMinus&) const = default;
    };   // --
    struct Identifier {
        auto operator<=>(const Identifier&) const = default;
        std::string value;
    };
    struct String {
        auto operator<=>(const String&) const = default;
        std::string value;
    };
    struct Integer {
        auto operator<=>(const Integer&) const = default;
        int32_t value;
    };

using Any = std::variant<
    Keyword, LPar, RPar, Equal, Equal2, LCurlyBracket, RCurlyBracket, Semicolon, Comma, Plus, Minus, Asterisk, Slash, Identifier,
    Integer, String,LSquareBracket,RSquareBracket,LAngleBracket,RAngleBracket,LAngleBracketEqual,RAngleBracketEqual,PlusPlus,MinusMinus,
    NoEqual>;
}