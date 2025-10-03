#include "Interpreter.h"
#include "BuiltinFunctions.h"
#include "ErrorCode.h"
#include <iostream>
#include <cmath>
#include <algorithm>

namespace Zelo {

Interpreter::Interpreter() : moduleSystem(this) {
    globals = std::make_shared<Environment>();
    environment = globals;
    
    // 初始化标准库
    BuiltinFunctions::initialize(globals);
    addContainerCloneSupport();
}

void Interpreter::interpret(const std::vector<StmtPtr>& statements) {
    try {
        for (const auto& statement : statements) {
            execute(statement);
        }
        
        // 定期垃圾回收
        collectGarbage();
    } catch (const RuntimeError& error) {
        std::cerr << "Runtime error [" << error.getCode() << "]: " << error.what() 
                  << " at line " << error.getLine() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

void Interpreter::executeBlock(const std::vector<StmtPtr>& statements, std::shared_ptr<Environment> blockEnvironment) {
    auto previous = environment;
    try {
        environment = blockEnvironment;
        for (const auto& statement : statements) {
            execute(statement);
        }
    } catch (...) {
        environment = previous;
        throw;
    }
    environment = previous;
}

void Interpreter::collectGarbage() {
    // 简单的垃圾回收策略：每执行1000条语句后执行一次垃圾回收
    static int statementCount = 0;
    statementCount++;
    
    if (statementCount >= 1000) {
        statementCount = 0;
        GarbageCollector::getInstance().collectGarbage(globals);
    }
}

Value Interpreter::visitLiteralExpr(const LiteralExpr& expr) {
    switch (expr.value.type) {
        case TokenType::NUMBER:
            if (expr.value.value.find('.') != std::string::npos) {
                return std::stod(expr.value.value);
            } else {
                return std::stoi(expr.value.value);
            }
        case TokenType::STRING:
            return expr.value.value;
        case TokenType::TRUE:
            return true;
        case TokenType::FALSE:
            return false;
        case TokenType::NULL_KEYWORD:
            return Value();
        default:
            return Value();
    }
}

Value Interpreter::visitIdentifierExpr(const IdentifierExpr& expr) {
    return environment->get(expr.name.value);
}

Value Interpreter::visitBinaryExpr(const BinaryExpr& expr) {
    Value left = evaluate(expr.left);
    Value right = evaluate(expr.right);
    
    // 检查左操作数是否有运算符重载方法
    if (std::holds_alternative<std::shared_ptr<ZeloObject>>(left)) {
        auto leftObj = std::get<std::shared_ptr<ZeloObject>>(left);
        std::string opMethodName = getOperatorMethodName(expr.op.type);
        
        if (auto method = leftObj->klass->findMethod(opMethodName)) {
            // 绑定方法到对象
            auto boundMethod = std::make_shared<ZeloFunction>(
                method->declaration,
                std::make_shared<Environment>(method->closure)
            );
            boundMethod->closure->define("this", leftObj);
            return boundMethod->call(this, {right});
        }
    }
    
    // 原有的基本类型运算逻辑
    switch (expr.op.type) {
        case TokenType::PLUS:
            if (std::holds_alternative<int>(left) && std::holds_alternative<int>(right)) {
                return std::get<int>(left) + std::get<int>(right);
            }
            if (std::holds_alternative<double>(left) && std::holds_alternative<double>(right)) {
                return std::get<double>(left) + std::get<double>(right);
            }
            if (std::holds_alternative<int>(left) && std::holds_alternative<double>(right)) {
                return std::get<int>(left) + std::get<double>(right);
            }
            if (std::holds_alternative<double>(left) && std::holds_alternative<int>(right)) {
                return std::get<double>(left) + std::get<int>(right);
            }
            if (std::holds_alternative<std::string>(left) || std::holds_alternative<std::string>(right)) {
                return valueToString(left) + valueToString(right);
            }
            throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                       "Operands must be two numbers or at least one string", 
                                       expr.op.line);
        
        case TokenType::MINUS:
            checkNumberOperands(expr.op, left, right);
            if (std::holds_alternative<int>(left) && std::holds_alternative<int>(right)) {
                return std::get<int>(left) - std::get<int>(right);
            }
            return toDouble(left) - toDouble(right);
        
        case TokenType::MULTIPLY:
            checkNumberOperands(expr.op, left, right);
            if (std::holds_alternative<int>(left) && std::holds_alternative<int>(right)) {
                return std::get<int>(left) * std::get<int>(right);
            }
            return toDouble(left) * toDouble(right);
        
        case TokenType::DIVIDE:
            checkNumberOperands(expr.op, left, right);
            if (toDouble(right) == 0.0) {
                throw RuntimeError::fromCode(ErrorCode::DIVISION_BY_ZERO, 
                                           "Division by zero", 
                                           expr.op.line);
            }
            return toDouble(left) / toDouble(right);
        
        case TokenType::MODULO:
            checkNumberOperands(expr.op, left, right);
            if (toDouble(right) == 0.0) {
                throw RuntimeError::fromCode(ErrorCode::DIVISION_BY_ZERO, 
                                           "Division by zero", 
                                           expr.op.line);
            }
            return std::fmod(toDouble(left), toDouble(right));
        
        case TokenType::EQUAL:
            return isEqual(left, right);
        
        case TokenType::NOT_EQUAL:
            return !isEqual(left, right);
        
        case TokenType::LESS:
            checkNumberOperands(expr.op, left, right);
            return toDouble(left) < toDouble(right);
        
        case TokenType::LESS_EQUAL:
            checkNumberOperands(expr.op, left, right);
            return toDouble(left) <= toDouble(right);
        
        case TokenType::GREATER:
            checkNumberOperands(expr.op, left, right);
            return toDouble(left) > toDouble(right);
        
        case TokenType::GREATER_EQUAL:
            checkNumberOperands(expr.op, left, right);
            return toDouble(left) >= toDouble(right);
        
        case TokenType::BIT_AND:
            if (std::holds_alternative<int>(left) && std::holds_alternative<int>(right)) {
                return std::get<int>(left) & std::get<int>(right);
            }
            throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                       "Operands must be integers", 
                                       expr.op.line);
        
        case TokenType::BIT_OR:
            if (std::holds_alternative<int>(left) && std::holds_alternative<int>(right)) {
                return std::get<int>(left) | std::get<int>(right);
            }
            throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                       "Operands must be integers", 
                                       expr.op.line);
        
        case TokenType::BIT_XOR:
            if (std::holds_alternative<int>(left) && std::holds_alternative<int>(right)) {
                return std::get<int>(left) ^ std::get<int>(right);
            }
            throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                       "Operands must be integers", 
                                       expr.op.line);
        
