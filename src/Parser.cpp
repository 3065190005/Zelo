#include "Parser.h"
#include "ErrorCode.h"
#include <stdexcept>

namespace Zelo {

// 线程局部变量跟踪导出状态
thread_local bool hasExportList = false;

Parser::Parser(const std::vector<Token>& tokens) : tokens(tokens), current(0) {}

std::vector<StmtPtr> Parser::parse() {
    std::vector<StmtPtr> statements;
    while (!isAtEnd()) {
        statements.push_back(declaration());
    }
    return statements;
}

StmtPtr Parser::declaration() {
    try {
        if (match({TokenType::LOC, TokenType::CONST})) {
            return varDeclaration();
        }
        if (match({TokenType::FUNC})) {
            return functionDeclaration();
        }
        if (match({TokenType::CLASS})) {
            return classDeclaration();
        }
        if (match({TokenType::IMPORT})) {
            return importStatement();
        }
        if (match({TokenType::EXPORT})) {
            return exportStatement();
        }
        if (match({TokenType::NAMESPACE})) {
            return namespaceDeclaration();
        }
        if (match({TokenType::MACRO})) {
            return macroDeclaration();
        }
        return statement();
    } catch (const std::runtime_error& error) {
        // 错误恢复：同步到下一个语句
        synchronize();
        return nullptr;
    }
}

StmtPtr Parser::varDeclaration() {
    bool isConst = previous().type == TokenType::CONST;
    Token name = consume(TokenType::IDENTIFIER, "Expect variable name.");

    TypeAnnotation type;
    if (match({TokenType::COLON})) {
        type = typeAnnotation();
    }

    ExprPtr initializer;
    if (match({TokenType::ASSIGN})) {
        initializer = expression();
    }

    consume(TokenType::SEMICOLON, "Expect ';' after variable declaration.");
    return std::make_shared<VarDeclStmt>(name, type, std::move(initializer), isConst);
}

StmtPtr Parser::functionDeclaration() {
    Token name = consume(TokenType::IDENTIFIER, "Expect function name.");
    consume(TokenType::LPAREN, "Expect '(' after function name.");

    std::vector<std::pair<Token, TypeAnnotation>> parameters;
    if (!check(TokenType::RPAREN)) {
        do {
            Token paramName = consume(TokenType::IDENTIFIER, "Expect parameter name.");
            TypeAnnotation paramType;
            if (match({TokenType::COLON})) {
                paramType = typeAnnotation();
            }
            parameters.emplace_back(paramName, paramType);
        } while (match({TokenType::COMMA}));
    }
    consume(TokenType::RPAREN, "Expect ')' after parameters.");

    TypeAnnotation returnType;
    if (match({TokenType::COLON})) {
        returnType = typeAnnotation();
    }

    StmtPtr body = blockStatement();
    return std::make_shared<FunctionDeclStmt>(name, parameters, returnType, std::move(body));
}

StmtPtr Parser::classDeclaration() {
    Token name = consume(TokenType::IDENTIFIER, "Expect class name.");
    
    Token superclass;
    if (match({TokenType::COLON})) {
        superclass = consume(TokenType::IDENTIFIER, "Expect superclass name.");
    }
    
    consume(TokenType::LBRACE, "Expect '{' before class body.");
    
    std::vector<StmtPtr> methods;
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        methods.push_back(functionDeclaration());
    }
    
    consume(TokenType::RBRACE, "Expect '}' after class body.");
    return std::make_shared<ClassDeclStmt>(name, superclass, std::move(methods));
}

