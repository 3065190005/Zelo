#include "Value.h"
#include "Interpreter.h"
#include <sstream>
#include <cmath>
#include <algorithm>

namespace Zelo {

Value ZeloObject::get(const std::string& name) {
    auto it = fields.find(name);
    if (it != fields.end()) {
        return it->second;
    }
    
    // 如果在实例中找不到，尝试从类的方法中查找
    auto method = klass->findMethod(name);
    if (method) {
        // 绑定方法到当前对象
        auto boundMethod = std::make_shared<ZeloFunction>(
            method->declaration, 
            std::make_shared<Environment>(method->closure)
        );
        boundMethod->closure->define("this", shared_from_this());
        return boundMethod;
    }
    
    // 尝试调用 __getattr__ 方法
    auto getattr = klass->findMethod("__getattr__");
    if (getattr) {
        auto boundGetattr = std::make_shared<ZeloFunction>(
            getattr->declaration, 
            std::make_shared<Environment>(getattr->closure)
        );
        boundGetattr->closure->define("this", shared_from_this());
        return boundGetattr->call(nullptr, {name});
    }
    
    throw RuntimeError(ErrorCode::UNDEFINED_PROPERTY, "Undefined property '" + name + "'", 0);
}

void ZeloObject::set(const std::string& name, const Value& value) {
    // 尝试调用 __setattr__ 方法
    auto setattr = klass->findMethod("__setattr__");
    if (setattr) {
        auto boundSetattr = std::make_shared<ZeloFunction>(
            setattr->declaration, 
            std::make_shared<Environment>(setattr->closure)
        );
        boundSetattr->closure->define("this", shared_from_this());
        boundSetattr->call(nullptr, {name, value});
        return;
    }
    
    // 如果没有 __setattr__ 方法，直接设置字段
    fields[name] = value;
}

Value ZeloFunction::call(Interpreter* interpreter, const std::vector<Value>& arguments) {
    auto environment = std::make_shared<Environment>(closure);
    
    for (size_t i = 0; i < declaration->parameters.size(); i++) {
        environment->define(declaration->parameters[i].first.value, 
                           i < arguments.size() ? arguments[i] : Value());
    }
    
    try {
        interpreter->executeBlock(declaration->body, environment);
    } catch (const ReturnException& returnValue) {
        if (isConstructor) {
            return closure->getAt(0, "this");
        }
        return returnValue.value;
    }
    
    if (isConstructor) {
        return closure->getAt(0, "this");
    }
    return Value();
}

int ZeloFunction::arity() const {
    return declaration->parameters.size();
}

std::shared_ptr<ZeloFunction> ZeloClass::findMethod(const std::string& name) {
    auto it = methods.find(name);
    if (it != methods.end()) {
        return it->second;
    }
    
    if (superclass) {
        return superclass->findMethod(name);
    }
    
    return nullptr;
}

Value ZeloClass::call(Interpreter* interpreter, const std::vector<Value>& arguments) {
    auto instance = std::make_shared<ZeloObject>(shared_from_this());
    auto initializer = findMethod("__init__");
    if (initializer) {
        initializer->bind(instance)->call(interpreter, arguments);
    }
    return instance;
}

int ZeloClass::arity() const {
    auto initializer = findMethod("__init__");
    if (!initializer) return 0;
    return initializer->arity();
}

void Environment::define(const std::string& name, const Value& value) {
    values[name] = value;
}

void Environment::assign(const std::string& name, const Value& value) {
    auto it = values.find(name);
    if (it != values.end()) {
        it->second = value;
        return;
    }
    
    if (enclosing) {
        enclosing->assign(name, value);
        return;
    }
    
    throw RuntimeError(ErrorCode::UNDEFINED_VARIABLE, "Undefined variable '" + name + "'", 0);
}

Value Environment::get(const std::string& name) {
    auto it = values.find(name);
    if (it != values.end()) {
        return it->second;
    }
    
    if (enclosing) {
        return enclosing->get(name);
    }
    
    throw RuntimeError(ErrorCode::UNDEFINED_VARIABLE, "Undefined variable '" + name + "'", 0);
}

Value Environment::getAt(int distance, const std::string& name) {
    return ancestor(distance)->values[name];
}

void Environment::assignAt(int distance, const std::string& name, const Value& value) {
    ancestor(distance)->values[name] = value;
}

std::shared_ptr<Environment> Environment::ancestor(int distance) {
    auto environment = shared_from_this();
    for (int i = 0; i < distance; i++) {
        environment = environment->enclosing;
    }
    return environment;
}

std::string valueToString(const Value& value) {
    if (std::holds_alternative<std::monostate>(value)) {
        return "null";
    } else if (std::holds_alternative<int>(value)) {
        return std::to_string(std::get<int>(value));
    } else if (std::holds_alternative<double>(value)) {
        std::string str = std::to_string(std::get<double>(value));
        str.erase(str.find_last_not_of('0') + 1, std::string::npos);
        if (str.back() == '.') {
            str.pop_back();
        }
        return str;
    } else if (std::holds_alternative<bool>(value)) {
        return std::get<bool>(value) ? "true" : "false";
    } else if (std::holds_alternative<std::string>(value)) {
        return std::get<std::string>(value);
    } else if (std::holds_alternative<std::shared_ptr<std::vector<Value>>>(value)) {
        auto array = std::get<std::shared_ptr<std::vector<Value>>>(value);
        std::string result = "[";
        for (size_t i = 0; i < array->size(); i++) {
            if (i > 0) result += ", ";
            result += valueToString((*array)[i]);
        }
        return result + "]";
    } else if (std::holds_alternative<std::shared_ptr<std::unordered_map<std::string, Value>>>(value)) {
        auto dict = std::get<std::shared_ptr<std::unordered_map<std::string, Value>>>(value);
        std::string result = "{";
        bool first = true;
        for (const auto& [key, val] : *dict) {
            if (!first) result += ", ";
            first = false;
            result += key + ": " + valueToString(val);
        }
        return result + "}";
    } else if (std::holds_alternative<std::shared_ptr<ZeloObject>>(value)) {
        auto object = std::get<std::shared_ptr<ZeloObject>>(value);
        return "<object " + object->klass->name + ">";
    } else if (std::holds_alternative<std::shared_ptr<ZeloFunction>>(value)) {
        return "<function>";
    } else if (std::holds_alternative<std::shared_ptr<ZeloClass>>(value)) {
        auto klass = std::get<std::shared_ptr<ZeloClass>>(value);
        return "<class " + klass->name + ">";
    }
    
    return "unknown";
}

bool isTruthy(const Value& value) {
    if (std::holds_alternative<std::monostate>(value)) return false;
    if (std::holds_alternative<bool>(value)) return std::get<bool>(value);
    return true;
}

bool isEqual(const Value& a, const Value& b) {
    if (std::holds_alternative<std::monostate>(a) && std::holds_alternative<std::monostate>(b)) return true;
    if (std::holds_alternative<std::monostate>(a) || std::holds_alternative<std::monostate>(b)) return false;
    
    if (std::holds_alternative<int>(a) && std::holds_alternative<int>(b)) {
        return std::get<int>(a) == std::get<int>(b);
    }
    if (std::holds_alternative<double>(a) && std::holds_alternative<double>(b)) {
        return std::get<double>(a) == std::get<double>(b);
    }
    if (std::holds_alternative<int>(a) && std::holds_alternative<double>(b)) {
        return std::get<int>(a) == std::get<double>(b);
    }
    if (std::holds_alternative<double>(a) && std::holds_alternative<int>(b)) {
        return std::get<double>(a) == std::get<int>(b);
    }
    if (std::holds_alternative<bool>(a) && std::holds_alternative<bool>(b)) {
        return std::get<bool>(a) == std::get<bool>(b);
    }
    if (std::holds_alternative<std::string>(a) && std::holds_alternative<std::string>(b)) {
        return std::get<std::string>(a) == std::get<std::string>(b);
    }
    
    // 比较数组
    if (std::holds_alternative<std::shared_ptr<std::vector<Value>>>(a) && 
        std::holds_alternative<std::shared_ptr<std::vector<Value>>>(b)) {
        auto aArray = std::get<std::shared_ptr<std::vector<Value>>>(a);
        auto bArray = std::get<std::shared_ptr<std::vector<Value>>>(b);
        if (aArray->size() != bArray->size()) return false;
        for (size_t i = 0; i < aArray->size(); i++) {
            if (!isEqual((*aArray)[i], (*bArray)[i])) return false;
        }
        return true;
    }
    
    // 比较字典
    if (std::holds_alternative<std::shared_ptr<std::unordered_map<std::string, Value>>>(a) && 
        std::holds_alternative<std::shared_ptr<std::unordered_map<std::string, Value>>>(b)) {
        auto aDict = std::get<std::shared_ptr<std::unordered_map<std::string, Value>>>(a);
        auto bDict = std::get<std::shared_ptr<std::unordered_map<std::string, Value>>>(b);
        if (aDict->size() != bDict->size()) return false;
        for (const auto& [key, value] : *aDict) {
            auto it = bDict->find(key);
            if (it == bDict->end() || !isEqual(value, it->second)) return false;
        }
        return true;
    }
    
    // 比较对象
    if (std::holds_alternative<std::shared_ptr<ZeloObject>>(a) && 
        std::holds_alternative<std::shared_ptr<ZeloObject>>(b)) {
        return std::get<std::shared_ptr<ZeloObject>>(a) == std::get<std::shared_ptr<ZeloObject>>(b);
    }
    
    return false;
}

void checkNumberOperand(const Token& op, const Value& operand) {
    if (std::holds_alternative<int>(operand) || std::holds_alternative<double>(operand)) return;
    throw RuntimeError(ErrorCode::TYPE_MISMATCH, "Operand must be a number", op.line);
}

void checkNumberOperands(const Token& op, const Value& left, const Value& right) {
    if ((std::holds_alternative<int>(left) || std::holds_alternative<double>(left)) &&
        (std::holds_alternative<int>(right) || std::holds_alternative<double>(right))) return;
    throw RuntimeError(ErrorCode::TYPE_MISMATCH, "Operands must be numbers", op.line);
}

} // namespace Zelo