        case TokenType::LSHIFT:
            if (std::holds_alternative<int>(left) && std::holds_alternative<int>(right)) {
                return std::get<int>(left) << std::get<int>(right);
            }
            throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                       "Operands must be integers", 
                                       expr.op.line);
        
        case TokenType::RSHIFT:
            if (std::holds_alternative<int>(left) && std::holds_alternative<int>(right)) {
                return std::get<int>(left) >> std::get<int>(right);
            }
            throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                       "Operands must be integers", 
                                       expr.op.line);
        
        default:
            throw RuntimeError::fromCode(ErrorCode::SYNTAX_ERROR, 
                                       "Unknown binary operator", 
                                       expr.op.line);
    }
}

Value Interpreter::visitUnaryExpr(const UnaryExpr& expr) {
    Value right = evaluate(expr.operand);
    
    switch (expr.op.type) {
        case TokenType::MINUS:
            checkNumberOperand(expr.op, right);
            if (std::holds_alternative<int>(right)) {
                return -std::get<int>(right);
            }
            return -std::get<double>(right);
        
        case TokenType::NOT:
            return !isTruthy(right);
        
        case TokenType::BIT_NOT:
            if (std::holds_alternative<int>(right)) {
                return ~std::get<int>(right);
            }
            throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                       "Operand must be an integer", 
                                       expr.op.line);
        
        case TokenType::INCREMENT:
            if (std::holds_alternative<int>(right)) {
                return std::get<int>(right) + 1;
            }
            if (std::holds_alternative<double>(right)) {
                return std::get<double>(right) + 1.0;
            }
            throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                       "Operand must be a number", 
                                       expr.op.line);
        
        case TokenType::DECREMENT:
            if (std::holds_alternative<int>(right)) {
                return std::get<int>(right) - 1;
            }
            if (std::holds_alternative<double>(right)) {
                return std::get<double>(right) - 1.0;
            }
            throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                       "Operand must be a number", 
                                       expr.op.line);
        
        default:
            throw RuntimeError::fromCode(ErrorCode::SYNTAX_ERROR, 
                                       "Unknown unary operator", 
                                       expr.op.line);
    }
}

