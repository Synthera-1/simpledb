#pragma once
#include <string>
#include <vector>
#include "token.h"

//here is where i turn the SQL languages and words into the tokens where i do stuff with them. 
class Lexer {
public:
    Lexer(const std::string& source);
    std::vector<Token> tokenize();

private:
    std::string source;
    int pos;
    int line;

    char current();
    char peek();
    void advance();
    void skipWhitespace();

    Token readString();
    Token readNumber();
    Token readIdentifierOrKeyword();

    bool isAlpha(char c);
    bool isDigit(char c);
    bool isAlphaNumeric(char c);
};
