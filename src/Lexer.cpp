#include "Lexer.h"
#include <cctype>
#include <iostream>

namespace Zelo {

Lexer::Lexer(const std::string& source) 
    : source(source), start(0), current(0), line(1), column(1) {
    // 初始化关键字表
    keywords = {
        {"loc", TokenType::LOC},
        {"func", TokenType::FUNC},
        {"class", TokenType::CLASS},
        {"if", TokenType::IF},
        {"elif", TokenType::ELIF},
        {"else", TokenType::ELSE},
        {"then", TokenType::THEN},
        {"while", TokenType::WHILE},
        {"do", TokenType::DO},
        {"for", TokenType::FOR},
        {"in", TokenType::IN},
        {"return", TokenType::RETURN},
        {"const", TokenType::CONST},
        {"new", TokenType::NEW},
        {"super", TokenType::SUPER},
        {"this", TokenType::THIS},
        {"try", TokenType::TRY},
        {"catch", TokenType::CATCH},
        {"throw", TokenType::THROW},
        {"public", TokenType::PUBLIC},
        {"protected", TokenType::PROTECTED},
        {"private", TokenType::PRIVATE},
        {"import", TokenType::IMPORT},
        {"export", TokenType::EXPORT},
        {"from", TokenType::FROM},
        {"as", TokenType::AS},
        {"require", TokenType::REQUIRE},
        {"include", TokenType::INCLUDE},
        {"namespace", TokenType::NAMESPACE},
        {"macro", TokenType::MACRO},
        {"async", TokenType::ASYNC},
        {"await", TokenType::AWAIT},
        {"true", TokenType::TRUE},
        {"false", TokenType::FALSE},
        {"null", TokenType::NULL_KEYWORD},
        {"int", TokenType::TYPE_INT},
        {"float", TokenType::TYPE_FLOAT},
        {"bool", TokenType::TYPE_BOOL},
        {"string", TokenType::TYPE_STRING},
        {"array", TokenType::TYPE_ARRAY},
        {"dict", TokenType::TYPE_DICT}
    };
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    
    while (!isAtEnd()) {
        start = current;
        char c = advance();
        
        switch (c) {
            case ' ': case '\t': case '\r':
                // 跳过空白字符
                break;
            case '\n':
                line++;
                column = 1;
                break;
            case '#':
                skipComment();
                break;
            case '/':
                if (match('/')) {
                    skipComment();
                } else if (match('*')) {
                    skipComment();
                } else {
                    tokens.push_back(Token(TokenType::DIVIDE, "/", line, column));
                }
                break;
            case '"': case '\'':
                tokens.push_back(readString());
                break;
            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':
                tokens.push_back(readNumber());
                break;
            case '+':
                if (match('+')) {
                    tokens.push_back(Token(TokenType::INCREMENT, "++", line, column));
                } else if (match('=')) {
                    tokens.push_back(Token(TokenType::PLUS_ASSIGN, "+=", line, column));
                } else {
                    tokens.push_back(Token(TokenType::PLUS, "+", line, column));
                }
                break;
            case '-':
                if (match('-')) {
                    tokens.push_back(Token(TokenType::DECREMENT, "--", line, column));
                } else if (match('=')) {
                    tokens.push_back(Token(TokenType::MINUS_ASSIGN, "-=", line, column));
                } else if (match('>')) {
                    tokens.push_back(Token(TokenType::RANGE, "->", line, column));
                } else {
                    tokens.push_back(Token(TokenType::MINUS, "-", line, column));
                }
                break;
            case '*':
                if (match('=')) {
                    tokens.push_back(Token(TokenType::MULTIPLY_ASSIGN, "*=", line, column));
                } else {
                    tokens.push_back(Token(TokenType::MULTIPLY, "*", line, column));
                }
                break;
            case '%':
                if (match('=')) {
                    tokens.push_back(Token(TokenType::MODULO_ASSIGN, "%=", line, column));
                } else {
                    tokens.push_back(Token(TokenType::MODULO, "%", line, column));
                }
                break;
            case '=':
                if (match('=')) {
                    tokens.push_back(Token(TokenType::EQUAL, "==", line, column));
                } else {
                    tokens.push_back(Token(TokenType::ASSIGN, "=", line, column));
                }
                break;
            case '!':
                if (match('=')) {
                    tokens.push_back(Token(TokenType::NOT_EQUAL, "!=", line, column));
                } else {
                    tokens.push_back(Token(TokenType::NOT, "!", line, column));
                }
                break;
            case '<':
                if (match('<')) {
                    if (match('=')) {
                        tokens.push_back(Token(TokenType::LSHIFT_ASSIGN, "<<=", line, column));
                    } else {
                        tokens.push_back(Token(TokenType::LSHIFT, "<<", line, column));
                    }
                } else if (match('=')) {
                    tokens.push_back(Token(TokenType::LESS_EQUAL, "<=", line, column));
                } else {
                    tokens.push_back(Token(TokenType::LESS, "<", line, column));
                }
                break;
            case '>':
                if (match('>')) {
                    if (match('=')) {
                        tokens.push_back(Token(TokenType::RSHIFT_ASSIGN, ">>=", line, column));
                    } else {
                        tokens.push_back(Token(TokenType::RSHIFT, ">>", line, column));
                    }
                } else if (match('=')) {
                    tokens.push_back(Token(TokenType::GREATER_EQUAL, ">=", line, column));
                } else {
                    tokens.push_back(Token(TokenType::GREATER, ">", line, column));
                }
                break;
            case '&':
                if (match('&')) {
                    tokens.push_back(Token(TokenType::AND, "&&", line, column));
                } else if (match('=')) {
                    tokens.push_back(Token(TokenType::BIT_AND_ASSIGN, "&=", line, column));
                } else {
                    tokens.push_back(Token(TokenType::BIT_AND, "&", line, column));
                }
                break;
            case '|':
                if (match('|')) {
                    tokens.push_back(Token(TokenType::OR, "||", line, column));
                } else if (match('=')) {
                    tokens.push_back(Token(TokenType::BIT_OR_ASSIGN, "|=", line, column));
                } else {
                    tokens.push_back(Token(TokenType::BIT_OR, "|", line, column));
                }
                break;
            case '^':
                if (match('=')) {
                    tokens.push_back(Token(TokenType::BIT_XOR_ASSIGN, "^=", line, column));
                } else {
                    tokens.push_back(Token(TokenType::BIT_XOR, "^", line, column));
                }
                break;
            case '~':
                tokens.push_back(Token(TokenType::BIT_NOT, "~", line, column));
                break;
            case '(':
                tokens.push_back(Token(TokenType::LPAREN, "(", line, column));
                break;
            case ')':
                tokens.push_back(Token(TokenType::RPAREN, ")", line, column));
                break;
            case '{':
                tokens.push_back(Token(TokenType::LBRACE, "{", line, column));
                break;
            case '}':
                tokens.push_back(Token(TokenType::RBRACE, "}", line, column));
                break;
            case '[':
                tokens.push_back(Token(TokenType::LBRACKET, "[", line, column));
                break;
            case ']':
                tokens.push_back(Token(TokenType::RBRACKET, "]", line, column));
                break;
            case ',':
                tokens.push_back(Token(TokenType::COMMA, ",", line, column));
                break;
            case ':':
                tokens.push_back(Token(TokenType::COLON, ":", line, column));
                break;
            case ';':
                tokens.push_back(Token(TokenType::SEMICOLON, ";", line, column));
                break;
            case '.':
                if (match('.')) {
                    if (match('.')) {
                        tokens.push_back(Token(TokenType::ELLIPSIS, "...", line, column));
                    } else {
                        // 错误处理：.. 不是有效标记
                        tokens.push_back(Token(TokenType::ERROR, "..", line, column));
                    }
                } else {
                    tokens.push_back(Token(TokenType::DOT, ".", line, column));
                }
                break;
            default:
                if (std::isalpha(c) || c == '_') {
                    tokens.push_back(readIdentifier());
                } else {
                    // 未知字符
                    tokens.push_back(Token(TokenType::ERROR, std::string(1, c), line, column));
                }
                break;
        }
    }
    
    tokens.push_back(Token(TokenType::END_OF_FILE, "", line, column));
    return tokens;
}

char Lexer::peek() const {
    if (isAtEnd()) return '\0';
    return source[current];
}

char Lexer::advance() {
    if (isAtEnd()) return '\0';
    column++;
    return source[current++];
}

void Lexer::skipWhitespace() {
    while (!isAtEnd()) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\r') {
            advance();
        } else if (c == '\n') {
            line++;
            column = 1;
            advance();
        } else {
            break;
        }
    }
}