Value Interpreter::visitArrayExpr(const ArrayExpr& expr) {
    auto array = std::make_shared<std::vector<Value>>();
    for (const auto& element : expr.elements) {
        array->push_back(evaluate(element));
    }
    return array;
}

Value Interpreter::visitDictExpr(const DictExpr& expr) {
    auto dict = std::make_shared<std::unordered_map<std::string, Value>>();
    for (const auto& [keyExpr, valueExpr] : expr.entries) {
        Value keyValue = evaluate(keyExpr);
        if (!std::holds_alternative<std::string>(keyValue)) {
            throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                       "Dictionary key must be a string", 
                                       0);
        }
        std::string key = std::get<std::string>(keyValue);
        (*dict)[key] = evaluate(valueExpr);
    }
    return dict;
}

Value Interpreter::visitCallExpr(const CallExpr& expr) {
    Value callee = evaluate(expr.callee);
    std::vector<Value> arguments;
    for (const auto& arg : expr.arguments) {
        arguments.push_back(evaluate(arg));
    }
    
    if (std::holds_alternative<std::shared_ptr<ZeloFunction>>(callee)) {
        auto function = std::get<std::shared_ptr<ZeloFunction>>(callee);
        if (arguments.size() != function->arity()) {
            throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                       "Expected " + std::to_string(function->arity()) + 
                                       " arguments but got " + std::to_string(arguments.size()), 
                                       0);
        }
        return function->call(this, arguments);
    }
    
    if (std::holds_alternative<std::shared_ptr<ZeloClass>>(callee)) {
        auto klass = std::get<std::shared_ptr<ZeloClass>>(callee);
        if (arguments.size() != klass->arity()) {
            throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                       "Expected " + std::to_string(klass->arity()) + 
                                       " arguments but got " + std::to_string(arguments.size()), 
                                       0);
        }
        return klass->call(this, arguments);
    }
    
    throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                               "Can only call functions and classes", 
                               0);
}

Value Interpreter::visitMemberExpr(const MemberExpr& expr) {
    Value object = evaluate(expr.object);
    if (std::holds_alternative<std::shared_ptr<ZeloObject>>(object)) {
        return std::get<std::shared_ptr<ZeloObject>>(object)->get(expr.property.value);
    }
    throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                               "Only objects have properties", 
                               expr.property.line);
}

Value Interpreter::visitIndexExpr(const IndexExpr& expr) {
    Value object = evaluate(expr.object);
    Value index = evaluate(expr.index);
    
    if (std::holds_alternative<std::shared_ptr<std::vector<Value>>>(object)) {
        auto array = std::get<std::shared_ptr<std::vector<Value>>>(object);
        if (!std::holds_alternative<int>(index)) {
            throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                       "Array index must be an integer", 
                                       0);
        }
        int idx = std::get<int>(index);
        
        // 处理负索引
        if (idx < 0) {
            idx = array->size() + idx;
            if (idx < 0) {
                throw RuntimeError::fromCode(ErrorCode::INDEX_OUT_OF_BOUNDS, 
                                           "Array index out of bounds", 
                                           0);
            }
        }
        
        if (idx < 0 || idx >= array->size()) {
            throw RuntimeError::fromCode(ErrorCode::INDEX_OUT_OF_BOUNDS, 
                                       "Array index out of bounds", 
                                       0);
        }
        return (*array)[idx];
    }
    
    if (std::holds_alternative<std::shared_ptr<std::unordered_map<std::string, Value>>>(object)) {
        auto dict = std::get<std::shared_ptr<std::unordered_map<std::string, Value>>>(object);
        if (!std::holds_alternative<std::string>(index)) {
            throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                       "Dictionary key must be a string", 
                                       0);
        }
        std::string key = std::get<std::string>(index);
        auto it = dict->find(key);
        if (it == dict->end()) {
            throw RuntimeError::fromCode(ErrorCode::KEY_NOT_FOUND, 
                                       "Key '" + key + "' not found in dictionary", 
                                       0);
        }
        return it->second;
    }
    
    throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                               "Only arrays and dictionaries can be indexed", 
                               0);
}

