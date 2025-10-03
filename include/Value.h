#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <variant>
#include <functional>
#include "AST.h"

namespace Zelo {

// 前向声明
class Interpreter;
class ZeloObject;
class ZeloFunction;
class ZeloClass;

// 定义包装器来解决循环依赖
struct Value;

using ValueArray = std::shared_ptr<std::vector<Value>>;
using ValueObject = std::shared_ptr<std::unordered_map<std::string, Value>>;

// 现在安全地定义 Value
using Value = std::variant<
    std::monostate,      // null
    int,                 // 整型
    double,              // 浮点型
    bool,                // 布尔型
    std::string,         // 字符串
    ValueArray,          // 数组
    ValueObject,         // 字典
    std::shared_ptr<ZeloObject>,
    std::shared_ptr<ZeloFunction>,
    std::shared_ptr<ZeloClass>
>;

// Value 的实际结构
struct Value {
    Value() : data(std::monostate{}) {}
    
    template<typename T>
    Value(T&& value) : data(std::forward<T>(value)) {}
    
    std::variant<
        std::monostate, int, double, bool, std::string,
        ValueArray, ValueObject,
        std::shared_ptr<ZeloObject>,
        std::shared_ptr<ZeloFunction>,
        std::shared_ptr<ZeloClass>
    > data;
};

// 对象实例
class ZeloObject {
public:
    std::shared_ptr<ZeloClass> klass;
    std::unordered_map<std::string, Value> fields;
    
    ZeloObject(std::shared_ptr<ZeloClass> klass) : klass(klass) {}
    
    Value get(const std::string& name);
    void set(const std::string& name, const Value& value);
};

// 函数
class ZeloFunction {
public:
    std::shared_ptr<FunctionDeclStmt> declaration;
    std::shared_ptr<Environment> closure;
    bool isConstructor;
    
    ZeloFunction(std::shared_ptr<FunctionDeclStmt> declaration, 
                 std::shared_ptr<Environment> closure, 
                 bool isConstructor = false)
        : declaration(declaration), closure(closure), isConstructor(isConstructor) {}
    
    Value call(Interpreter* interpreter, const std::vector<Value>& arguments);
    int arity() const;
};

// 类
class ZeloClass {
public:
    std::string name;
    std::shared_ptr<ZeloClass> superclass;
    std::unordered_map<std::string, std::shared_ptr<ZeloFunction>> methods;
    
    ZeloClass(const std::string& name, 
              std::shared_ptr<ZeloClass> superclass,
              const std::unordered_map<std::string, std::shared_ptr<ZeloFunction>>& methods)
        : name(name), superclass(superclass), methods(methods) {}
    
    std::shared_ptr<ZeloFunction> findMethod(const std::string& name);
    Value call(Interpreter* interpreter, const std::vector<Value>& arguments);
    int arity() const;
};

// 环境（作用域）
class Environment : public std::enable_shared_from_this<Environment> {
public:
    Environment() = default;
    explicit Environment(std::shared_ptr<Environment> enclosing) : enclosing(enclosing) {}
    
    void define(const std::string& name, const Value& value);
    void assign(const std::string& name, const Value& value);
    Value get(const std::string& name);
    Value getAt(int distance, const std::string& name);
    void assignAt(int distance, const std::string& name, const Value& value);
    
    std::shared_ptr<Environment> getEnclosing() const { return enclosing; }
    std::unordered_map<std::string, Value> getValues() const { return values; }
    
private:
    std::unordered_map<std::string, Value> values;
    std::shared_ptr<Environment> enclosing;
};

// 返回语句异常（用于实现return）
class ReturnException : public std::exception {
public:
    ReturnException(Value value) : value(value) {}
    Value value;
};

// 运行时错误
class RuntimeError : public std::runtime_error {
public:
    RuntimeError(const std::string& message, size_t line) 
        : std::runtime_error(message), line(line) {}
    size_t line;
};

// 工具函数
std::string valueToString(const Value& value);
bool isTruthy(const Value& value);
bool isEqual(const Value& a, const Value& b);
void checkNumberOperand(const Token& op, const Value& operand);
void checkNumberOperands(const Token& op, const Value& left, const Value& right);

} // namespace Zelo