StmtPtr Parser::importStatement() {
    Token moduleName;
    std::vector<Token> imports;
    Token alias;
    
    if (match({TokenType::LBRACE})) {
        // 解构导入: import {a, b} from "module"
        do {
            imports.push_back(consume(TokenType::IDENTIFIER, "Expect identifier in import list."));
        } while (match({TokenType::COMMA}));
        consume(TokenType::RBRACE, "Expect '}' after import list.");
        consume(TokenType::FROM, "Expect 'from' after import list.");
        moduleName = consume(TokenType::STRING, "Expect module name string.");
        
        if (match({TokenType::AS})) {
            alias = consume(TokenType::IDENTIFIER, "Expect alias name after 'as'.");
        }
    } else {
        // 简单导入: import "module"
        moduleName = consume(TokenType::STRING, "Expect module name string.");
    }
    
    consume(TokenType::SEMICOLON, "Expect ';' after import statement.");
    return std::make_shared<ImportStmt>(moduleName, imports, alias);
}

StmtPtr Parser::exportStatement() {
    // 检查是否已经有导出列表
    if (hasExportList && previous().type == TokenType::EXPORT) {
        throw std::runtime_error("Cannot mix export list with individual exports.");
    }
    
    std::vector<Token> exports;
    
    if (match({TokenType::LBRACE})) {
        hasExportList = true;
        // 导出列表: export {a, b}
        do {
            Token exportName = consume(TokenType::IDENTIFIER, "Expect identifier in export list.");
            
            // 检查是否有重命名
            if (match({TokenType::ASSIGN})) {
                Token originalName = consume(TokenType::IDENTIFIER, "Expect identifier after '='.");
                // 这里我们存储重命名信息，但简化处理
                exports.push_back(exportName);
            } else {
                exports.push_back(exportName);
            }
        } while (match({TokenType::COMMA}));
        consume(TokenType::RBRACE, "Expect '}' after export list.");
        
        if (match({TokenType::FROM})) {
            Token moduleName = consume(TokenType::STRING, "Expect module name string.");
            // 处理重导出 - 存储模块名信息
        }
    } else {
        // 单独的导出: export function/class
        if (hasExportList) {
            throw std::runtime_error("Cannot mix export list with individual exports.");
        }
        
        // 解析要导出的声明
        StmtPtr decl = declaration();
        if (auto funcDecl = std::dynamic_pointer_cast<FunctionDeclStmt>(decl)) {
            exports.push_back(funcDecl->name);
        } else if (auto classDecl = std::dynamic_pointer_cast<ClassDeclStmt>(decl)) {
            exports.push_back(classDecl->name);
        } else if (auto varDecl = std::dynamic_pointer_cast<VarDeclStmt>(decl)) {
            exports.push_back(varDecl->name);
        } else {
            throw std::runtime_error("Only functions, classes and variables can be exported.");
        }
    }
    
    consume(TokenType::SEMICOLON, "Expect ';' after export statement.");
    return std::make_shared<ExportStmt>(exports);
}

StmtPtr Parser::namespaceDeclaration() {
    Token name = consume(TokenType::IDENTIFIER, "Expect namespace name.");
    
    consume(TokenType::LBRACE, "Expect '{' before namespace body.");
    
    std::vector<StmtPtr> body;
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        body.push_back(declaration());
    }
    
    consume(TokenType::RBRACE, "Expect '}' after namespace body.");
    return std::make_shared<NamespaceStmt>(name, body);
}

StmtPtr Parser::macroDeclaration() {
    Token name = consume(TokenType::IDENTIFIER, "Expect macro name.");
    
    std::vector<Token> parameters;
    bool isFunctionLike = false;
    
    if (match({TokenType::LPAREN})) {
        isFunctionLike = true;
        if (!check(TokenType::RPAREN)) {
            do {
                parameters.push_back(consume(TokenType::IDENTIFIER, "Expect parameter name."));
            } while (match({TokenType::COMMA}));
        }
        consume(TokenType::RPAREN, "Expect ')' after macro parameters.");
    }
    
    // 解析宏体
    std::vector<Token> body;
    while (!check(TokenType::SEMICOLON) && !isAtEnd()) {
        body.push_back(advance());
    }
    
    consume(TokenType::SEMICOLON, "Expect ';' after macro declaration.");
    return std::make_shared<MacroStmt>(name, parameters, body, isFunctionLike);
}