void Lexer::skipComment() {
    if (source[start] == '#') {
        // 单行注释 #
        while (!isAtEnd() && peek() != '\n') {
            advance();
        }
    } else if (source[start] == '/' && peek() == '/') {
        // 单行注释 //
        while (!isAtEnd() && peek() != '\n') {
            advance();
        }
    } else if (source[start] == '/' && peek() == '*') {
        // 多行注释 /* */
        advance(); // 跳过 *
        while (!isAtEnd()) {
            if (peek() == '*' && current + 1 < source.size() && source[current + 1] == '/') {
                advance(); // 跳过 *
                advance(); // 跳过 /
                break;
            }
            if (peek() == '\n') {
                line++;
                column = 1;
            }
            advance();
        }
    }
}

Token Lexer::readNumber() {
    bool isFloat = false;
    bool isHex = false;
    bool isBinary = false;
    bool isOctal = false;
    
    // 检查十六进制、二进制、八进制前缀
    if (peek() == 'x' || peek() == 'X') {
        isHex = true;
        advance();
    } else if (peek() == 'b' || peek() == 'B') {
        isBinary = true;
        advance();
    } else if (peek() == 'o' || peek() == 'O') {
        isOctal = true;
        advance();
    }
    
    while (!isAtEnd()) {
        char c = peek();
        
        if (isHex) {
            if (std::isxdigit(c)) {
                advance();
            } else {
                break;
            }
        } else if (isBinary) {
            if (c == '0' || c == '1') {
                advance();
            } else {
                break;
            }
        } else if (isOctal) {
            if (c >= '0' && c <= '7') {
                advance();
            } else {
                break;
            }
        } else if (std::isdigit(c)) {
            advance();
        } else if (c == '.') {
            if (isFloat) break; // 多个小数点
            isFloat = true;
            advance();
        } else if (c == 'e' || c == 'E') {
            isFloat = true;
            advance();
            if (peek() == '+' || peek() == '-') {
                advance();
            }
        } else {
            break;
        }
    }
    
    std::string value = source.substr(start, current - start);
    return Token(isFloat ? TokenType::NUMBER : TokenType::NUMBER, value, line, column);
}