Value Interpreter::visitSliceExpr(const SliceExpr& expr) {
    Value object = evaluate(expr.object);
    if (!std::holds_alternative<std::shared_ptr<std::vector<Value>>>(object)) {
        throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                   "Slice operation only supported for arrays", 
                                   0);
    }
    
    auto array = std::get<std::shared_ptr<std::vector<Value>>>(object);
    int size = static_cast<int>(array->size());
    
    // 处理起始索引
    int start = 0;
    if (expr.start) {
        Value startValue = evaluate(expr.start);
        if (!std::holds_alternative<int>(startValue)) {
            throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                       "Slice start must be an integer", 
                                       0);
        }
        start = std::get<int>(startValue);
        // 处理负索引
        if (start < 0) {
            start = size + start;
            if (start < 0) start = 0;
        }
        start = std::min(start, size);
    }
    
    // 处理结束索引
    int stop = size;
    if (expr.stop) {
        Value stopValue = evaluate(expr.stop);
        if (!std::holds_alternative<int>(stopValue)) {
            throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                       "Slice stop must be an integer", 
                                       0);
        }
        stop = std::get<int>(stopValue);
        // 处理负索引
        if (stop < 0) {
            stop = size + stop;
            if (stop < 0) stop = 0;
        }
        stop = std::min(stop, size);
    }
    
    // 处理步长
    int step = 1;
    if (expr.step) {
        Value stepValue = evaluate(expr.step);
        if (!std::holds_alternative<int>(stepValue)) {
            throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                       "Slice step must be an integer", 
                                       0);
        }
        step = std::get<int>(stepValue);
        if (step == 0) {
            throw RuntimeError::fromCode(ErrorCode::INVALID_OPERATION, 
                                       "Slice step cannot be zero", 
                                       0);
        }
    }
    
    // 创建切片结果
    auto result = std::make_shared<std::vector<Value>>();
    
    if (step > 0) {
        // 正向切片
        for (int i = start; i < stop; i += step) {
            if (i >= 0 && i < size) {
                result->push_back((*array)[i]);
            }
        }
    } else {
        // 反向切片
        for (int i = start; i > stop; i += step) {
            if (i >= 0 && i < size) {
                result->push_back((*array)[i]);
            }
        }
    }
    
    return result;
}

Value Interpreter::visitConditionalExpr(const ConditionalExpr& expr) {
    Value condition = evaluate(expr.condition);
    if (isTruthy(condition)) {
        return evaluate(expr.thenExpr);
    } else {
        return evaluate(expr.elseExpr);
    }
}

Value Interpreter::visitAssignExpr(const AssignExpr& expr) {
    Value value = evaluate(expr.value);
    
    if (auto identifier = std::dynamic_pointer_cast<IdentifierExpr>(expr.target)) {
        environment->assign(identifier->name.value, value);
        return value;
    }
    
    if (auto member = std::dynamic_pointer_cast<MemberExpr>(expr.target)) {
        Value object = evaluate(member->object);
        if (std::holds_alternative<std::shared_ptr<ZeloObject>>(object)) {
            std::get<std::shared_ptr<ZeloObject>>(object)->set(member->property.value, value);
            return value;
        }
        throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                   "Only objects have properties", 
                                   member->property.line);
    }
    
    if (auto index = std::dynamic_pointer_cast<IndexExpr>(expr.target)) {
        Value object = evaluate(index->object);
        Value indexValue = evaluate(index->index);
        
        if (std::holds_alternative<std::shared_ptr<std::vector<Value>>>(object)) {
            auto array = std::get<std::shared_ptr<std::vector<Value>>>(object);
            if (!std::holds_alternative<int>(indexValue)) {
                throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                           "Array index must be an integer", 
                                           0);
            }
            int idx = std::get<int>(indexValue);
            if (idx < 0) idx = array->size() + idx;
            if (idx < 0 || idx >= array->size()) {
                throw RuntimeError::fromCode(ErrorCode::INDEX_OUT_OF_BOUNDS, 
                                           "Array index out of bounds", 
                                           0);
            }
            (*array)[idx] = value;
            return value;
        }
        
        if (std::holds_alternative<std::shared_ptr<std::unordered_map<std::string, Value>>>(object)) {
            auto dict = std::get<std::shared_ptr<std::unordered_map<std::string, Value>>>(object);
            if (!std::holds_alternative<std::string>(indexValue)) {
                throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                           "Dictionary key must be a string", 
                                           0);
            }
            std::string key = std::get<std::string>(indexValue);
            (*dict)[key] = value;
            return value;
        }
        
        throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                   "Only arrays and dictionaries can be indexed", 
                                   0);
    }
    
    throw RuntimeError::fromCode(ErrorCode::SYNTAX_ERROR, 
                               "Invalid assignment target", 
                               0);
}