StmtPtr Parser::statement() {
    if (match({TokenType::IF})) return ifStatement();
    if (match({TokenType::WHILE})) return whileStatement();
    if (match({TokenType::FOR})) return forStatement();
    if (match({TokenType::RETURN})) return returnStatement();
    if (match({TokenType::BREAK})) return breakStatement();
    if (match({TokenType::CONTINUE})) return continueStatement();
    if (match({TokenType::TRY})) return tryCatchStatement();
    if (match({TokenType::THROW})) return throwStatement();
    if (match({TokenType::LBRACE})) return blockStatement();
    return expressionStatement();
}

StmtPtr Parser::ifStatement() {
    ExprPtr condition = expression();
    consume(TokenType::THEN, "Expect 'then' after if condition.");
    
    StmtPtr thenBranch = statement();
    StmtPtr elseBranch;
    
    if (match({TokenType::ELIF})) {
        elseBranch = ifStatement(); // 递归处理 elif
    } else if (match({TokenType::ELSE})) {
        elseBranch = statement();
    }
    
    return std::make_shared<IfStmt>(std::move(condition), std::move(thenBranch), std::move(elseBranch));
}

StmtPtr Parser::whileStatement() {
    ExprPtr condition = expression();
    StmtPtr body = statement();
    return std::make_shared<WhileStmt>(std::move(condition), std::move(body));
}

StmtPtr Parser::forStatement() {
    consume(TokenType::LPAREN, "Expect '(' after 'for'.");
    
    Token variable = consume(TokenType::IDENTIFIER, "Expect variable name after 'for'.");
    consume(TokenType::IN, "Expect 'in' after variable name.");
    
    ExprPtr iterable = expression();
    consume(TokenType::RPAREN, "Expect ')' after for clauses.");
    
    StmtPtr body = statement();
    return std::make_shared<ForStmt>(variable, std::move(iterable), std::move(body));
}

StmtPtr Parser::returnStatement() {
    ExprPtr value;
    if (!check(TokenType::SEMICOLON)) {
        value = expression();
    }
    consume(TokenType::SEMICOLON, "Expect ';' after return value.");
    return std::make_shared<ReturnStmt>(std::move(value));
}

StmtPtr Parser::breakStatement() {
    consume(TokenType::SEMICOLON, "Expect ';' after 'break'.");
    return std::make_shared<BreakStmt>();
}

StmtPtr Parser::continueStatement() {
    consume(TokenType::SEMICOLON, "Expect ';' after 'continue'.");
    return std::make_shared<ContinueStmt>();
}

StmtPtr Parser::tryCatchStatement() {
    StmtPtr tryBlock = blockStatement();
    consume(TokenType::CATCH, "Expect 'catch' after try block.");
    
    consume(TokenType::LPAREN, "Expect '(' after 'catch'.");
    Token catchVar = consume(TokenType::IDENTIFIER, "Expect variable name in catch clause.");
    
    TypeAnnotation catchType;
    if (match({TokenType::COLON})) {
        catchType = typeAnnotation();
    }
    
    consume(TokenType::RPAREN, "Expect ')' after catch variable.");
    StmtPtr catchBlock = blockStatement();
    
    return std::make_shared<TryCatchStmt>(std::move(tryBlock), catchVar, catchType, std::move(catchBlock));
}

StmtPtr Parser::throwStatement() {
    ExprPtr expression = this->expression();
    consume(TokenType::SEMICOLON, "Expect ';' after throw expression.");
    return std::make_shared<ThrowStmt>(std::move(expression));
}

StmtPtr Parser::expressionStatement() {
    ExprPtr expr = expression();
    consume(TokenType::SEMICOLON, "Expect ';' after expression.");
    return std::make_shared<ExprStmt>(std::move(expr));
}

