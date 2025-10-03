#pragma once

#include "AST.h"
#include "Value.h"
#include "ModuleSystem.h"
#include "GarbageCollector.h"
#include <memory>
#include <unordered_map>

namespace Zelo {

class Interpreter : public ExprVisitor, public StmtVisitor {
public:
    Interpreter();
    
    void interpret(const std::vector<StmtPtr>& statements);
    void executeBlock(const std::vector<StmtPtr>& statements, std::shared_ptr<Environment> environment);
    
    // 新增方法
    std::shared_ptr<Environment> getGlobals() { return globals; }
    std::shared_ptr<Environment> getEnvironment() { return environment; }
    ModuleSystem& getModuleSystem() { return moduleSystem; }
    
    void collectGarbage();
    
    // 表达式访问者方法
    Value visitLiteralExpr(const LiteralExpr& expr) override;
    Value visitIdentifierExpr(const IdentifierExpr& expr) override;
    Value visitBinaryExpr(const BinaryExpr& expr) override;
    Value visitUnaryExpr(const UnaryExpr& expr) override;
    Value visitArrayExpr(const ArrayExpr& expr) override;
    Value visitDictExpr(const DictExpr& expr) override;
    Value visitCallExpr(const CallExpr& expr) override;
    Value visitMemberExpr(const MemberExpr& expr) override;
    Value visitIndexExpr(const IndexExpr& expr) override;
    Value visitSliceExpr(const SliceExpr& expr) override;
    Value visitConditionalExpr(const ConditionalExpr& expr) override;
    Value visitAssignExpr(const AssignExpr& expr) override;
    Value visitCastExpr(const CastExpr& expr) override;
    
    // 语句访问者方法
    void visitExprStmt(const ExprStmt& stmt) override;
    void visitBlockStmt(const BlockStmt& stmt) override;
    void visitVarDeclStmt(const VarDeclStmt& stmt) override;
    void visitFunctionDeclStmt(const FunctionDeclStmt& stmt) override;
    void visitClassDeclStmt(const ClassDeclStmt& stmt) override;
    void visitIfStmt(const IfStmt& stmt) override;
    void visitWhileStmt(const WhileStmt& stmt) override;
    void visitForStmt(const ForStmt& stmt) override;
    void visitReturnStmt(const ReturnStmt& stmt) override;
    void visitBreakStmt(const BreakStmt& stmt) override;
    void visitContinueStmt(const ContinueStmt& stmt) override;
    void visitImportStmt(const ImportStmt& stmt) override;
    void visitExportStmt(const ExportStmt& stmt) override;
    void visitTryCatchStmt(const TryCatchStmt& stmt) override;
    void visitThrowStmt(const ThrowStmt& stmt) override;
    
private:
    std::shared_ptr<Environment> globals;
    std::shared_ptr<Environment> environment;
    ModuleSystem moduleSystem;
    
    Value evaluate(const ExprPtr& expr);
    void execute(const StmtPtr& stmt);
    
    // 运算符重载辅助函数
    std::string getOperatorMethodName(TokenType op);
    Value evaluateOperatorOverload(const std::shared_ptr<ZeloObject>& object, 
                                 const std::string& methodName, 
                                 const std::vector<Value>& arguments);
    
    // 类型检查辅助函数
    bool checkType(const Value& value, const TypeAnnotation& type);
    Value castValue(const Value& value, const TypeAnnotation& type);
    
    // 内置函数
    void defineBuiltinFunctions();
    
    // 容器克隆支持
    void addContainerCloneSupport();
};

} // namespace Zelo