Token Lexer::readString() {
    char quote = source[start];
    std::string value;
    bool escape = false;
    
    while (!isAtEnd()) {
        char c = advance();
        
        if (escape) {
            switch (c) {
                case 'n': value += '\n'; break;
                case 't': value += '\t'; break;
                case 'r': value += '\r'; break;
                case '\\': value += '\\'; break;
                case '"': value += '"'; break;
                case '\'': value += '\''; break;
                default: value += c; break;
            }
            escape = false;
        } else if (c == '\\') {
            escape = true;
        } else if (c == quote) {
            break;
        } else {
            value += c;
        }
        
        if (c == '\n') {
            line++;
            column = 1;
        }
    }
    
    return Token(TokenType::STRING, value, line, column);
}

Token Lexer::readIdentifier() {
    while (!isAtEnd()) {
        char c = peek();
        if (std::isalnum(c) || c == '_') {
            advance();
        } else {
            break;
        }
    }
    
    std::string text = source.substr(start, current - start);
    TokenType type = TokenType::IDENTIFIER;
    
    auto it = keywords.find(text);
    if (it != keywords.end()) {
        type = it->second;
    }
    
    return Token(type, text, line, column);
}

bool Lexer::match(char expected) {
    if (isAtEnd()) return false;
    if (source[current] != expected) return false;
    
    current++;
    column++;
    return true;
}

bool Lexer::isAtEnd() const {
    return current >= source.size();
}

} // namespace Zelo