StmtPtr Parser::blockStatement() {
    consume(TokenType::LBRACE, "Expect '{' before block.");
    
    std::vector<StmtPtr> statements;
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        statements.push_back(declaration());
    }
    
    consume(TokenType::RBRACE, "Expect '}' after block.");
    return std::make_shared<BlockStmt>(std::move(statements));
}

ExprPtr Parser::expression() {
    return assignment();
}

ExprPtr Parser::assignment() {
    ExprPtr expr = ternary();
    
    if (match({TokenType::ASSIGN, TokenType::PLUS_ASSIGN, TokenType::MINUS_ASSIGN,
               TokenType::MULTIPLY_ASSIGN, TokenType::DIVIDE_ASSIGN, TokenType::MODULO_ASSIGN,
               TokenType::BIT_AND_ASSIGN, TokenType::BIT_OR_ASSIGN, TokenType::BIT_XOR_ASSIGN,
               TokenType::LSHIFT_ASSIGN, TokenType::RSHIFT_ASSIGN})) {
        Token op = previous();
        ExprPtr value = assignment();
        
        return std::make_shared<AssignExpr>(std::move(expr), op, std::move(value));
    }
    
    return expr;
}

ExprPtr Parser::ternary() {
    ExprPtr expr = logicalOr();
    
    if (match({TokenType::QUESTION})) {
        ExprPtr thenExpr = expression();
        consume(TokenType::COLON, "Expect ':' after ternary then expression.");
        ExprPtr elseExpr = ternary();
        expr = std::make_shared<ConditionalExpr>(std::move(expr), std::move(thenExpr), std::move(elseExpr));
    }
    
    return expr;
}

ExprPtr Parser::logicalOr() {
    ExprPtr expr = logicalAnd();
    
    while (match({TokenType::OR})) {
        Token op = previous();
        ExprPtr right = logicalAnd();
        expr = std::make_shared<BinaryExpr>(std::move(expr), op, std::move(right));
    }
    
    return expr;
}

ExprPtr Parser::logicalAnd() {
    ExprPtr expr = equality();
    
    while (match({TokenType::AND})) {
        Token op = previous();
        ExprPtr right = equality();
        expr = std::make_shared<BinaryExpr>(std::move(expr), op, std::move(right));
    }
    
    return expr;
}

ExprPtr Parser::equality() {
    ExprPtr expr = comparison();
    
    while (match({TokenType::EQUAL, TokenType::NOT_EQUAL})) {
        Token op = previous();
        ExprPtr right = comparison();
        expr = std::make_shared<BinaryExpr>(std::move(expr), op, std::move(right));
    }
    
    return expr;
}

ExprPtr Parser::comparison() {
    ExprPtr expr = term();
    
    while (match({TokenType::LESS, TokenType::LESS_EQUAL, TokenType::GREATER, TokenType::GREATER_EQUAL})) {
        Token op = previous();
        ExprPtr right = term();
        expr = std::make_shared<BinaryExpr>(std::move(expr), op, std::move(right));
    }
    
    return expr;
}

ExprPtr Parser::term() {
    ExprPtr expr = factor();
    
    while (match({TokenType::PLUS, TokenType::MINUS})) {
        Token op = previous();
        ExprPtr right = factor();
        expr = std::make_shared<BinaryExpr>(std::move(expr), op, std::move(right));
    }
    
    return expr;
}

ExprPtr Parser::factor() {
    ExprPtr expr = unary();
    
    while (match({TokenType::MULTIPLY, TokenType::DIVIDE, TokenType::MODULO})) {
        Token op = previous();
        ExprPtr right = unary();
        expr = std::make_shared<BinaryExpr>(std::move(expr), op, std::move(right));
    }
    
    return expr;
}

ExprPtr Parser::unary() {
    if (match({TokenType::NOT, TokenType::MINUS, TokenType::BIT_NOT, TokenType::INCREMENT, TokenType::DECREMENT})) {
        Token op = previous();
        ExprPtr right = unary();
        return std::make_shared<UnaryExpr>(op, std::move(right));
    }
    
    return call();
}