Value Interpreter::visitCastExpr(const CastExpr& expr) {
    Value value = evaluate(expr.expression);
    // 这里需要实现类型转换逻辑
    // 由于时间关系，我们先简单返回原值
    return value;
}

void Interpreter::visitExprStmt(const ExprStmt& stmt) {
    evaluate(stmt.expression);
}

void Interpreter::visitBlockStmt(const BlockStmt& stmt) {
    executeBlock(stmt.statements, std::make_shared<Environment>(environment));
}

void Interpreter::visitVarDeclStmt(const VarDeclStmt& stmt) {
    Value value;
    if (stmt.initializer) {
        value = evaluate(stmt.initializer);
        
        // 检查类型注解
        if (!stmt.type.types.empty() || stmt.type.isArray || stmt.type.isDict) {
            if (!checkType(value, stmt.type)) {
                throw RuntimeError::fromCode(ErrorCode::TYPE_MISMATCH, 
                                           "Type mismatch in variable declaration", 
                                           stmt.name.line);
            }
        }
    }
    environment->define(stmt.name.value, value);
}

void Interpreter::visitFunctionDeclStmt(const FunctionDeclStmt& stmt) {
    auto function = std::make_shared<ZeloFunction>(std::make_shared<FunctionDeclStmt>(stmt), environment);
    environment->define(stmt.name.value, function);
}

void Interpreter::visitClassDeclStmt(const ClassDeclStmt& stmt) {
    std::shared_ptr<ZeloClass> superclass;
    if (!stmt.superclass.value.empty()) {
        Value superclassValue = environment->get(stmt.superclass.value);
        if (!std::holds_alternative<std::shared_ptr<ZeloClass>>(superclassValue)) {
            throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                       "Superclass must be a class", 
                                       stmt.superclass.line);
        }
        superclass = std::get<std::shared_ptr<ZeloClass>>(superclassValue);
    }
    
    environment->define(stmt.name.value, Value());
    
    std::unordered_map<std::string, std::shared_ptr<ZeloFunction>> methods;
    for (const auto& methodStmt : stmt.body) {
        if (auto function = std::dynamic_pointer_cast<FunctionDeclStmt>(methodStmt)) {
            bool isInitializer = function->name.value == "__init__";
            auto method = std::make_shared<ZeloFunction>(
                function, environment, isInitializer);
            methods[function->name.value] = method;
        }
    }
    
    auto klass = std::make_shared<ZeloClass>(stmt.name.value, superclass, methods);
    environment->assign(stmt.name.value, klass);
}

void Interpreter::visitIfStmt(const IfStmt& stmt) {
    Value condition = evaluate(stmt.condition);
    if (isTruthy(condition)) {
        execute(stmt.thenBranch);
    } else if (stmt.elseBranch) {
        execute(stmt.elseBranch);
    }
}

void Interpreter::visitWhileStmt(const WhileStmt& stmt) {
    while (isTruthy(evaluate(stmt.condition))) {
        try {
            execute(stmt.body);
        } catch (const BreakStmt&) {
            break;
        } catch (const ContinueStmt&) {
            continue;
        }
    }
}

void Interpreter::visitForStmt(const ForStmt& stmt) {
    Value iterable = evaluate(stmt.iterable);
    
    if (std::holds_alternative<std::shared_ptr<std::vector<Value>>>(iterable)) {
        auto array = std::get<std::shared_ptr<std::vector<Value>>>(iterable);
        for (const auto& element : *array) {
            environment->define(stmt.variable.value, element);
            try {
                execute(stmt.body);
            } catch (const BreakStmt&) {
                break;
            } catch (const ContinueStmt&) {
                continue;
            }
        }
    } else {
        throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                   "For loop requires an iterable object", 
                                   stmt.variable.line);
    }
}

void Interpreter::visitReturnStmt(const ReturnStmt& stmt) {
    Value value;
    if (stmt.value) {
        value = evaluate(stmt.value);
    }
    throw ReturnException(value);
}

void Interpreter::visitBreakStmt(const BreakStmt& stmt) {
    throw BreakStmt();
}

void Interpreter::visitContinueStmt(const ContinueStmt& stmt) {
    throw ContinueStmt();
}

void Interpreter::visitImportStmt(const ImportStmt& stmt) {
    auto& moduleSystem = getModuleSystem();
    
    if (stmt.imports.empty()) {
        // 简单导入: import "module"
        moduleSystem.requireModule(stmt.moduleName.value);
    } else {
        // 解构导入: import {a, b} from "module"
        moduleSystem.importModule(stmt.moduleName.value, stmt.imports, stmt.alias);
    }
}

