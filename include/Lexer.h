#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>

namespace Zelo {

enum class TokenType {
    // 标识符和字面量
    IDENTIFIER, NUMBER, STRING, BOOL, NULL_LITERAL,

    // 运算符
    PLUS, MINUS, MULTIPLY, DIVIDE, MODULO,
    ASSIGN, PLUS_ASSIGN, MINUS_ASSIGN, MULTIPLY_ASSIGN, DIVIDE_ASSIGN, MODULO_ASSIGN,
    BIT_AND, BIT_OR, BIT_XOR, BIT_NOT, BIT_AND_ASSIGN, BIT_OR_ASSIGN, BIT_XOR_ASSIGN,
    LSHIFT, RSHIFT, LSHIFT_ASSIGN, RSHIFT_ASSIGN,
    EQUAL, NOT_EQUAL, LESS, LESS_EQUAL, GREATER, GREATER_EQUAL,
    AND, OR, NOT,
    INCREMENT, DECREMENT,

    // 分隔符
    LPAREN, RPAREN, LBRACE, RBRACE, LBRACKET, RBRACKET,
    COMMA, COLON, SEMICOLON, DOT, RANGE, PIPE, ELLIPSIS,

    // 关键字
    LOC, FUNC, CLASS, IF, ELIF, ELSE, THEN, WHILE, DO, FOR, IN,
    RETURN, CONST, NEW, SUPER, THIS, TRY, CATCH, THROW,
    PUBLIC, PROTECTED, PRIVATE,
    IMPORT, EXPORT, FROM, AS, REQUIRE, INCLUDE,
    NAMESPACE, MACRO,
    ASYNC, AWAIT,
    TRUE, FALSE, NULL_KEYWORD,

    // 类型注解
    TYPE_INT, TYPE_FLOAT, TYPE_BOOL, TYPE_STRING, TYPE_ARRAY, TYPE_DICT,

    // 特殊
    END_OF_FILE, ERROR
};

struct Token {
    TokenType type;
    std::string value;
    size_t line;
    size_t column;

    Token(TokenType type, const std::string& value, size_t line, size_t column)
        : type(type), value(value), line(line), column(column) {}
};

class Lexer {
public:
    explicit Lexer(const std::string& source);
    std::vector<Token> tokenize();

private:
    char peek() const;
    char advance();
    void skipWhitespace();
    void skipComment();
    
    Token readNumber();
    Token readString();
    Token readIdentifier();
    Token readOperator();
    
    bool match(char expected);
    bool isAtEnd() const;

    const std::string source;
    size_t start;
    size_t current;
    size_t line;
    size_t column;
    
    std::unordered_map<std::string, TokenType> keywords;
};

} // namespace Zelo