ExprPtr Parser::call() {
    ExprPtr expr = primary();
    
    while (true) {
        if (match({TokenType::LPAREN})) {
            // 函数调用
            std::vector<ExprPtr> arguments;
            if (!check(TokenType::RPAREN)) {
                do {
                    arguments.push_back(expression());
                } while (match({TokenType::COMMA}));
            }
            consume(TokenType::RPAREN, "Expect ')' after arguments.");
            expr = std::make_shared<CallExpr>(std::move(expr), std::move(arguments));
        } else if (match({TokenType::DOT})) {
            // 成员访问
            Token name = consume(TokenType::IDENTIFIER, "Expect property name after '.'.");
            expr = std::make_shared<MemberExpr>(std::move(expr), name);
        } else if (match({TokenType::LBRACKET})) {
            // 索引访问或切片
            ExprPtr index = expression();
            
            if (match({TokenType::COLON})) {
                // 切片操作
                ExprPtr stop;
                if (!check(TokenType::COLON) && !check(TokenType::RBRACKET)) {
                    stop = expression();
                }
                
                ExprPtr step;
                if (match({TokenType::COLON})) {
                    if (!check(TokenType::RBRACKET)) {
                        step = expression();
                    }
                }
                
                consume(TokenType::RBRACKET, "Expect ']' after slice.");
                expr = std::make_shared<SliceExpr>(std::move(expr), std::move(index), std::move(stop), std::move(step));
            } else {
                // 索引访问
                consume(TokenType::RBRACKET, "Expect ']' after index.");
                expr = std::make_shared<IndexExpr>(std::move(expr), std::move(index));
            }
        } else {
            break;
        }
    }
    
    return expr;
}

ExprPtr Parser::primary() {
    if (match({TokenType::FALSE})) return std::make_shared<LiteralExpr>(previous());
    if (match({TokenType::TRUE})) return std::make_shared<LiteralExpr>(previous());
    if (match({TokenType::NULL_KEYWORD})) return std::make_shared<LiteralExpr>(previous());
    if (match({TokenType::NUMBER, TokenType::STRING})) return std::make_shared<LiteralExpr>(previous());
    
    if (match({TokenType::IDENTIFIER})) {
        return std::make_shared<IdentifierExpr>(previous());
    }
    
    if (match({TokenType::LPAREN})) {
        ExprPtr expr = expression();
        consume(TokenType::RPAREN, "Expect ')' after expression.");
        return expr;
    }
    
    if (match({TokenType::LBRACKET})) {
        // 数组字面量
        std::vector<ExprPtr> elements;
        if (!check(TokenType::RBRACKET)) {
            do {
                elements.push_back(expression());
            } while (match({TokenType::COMMA}));
        }
        consume(TokenType::RBRACKET, "Expect ']' after array elements.");
        return std::make_shared<ArrayExpr>(std::move(elements));
    }
    
    if (match({TokenType::LBRACE})) {
        // 字典字面量
        std::vector<std::pair<ExprPtr, ExprPtr>> entries;
        if (!check(TokenType::RBRACE)) {
            do {
                ExprPtr key = expression();
                consume(TokenType::COLON, "Expect ':' after key.");
                ExprPtr value = expression();
                entries.emplace_back(std::move(key), std::move(value));
            } while (match({TokenType::COMMA}));
        }
        consume(TokenType::RBRACE, "Expect '}' after dictionary entries.");
        return std::make_shared<DictExpr>(std::move(entries));
    }
    
    throw std::runtime_error("Expect expression.");
}

