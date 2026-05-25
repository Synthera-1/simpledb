#include "lexer.h"
#include <unordered_map>
#include <stdexcept>

// The lexer: converts raw SQL text into tokens.
// Also: yes, the keyword map is large. SQL has a lot of ways to be dramatic.
static const std::unordered_map<std::string, TokenType> KEYWORDS = {

    {"SELECT", TokenType::SELECT}, {"INSERT", TokenType::INSERT},
    {"UPDATE", TokenType::UPDATE}, {"DELETE", TokenType::DELETE},
    {"CREATE", TokenType::CREATE}, {"DROP",   TokenType::DROP},
    {"TABLE",  TokenType::TABLE},  {"INTO",   TokenType::INTO},
    {"VALUES", TokenType::VALUES}, {"FROM",   TokenType::FROM},
    {"WHERE",  TokenType::WHERE},  {"AND",    TokenType::AND},
    {"OR",     TokenType::OR},     {"NOT",    TokenType::NOT},
    {"ORDER",  TokenType::ORDER},  {"BY",     TokenType::BY},
    {"ASC",    TokenType::ASC},    {"DESC",   TokenType::DESC},
    {"LIMIT",  TokenType::LIMIT},  {"JOIN",   TokenType::JOIN},
    {"INNER",  TokenType::INNER},  {"LEFT",   TokenType::LEFT},
    {"ON",     TokenType::ON},     {"SET",    TokenType::SET},
    {"PRIMARY",TokenType::PRIMARY},{"KEY",    TokenType::KEY},
    {"NULL",   TokenType::NULL_VAL},{"UNIQUE", TokenType::UNIQUE},
    {"INT",    TokenType::INT},    {"FLOAT",  TokenType::FLOAT},
    {"TEXT",   TokenType::TEXT},   {"COUNT",  TokenType::COUNT},
    {"SUM",    TokenType::SUM},    {"AVG",    TokenType::AVG},
    {"MIN",    TokenType::MIN},    {"MAX",    TokenType::MAX}
};

Lexer::Lexer(const std::string& source) : source(source), pos(0), line(1) {}

//what character am i lookin at right now
char Lexer::current() {
    if (pos >= source.size()) return '\0';
    return source[pos];
}

//(peeking at the next character in the sequence)
char Lexer::peek() {
    if (pos + 1 >= source.size()) return '\0';
    return source[pos + 1];
}

void Lexer::advance() { pos++; }

//skips the blanks. we dont care about them. tracks the line number we are on so we can 
//say for example: "horrible syntax at line 999" :)
void Lexer::skipWhitespace() {
    while (pos < source.size() && isspace(current())) {
        if (current() == '\n') line++;
        advance();
    }
}

bool Lexer::isAlpha(char c)        { return isalpha(c) || c == '_'; }
bool Lexer::isDigit(char c)        { return isdigit(c); }
bool Lexer::isAlphaNumeric(char c) { return isAlpha(c) || isDigit(c); }

//hmm. i wonder what it reads. it reads strings. it looks for text in 
//single quotes like these: ''. it goes from first quote to last quote and reads everything in the middle.
Token Lexer::readString() {
    advance(); // skip opening quote
    std::string result;
    while (pos < source.size() && current() != '\'') {
        result += current();
        advance();
    }
    advance(); // skip closing quote
    return Token(TokenType::STRING_LITERAL, result, line);
}

//it reads digits until it hits something that is not a digit. if its a dot then it turns into a floaty. 
Token Lexer::readNumber() {
    std::string result;
    bool isFloat = false;
    while (pos < source.size() && (isDigit(current()) || current() == '.')) {
        if (current() == '.') isFloat = true;
        result += current();
        advance();
    }
    if (isFloat) return Token(TokenType::FLOAT_LITERAL, result, line);
    return Token(TokenType::INTEGER_LITERAL, result, line);
}

//reads the thing character by character. it makes everything big letters. (uppercase) bc SQL is not case sensitive (fun fact).
Token Lexer::readIdentifierOrKeyword() {
    std::string result;
    while (pos < source.size() && isAlphaNumeric(current())) {
        result += toupper(current());
        advance();
    }
    auto it = KEYWORDS.find(result);
    if (it != KEYWORDS.end()) {
        return Token(it->second, result, line);
    }
    return Token(TokenType::IDENTIFIER, result, line);
}

//it keeps reading letters, skipping blanks and routing to the correct reader based on what it see.
std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (pos < source.size()) {
        skipWhitespace();
        if (pos >= source.size()) break;

        char c = current();

        if (c == '\'') { tokens.push_back(readString()); continue; }
        if (isDigit(c)) { tokens.push_back(readNumber()); continue; }
        if (isAlpha(c)) { tokens.push_back(readIdentifierOrKeyword()); continue; }

        switch (c) {
            case '*': tokens.push_back(Token(TokenType::STAR,      "*", line)); advance(); break;
            case ',': tokens.push_back(Token(TokenType::COMMA,     ",", line)); advance(); break;
            case ';': tokens.push_back(Token(TokenType::SEMICOLON, ";", line)); advance(); break;
            case '(': tokens.push_back(Token(TokenType::LPAREN,    "(", line)); advance(); break;
            case ')': tokens.push_back(Token(TokenType::RPAREN,    ")", line)); advance(); break;
            case '.': tokens.push_back(Token(TokenType::DOT,       ".", line)); advance(); break;
            case '=': tokens.push_back(Token(TokenType::EQ,        "=", line)); advance(); break;
            case '<':
                if (peek() == '=') {
                    tokens.push_back(Token(TokenType::LTE, "<=", line));
                    advance(); advance();
                } else {
                    tokens.push_back(Token(TokenType::LT, "<", line));
                    advance();
                }
                break;
            case '>':
                if (peek() == '=') {
                    tokens.push_back(Token(TokenType::GTE, ">=", line));
                    advance(); advance();
                } else {
                    tokens.push_back(Token(TokenType::GT, ">", line));
                    advance();
                }
                break;
            case '!':
                if (peek() == '=') {
                    tokens.push_back(Token(TokenType::NEQ, "!=", line));
                    advance(); advance();
                } else {
                    throw std::runtime_error("Unexpected character '!' at line " + std::to_string(line));
                }
                break;
            default:
                throw std::runtime_error("Unknown character '" + std::string(1, c) + "' at line " + std::to_string(line));
        }
    }

    tokens.push_back(Token(TokenType::END_OF_FILE, "", line));
    return tokens;
}