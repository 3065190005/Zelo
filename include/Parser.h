#pragma once

#include "Lexer.h"
#include "AST.h"
#include <vector>
#include <memory>

namespace Zelo {

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens);
    std::vector<StmtPtr> parse();

private:
    StmtPtr declaration();
    StmtPtr varDeclaration();
    StmtPtr functionDeclaration();
    StmtPtr classDeclaration();
    StmtPtr statement();
    StmtPtr ifStatement();
    StmtPtr whileStatement();
    StmtPtr forStatement();
    StmtPtr returnStatement();
    StmtPtr breakStatement();
    StmtPtr continueStatement();
    StmtPtr importStatement();
    StmtPtr exportStatement();
    StmtPtr tryCatchStatement();
    StmtPtr throwStatement();
    StmtPtr expressionStatement();
    StmtPtr blockStatement();

    ExprPtr expression();
    ExprPtr assignment();
    ExprPtr logicalOr();
    ExprPtr logicalAnd();
    ExprPtr equality();
    ExprPtr comparison();
    ExprPtr term();
    ExprPtr factor();
    ExprPtr unary();
    ExprPtr call();
    ExprPtr primary();

    TypeAnnotation typeAnnotation();

    bool match(std::initializer_list<TokenType> types);
    bool check(TokenType type) const;
    Token advance();
    Token peek() const;
    Token previous() const;
    bool isAtEnd() const;
    Token consume(TokenType type, const std::string& message);

    std::vector<Token> tokens;
    size_t current;
};

} // namespace Zelo