TypeAnnotation Parser::typeAnnotation() {
    TypeAnnotation type;
    bool hasTypes = false;
    
    // 处理联合类型
    do {
        if (match({TokenType::TYPE_INT})) {
            type.types.push_back(TokenType::TYPE_INT);
            hasTypes = true;
        } else if (match({TokenType::TYPE_FLOAT})) {
            type.types.push_back(TokenType::TYPE_FLOAT);
            hasTypes = true;
        } else if (match({TokenType::TYPE_BOOL})) {
            type.types.push_back(TokenType::TYPE_BOOL);
            hasTypes = true;
        } else if (match({TokenType::TYPE_STRING})) {
            type.types.push_back(TokenType::TYPE_STRING);
            hasTypes = true;
        } else if (match({TokenType::TYPE_ARRAY})) {
            if (type.isArray || type.isDict) {
                throw std::runtime_error("Type cannot be both array and dict.");
            }
            type.isArray = true;
            consume(TokenType::LBRACKET, "Expect '[' after 'array'.");
            type.valueType = std::make_shared<TypeAnnotation>(typeAnnotation());
            consume(TokenType::RBRACKET, "Expect ']' after array type.");
            hasTypes = true;
        } else if (match({TokenType::TYPE_DICT})) {
            if (type.isArray || type.isDict) {
                throw std::runtime_error("Type cannot be both array and dict.");
            }
            type.isDict = true;
            consume(TokenType::LBRACE, "Expect '{' after 'dict'.");
            type.keyType = std::make_shared<TypeAnnotation>(typeAnnotation());
            consume(TokenType::COLON, "Expect ':' after key type.");
            type.valueType = std::make_shared<TypeAnnotation>(typeAnnotation());
            consume(TokenType::RBRACE, "Expect '}' after dict type.");
            hasTypes = true;
        } else if (match({TokenType::ELLIPSIS})) {
            type.types.push_back(TokenType::ELLIPSIS);
            hasTypes = true;
        } else {
            break;
        }
    } while (match({TokenType::PIPE}));
    
    if (!hasTypes) {
        throw std::runtime_error("Expect type annotation.");
    }
    
    // 验证类型注解的合理性
    if (type.isArray && !type.valueType) {
        throw std::runtime_error("Array type must have element type.");
    }
    
    if (type.isDict && (!type.keyType || !type.valueType)) {
        throw std::runtime_error("Dict type must have key and value types.");
    }
    
    // 检查字典键类型是否合理（必须是基本类型或字符串）
    if (type.isDict && type.keyType) {
        bool validKeyType = false;
        for (TokenType t : type.keyType->types) {
            if (t == TokenType::TYPE_STRING || t == TokenType::TYPE_INT || 
                t == TokenType::TYPE_FLOAT || t == TokenType::TYPE_BOOL) {
                validKeyType = true;
                break;
            }
        }
        if (!validKeyType && !type.keyType->types.empty()) {
            throw std::runtime_error("Dictionary key must be a basic type (string, int, float, bool).");
        }
    }
    
    return type;
}

bool Parser::match(std::initializer_list<TokenType> types) {
    for (TokenType type : types) {
        if (check(type)) {
            advance();
            return true;
        }
    }
    return false;
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) return false;
    return peek().type == type;
}

Token Parser::advance() {
    if (!isAtEnd()) current++;
    return previous();
}

Token Parser::peek() const {
    return tokens[current];
}

Token Parser::previous() const {
    return tokens[current - 1];
}

bool Parser::isAtEnd() const {
    return peek().type == TokenType::END_OF_FILE;
}

Token Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) return advance();
    throw std::runtime_error(message);
}

void Parser::synchronize() {
    advance();
    
    while (!isAtEnd()) {
        if (previous().type == TokenType::SEMICOLON) return;
        
        switch (peek().type) {
            case TokenType::CLASS:
            case TokenType::FUNC:
            case TokenType::LOC:
            case TokenType::CONST:
            case TokenType::FOR:
            case TokenType::IF:
            case TokenType::WHILE:
            case TokenType::RETURN:
                return;
            default:
                break;
        }
        
        advance();
    }
}

} // namespace Zelo