void Interpreter::visitExportStmt(const ExportStmt& stmt) {
    // 创建或获取导出环境
    Value exportsValue = environment->get("__exports__");
    std::shared_ptr<Environment> exportsEnv;
    
    if (std::holds_alternative<std::shared_ptr<Environment>>(exportsValue)) {
        exportsEnv = std::get<std::shared_ptr<Environment>>(exportsValue);
    } else {
        exportsEnv = std::make_shared<Environment>();
        environment->define("__exports__", exportsEnv);
    }
    
    // 导出指定的符号
    for (const auto& exportItem : stmt.exports) {
        Value value = environment->get(exportItem.value);
        exportsEnv->define(exportItem.value, value);
    }
}

void Interpreter::visitTryCatchStmt(const TryCatchStmt& stmt) {
    try {
        execute(stmt.tryBlock);
    } catch (const RuntimeError& error) {
        if (!stmt.catchVar.value.empty()) {
            environment->define(stmt.catchVar.value, error.what());
        }
        execute(stmt.catchBlock);
    }
}

void Interpreter::visitThrowStmt(const ThrowStmt& stmt) {
    Value value = evaluate(stmt.expression);
    throw RuntimeError::fromCode(ErrorCode::RUNTIME_ERROR, 
                               valueToString(value), 
                               0);
}

Value Interpreter::evaluate(const ExprPtr& expr) {
    return expr->accept(*this);
}

void Interpreter::execute(const StmtPtr& stmt) {
    stmt->accept(*this);
}

std::string Interpreter::getOperatorMethodName(TokenType op) {
    static const std::unordered_map<TokenType, std::string> opMap = {
        {TokenType::PLUS, "__add__"},
        {TokenType::MINUS, "__sub__"},
        {TokenType::MULTIPLY, "__mul__"},
        {TokenType::DIVIDE, "__div__"},
        {TokenType::MODULO, "__mod__"},
        {TokenType::BIT_AND, "__and__"},
        {TokenType::BIT_OR, "__or__"},
        {TokenType::BIT_XOR, "__xor__"},
        {TokenType::LSHIFT, "__lshift__"},
        {TokenType::RSHIFT, "__rshift__"},
        {TokenType::EQUAL, "__eq__"},
        {TokenType::NOT_EQUAL, "__ne__"},
        {TokenType::LESS, "__lt__"},
        {TokenType::LESS_EQUAL, "__le__"},
        {TokenType::GREATER, "__gt__"},
        {TokenType::GREATER_EQUAL, "__ge__"},
        {TokenType::ASSIGN, "__set__"},
        {TokenType::LBRACKET, "__getitem__"},
        {TokenType::INCREMENT, "__inc__"},
        {TokenType::DECREMENT, "__dec__"},
        {TokenType::NOT, "__not__"},
        {TokenType::BIT_NOT, "__invert__"}
    };
    
    auto it = opMap.find(op);
    if (it != opMap.end()) {
        return it->second;
    }
    
    return "";
}

Value Interpreter::evaluateOperatorOverload(const std::shared_ptr<ZeloObject>& object, 
                                          const std::string& methodName, 
                                          const std::vector<Value>& arguments) {
    auto method = object->klass->findMethod(methodName);
    if (!method) {
        throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                   "Operator not supported for this type", 
                                   0);
    }
    
    // 绑定方法到对象
    auto boundMethod = std::make_shared<ZeloFunction>(
        method->declaration,
        std::make_shared<Environment>(method->closure)
    );
    boundMethod->closure->define("this", object);
    
    return boundMethod->call(this, arguments);
}

bool Interpreter::checkType(const Value& value, const TypeAnnotation& type) {
    // 使用 TypeSystem 进行类型检查
    return TypeSystem::checkType(value, type);
}

Value Interpreter::castValue(const Value& value, const TypeAnnotation& type) {
    // 使用 TypeSystem 进行类型转换
    return TypeSystem::castValue(value, type);
}

void Interpreter::defineBuiltinFunctions() {
    // 内置函数已经在 BuiltinFunctions::initialize 中定义
    // 这里不需要额外操作
}

void Interpreter::addContainerCloneSupport() {
    // 为数组和字典添加 __clone__ 方法支持
    // 这些方法已经在 BuiltinFunctions 中定义
}

} // namespace Zelo