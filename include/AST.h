#pragma once

#include <string>
#include <vector>
#include <memory>
#include <variant>
#include "Lexer.h"
#include "Value.h"

namespace Zelo {

// 前向声明
class Expr;
class Stmt;

using ExprPtr = std::shared_ptr<Expr>;
using StmtPtr = std::shared_ptr<Stmt>;

// 类型注解
struct TypeAnnotation {
    std::vector<TokenType> types; // 支持联合类型，如 int|float
    bool isArray = false;
    bool isDict = false;
    std::shared_ptr<TypeAnnotation> keyType; // 用于字典的键类型
    std::shared_ptr<TypeAnnotation> valueType; // 用于字典的值类型或数组的元素类型
};

// 表达式访问者接口
class ExprVisitor {
public:
    virtual ~ExprVisitor() = default;
    virtual Value visitLiteralExpr(const LiteralExpr& expr) = 0;
    virtual Value visitIdentifierExpr(const IdentifierExpr& expr) = 0;
    virtual Value visitBinaryExpr(const BinaryExpr& expr) = 0;
    virtual Value visitUnaryExpr(const UnaryExpr& expr) = 0;
    virtual Value visitArrayExpr(const ArrayExpr& expr) = 0;
    virtual Value visitDictExpr(const DictExpr& expr) = 0;
    virtual Value visitCallExpr(const CallExpr& expr) = 0;
    virtual Value visitMemberExpr(const MemberExpr& expr) = 0;
    virtual Value visitIndexExpr(const IndexExpr& expr) = 0;
    virtual Value visitSliceExpr(const SliceExpr& expr) = 0;
    virtual Value visitConditionalExpr(const ConditionalExpr& expr) = 0;
    virtual Value visitAssignExpr(const AssignExpr& expr) = 0;
    virtual Value visitCastExpr(const CastExpr& expr) = 0;
};

// 语句访问者接口
class StmtVisitor {
public:
    virtual ~StmtVisitor() = default;
    virtual void visitExprStmt(const ExprStmt& stmt) = 0;
    virtual void visitBlockStmt(const BlockStmt& stmt) = 0;
    virtual void visitVarDeclStmt(const VarDeclStmt& stmt) = 0;
    virtual void visitFunctionDeclStmt(const FunctionDeclStmt& stmt) = 0;
    virtual void visitClassDeclStmt(const ClassDeclStmt& stmt) = 0;
    virtual void visitIfStmt(const IfStmt& stmt) = 0;
    virtual void visitWhileStmt(const WhileStmt& stmt) = 0;
    virtual void visitForStmt(const ForStmt& stmt) = 0;
    virtual void visitReturnStmt(const ReturnStmt& stmt) = 0;
    virtual void visitBreakStmt(const BreakStmt& stmt) = 0;
    virtual void visitContinueStmt(const ContinueStmt& stmt) = 0;
    virtual void visitImportStmt(const ImportStmt& stmt) = 0;
    virtual void visitExportStmt(const ExportStmt& stmt) = 0;
    virtual void visitTryCatchStmt(const TryCatchStmt& stmt) = 0;
    virtual void visitThrowStmt(const ThrowStmt& stmt) = 0;
};

// 表达式基类
class Expr {
public:
    virtual ~Expr() = default;
    virtual Value accept(ExprVisitor& visitor) = 0;
    virtual std::string type() const { return "Expr"; }
};

// 字面量表达式
class LiteralExpr : public Expr {
public:
    Token value;
    LiteralExpr(const Token& value) : value(value) {}
    Value accept(ExprVisitor& visitor) override { return visitor.visitLiteralExpr(*this); }
    std::string type() const override { return "LiteralExpr"; }
};

// 标识符表达式
class IdentifierExpr : public Expr {
public:
    Token name;
    IdentifierExpr(const Token& name) : name(name) {}
    Value accept(ExprVisitor& visitor) override { return visitor.visitIdentifierExpr(*this); }
    std::string type() const override { return "IdentifierExpr"; }
};

// 二元运算符表达式
class BinaryExpr : public Expr {
public:
    ExprPtr left;
    Token op;
    ExprPtr right;
    BinaryExpr(ExprPtr left, const Token& op, ExprPtr right)
        : left(std::move(left)), op(op), right(std::move(right)) {}
    Value accept(ExprVisitor& visitor) override { return visitor.visitBinaryExpr(*this); }
    std::string type() const override { return "BinaryExpr"; }
};

// 一元运算符表达式
class UnaryExpr : public Expr {
public:
    Token op;
    ExprPtr operand;
    UnaryExpr(const Token& op, ExprPtr operand)
        : op(op), operand(std::move(operand)) {}
    Value accept(ExprVisitor& visitor) override { return visitor.visitUnaryExpr(*this); }
    std::string type() const override { return "UnaryExpr"; }
};

// 数组表达式
class ArrayExpr : public Expr {
public:
    std::vector<ExprPtr> elements;
    ArrayExpr(std::vector<ExprPtr> elements) : elements(std::move(elements)) {}
    Value accept(ExprVisitor& visitor) override { return visitor.visitArrayExpr(*this); }
    std::string type() const override { return "ArrayExpr"; }
};

// 字典表达式
class DictExpr : public Expr {
public:
    std::vector<std::pair<ExprPtr, ExprPtr>> entries;
    DictExpr(std::vector<std::pair<ExprPtr, ExprPtr>> entries) : entries(std::move(entries)) {}
    Value accept(ExprVisitor& visitor) override { return visitor.visitDictExpr(*this); }
    std::string type() const override { return "DictExpr"; }
};

// 函数调用表达式
class CallExpr : public Expr {
public:
    ExprPtr callee;
    std::vector<ExprPtr> arguments;
    CallExpr(ExprPtr callee, std::vector<ExprPtr> arguments)
        : callee(std::move(callee)), arguments(std::move(arguments)) {}
    Value accept(ExprVisitor& visitor) override { return visitor.visitCallExpr(*this); }
    std::string type() const override { return "CallExpr"; }
};

// 成员访问表达式
class MemberExpr : public Expr {
public:
    ExprPtr object;
    Token property;
    MemberExpr(ExprPtr object, const Token& property)
        : object(std::move(object)), property(property) {}
    Value accept(ExprVisitor& visitor) override { return visitor.visitMemberExpr(*this); }
    std::string type() const override { return "MemberExpr"; }
};

// 索引访问表达式
class IndexExpr : public Expr {
public:
    ExprPtr object;
    ExprPtr index;
    IndexExpr(ExprPtr object, ExprPtr index)
        : object(std::move(object)), index(std::move(index)) {}
    Value accept(ExprVisitor& visitor) override { return visitor.visitIndexExpr(*this); }
    std::string type() const override { return "IndexExpr"; }
};

// 切片表达式
class SliceExpr : public Expr {
public:
    ExprPtr object;
    ExprPtr start;
    ExprPtr stop;
    ExprPtr step;
    SliceExpr(ExprPtr object, ExprPtr start, ExprPtr stop, ExprPtr step)
        : object(std::move(object)), start(std::move(start)), stop(std::move(stop)), step(std::move(step)) {}
    Value accept(ExprVisitor& visitor) override { return visitor.visitSliceExpr(*this); }
    std::string type() const override { return "SliceExpr"; }
};

// 条件表达式（三元运算符）
class ConditionalExpr : public Expr {
public:
    ExprPtr condition;
    ExprPtr thenExpr;
    ExprPtr elseExpr;
    ConditionalExpr(ExprPtr condition, ExprPtr thenExpr, ExprPtr elseExpr)
        : condition(std::move(condition)), thenExpr(std::move(thenExpr)), elseExpr(std::move(elseExpr)) {}
    Value accept(ExprVisitor& visitor) override { return visitor.visitConditionalExpr(*this); }
    std::string type() const override { return "ConditionalExpr"; }
};

// 赋值表达式
class AssignExpr : public Expr {
public:
    ExprPtr target;
    Token op;
    ExprPtr value;
    AssignExpr(ExprPtr target, const Token& op, ExprPtr value)
        : target(std::move(target)), op(op), value(std::move(value)) {}
    Value accept(ExprVisitor& visitor) override { return visitor.visitAssignExpr(*this); }
    std::string type() const override { return "AssignExpr"; }
};

// 类型转换表达式
class CastExpr : public Expr {
public:
    ExprPtr expression;
    TypeAnnotation type;
    CastExpr(ExprPtr expression, const TypeAnnotation& type)
        : expression(std::move(expression)), type(type) {}
    Value accept(ExprVisitor& visitor) override { return visitor.visitCastExpr(*this); }
    std::string type() const override { return "CastExpr"; }
};

// 语句基类
class Stmt {
public:
    virtual ~Stmt() = default;
    virtual void accept(StmtVisitor& visitor) = 0;
};

// 表达式语句
class ExprStmt : public Stmt {
public:
    ExprPtr expression;
    ExprStmt(ExprPtr expression) : expression(std::move(expression)) {}
    void accept(StmtVisitor& visitor) override { visitor.visitExprStmt(*this); }
};

// 块语句
class BlockStmt : public Stmt {
public:
    std::vector<StmtPtr> statements;
    BlockStmt(std::vector<StmtPtr> statements) : statements(std::move(statements)) {}
    void accept(StmtVisitor& visitor) override { visitor.visitBlockStmt(*this); }
};

// 变量声明语句
class VarDeclStmt : public Stmt {
public:
    Token name;
    TypeAnnotation type; // 可选的类型注解
    ExprPtr initializer;
    bool isConst;
    VarDeclStmt(const Token& name, const TypeAnnotation& type, ExprPtr initializer, bool isConst)
        : name(name), type(type), initializer(std::move(initializer)), isConst(isConst) {}
    void accept(StmtVisitor& visitor) override { visitor.visitVarDeclStmt(*this); }
};

// 函数声明语句
class FunctionDeclStmt : public Stmt {
public:
    Token name;
    std::vector<std::pair<Token, TypeAnnotation>> parameters; // 参数名和类型注解
    TypeAnnotation returnType;
    StmtPtr body;
    FunctionDeclStmt(const Token& name, 
                     std::vector<std::pair<Token, TypeAnnotation>> parameters, 
                     const TypeAnnotation& returnType, 
                     StmtPtr body)
        : name(name), parameters(std::move(parameters)), returnType(returnType), body(std::move(body)) {}
    void accept(StmtVisitor& visitor) override { visitor.visitFunctionDeclStmt(*this); }
};

// 类声明语句
class ClassDeclStmt : public Stmt {
public:
    Token name;
    Token superclass; // 基类名
    std::vector<StmtPtr> body;
    ClassDeclStmt(const Token& name, const Token& superclass, std::vector<StmtPtr> body)
        : name(name), superclass(superclass), body(std::move(body)) {}
    void accept(StmtVisitor& visitor) override { visitor.visitClassDeclStmt(*this); }
};

// 控制流语句（if/while/for等）
class IfStmt : public Stmt {
public:
    ExprPtr condition;
    StmtPtr thenBranch;
    StmtPtr elseBranch;
    IfStmt(ExprPtr condition, StmtPtr thenBranch, StmtPtr elseBranch)
        : condition(std::move(condition)), thenBranch(std::move(thenBranch)), elseBranch(std::move(elseBranch)) {}
    void accept(StmtVisitor& visitor) override { visitor.visitIfStmt(*this); }
};

class WhileStmt : public Stmt {
public:
    ExprPtr condition;
    StmtPtr body;
    WhileStmt(ExprPtr condition, StmtPtr body)
        : condition(std::move(condition)), body(std::move(body)) {}
    void accept(StmtVisitor& visitor) override { visitor.visitWhileStmt(*this); }
};

class ForStmt : public Stmt {
public:
    Token variable;
    ExprPtr iterable;
    StmtPtr body;
    ForStmt(const Token& variable, ExprPtr iterable, StmtPtr body)
        : variable(variable), iterable(std::move(iterable)), body(std::move(body)) {}
    void accept(StmtVisitor& visitor) override { visitor.visitForStmt(*this); }
};

class ReturnStmt : public Stmt {
public:
    ExprPtr value;
    ReturnStmt(ExprPtr value) : value(std::move(value)) {}
    void accept(StmtVisitor& visitor) override { visitor.visitReturnStmt(*this); }
};

class BreakStmt : public Stmt {
public:
    BreakStmt() = default;
    void accept(StmtVisitor& visitor) override { visitor.visitBreakStmt(*this); }
};

class ContinueStmt : public Stmt {
public:
    ContinueStmt() = default;
    void accept(StmtVisitor& visitor) override { visitor.visitContinueStmt(*this); }
};

class ImportStmt : public Stmt {
public:
    Token moduleName;
    std::vector<Token> imports;
    Token alias;
    ImportStmt(const Token& moduleName, std::vector<Token> imports, const Token& alias)
        : moduleName(moduleName), imports(std::move(imports)), alias(alias) {}
    void accept(StmtVisitor& visitor) override { visitor.visitImportStmt(*this); }
};

class ExportStmt : public Stmt {
public:
    std::vector<Token> exports;
    ExportStmt(std::vector<Token> exports) : exports(std::move(exports)) {}
    void accept(StmtVisitor& visitor) override { visitor.visitExportStmt(*this); }
};

class TryCatchStmt : public Stmt {
public:
    StmtPtr tryBlock;
    Token catchVar;
    TypeAnnotation catchType;
    StmtPtr catchBlock;
    TryCatchStmt(StmtPtr tryBlock, const Token& catchVar, const TypeAnnotation& catchType, StmtPtr catchBlock)
        : tryBlock(std::move(tryBlock)), catchVar(catchVar), catchType(catchType), catchBlock(std::move(catchBlock)) {}
    void accept(StmtVisitor& visitor) override { visitor.visitTryCatchStmt(*this); }
};

class ThrowStmt : public Stmt {
public:
    ExprPtr expression;
    ThrowStmt(ExprPtr expression) : expression(std::move(expression)) {}
    void accept(StmtVisitor& visitor) override { visitor.visitThrowStmt(*this); }
};

} // namespace Zelo