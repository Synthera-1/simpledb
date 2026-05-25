#pragma once
#include <string>

// all the possible token types the lexer thing can produce
enum class TokenType {
    // Keywords
    SELECT, INSERT, UPDATE, DELETE, CREATE, DROP,
    TABLE, INTO, VALUES, FROM, WHERE, AND, OR, NOT,
    ORDER, BY, ASC, DESC, LIMIT, JOIN, INNER, LEFT,
    ON, SET, PRIMARY, KEY, NULL_VAL, UNIQUE,

    // daata types
    INT, FLOAT, TEXT,

    // aggregate fimctopms
    COUNT, SUM, AVG, MIN, MAX,

    // symbols
    STAR,           // *
    COMMA,          // ,
    SEMICOLON,      // ;
    LPAREN,         // (
    RPAREN,         // )
    DOT,            // .

    // Operators
    EQ,             // =
    NEQ,            // !=
    LT,             // 
    GT,             // >
    LTE,            // <=
    GTE,            // >=

    // literals
    INTEGER_LITERAL,
    FLOAT_LITERAL,
    STRING_LITERAL,
    IDENTIFIER,

    // special
    END_OF_FILE,
    UNKNOWN
};

//a token thing, that is made up of a ToekType, string, and int.
struct Token {
    TokenType type;
    std::string value;
    int line;

    Token(TokenType type, std::string value, int line)
        : type(type), value(value), line(line) {}
};