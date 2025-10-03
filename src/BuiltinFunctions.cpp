#include "BuiltinFunctions.h"
#include "Interpreter.h"
#include "ErrorCode.h"
#include <iostream>
#include <sstream>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <algorithm>

namespace Zelo {

void BuiltinFunctions::initialize(std::shared_ptr<Environment> globals) {
    // 核心内置函数
    defineFunction(globals, "print", print, -1);
    defineFunction(globals, "type", type, 1);
    defineFunction(globals, "len", len, 1);
    defineFunction(globals, "typeof", typeof, 1);
    defineFunction(globals, "input", input, -1);
    defineFunction(globals, "int", intFunc, 1);
    defineFunction(globals, "float", floatFunc, 1);
    defineFunction(globals, "str", strFunc, 1);
    defineFunction(globals, "bool", boolFunc, 1);
    defineFunction(globals, "exit", exitFunc, -1);
    defineFunction(globals, "assert", assertFunc, -1);
    defineFunction(globals, "panic", panicFunc, 1);
    defineFunction(globals, "range", rangeFunc, -1);
    defineFunction(globals, "chr", chrFunc, 1);
    defineFunction(globals, "ord", ordFunc, 1);
    defineFunction(globals, "repr", reprFunc, 1);
    defineFunction(globals, "isinstance", isinstanceFunc, 2);
    defineFunction(globals, "abs", absFunc, 1);
    defineFunction(globals, "min", minFunc, -1);
    defineFunction(globals, "max", maxFunc, -1);
    defineFunction(globals, "hash", hashFunc, 1);
    
    // 容器操作函数
    defineFunction(globals, "array_push", arrayPush, 2);
    defineFunction(globals, "array_pop", arrayPop, 1);
    defineFunction(globals, "array_slice", arraySlice, -1);
    defineFunction(globals, "dict_keys", dictKeys, 1);
    defineFunction(globals, "dict_values", dictValues, 1);
    defineFunction(globals, "dict_has_key", dictHasKey, 2);
    
    // 对象操作函数
    defineFunction(globals, "object_clone", objectClone, -1);
    defineFunction(globals, "object_fields", objectFields, 1);
    defineFunction(globals, "object_methods", objectMethods, 1);
    
    // 类型检查函数
    defineFunction(globals, "is_int", isInt, 1);
    defineFunction(globals, "is_float", isFloat, 1);
    defineFunction(globals, "is_bool", isBool, 1);
    defineFunction(globals, "is_string", isString, 1);
    defineFunction(globals, "is_array", isArray, 1);
    defineFunction(globals, "is_dict", isDict, 1);
    defineFunction(globals, "is_object", isObject, 1);
    defineFunction(globals, "is_function", isFunction, 1);
    defineFunction(globals, "is_null", isNull, 1);
    
    // 容器克隆函数（内部使用）
    defineFunction(globals, "__array_clone__", __array_clone__, 2);
    defineFunction(globals, "__dict_clone__", __dict_clone__, 2);
}

void BuiltinFunctions::defineFunction(std::shared_ptr<Environment> env, 
                                     const std::string& name,
                                     std::function<Value(const std::vector<Value>&)> func,
                                     int arity) {
    auto zeloFunc = std::make_shared<ZeloFunction>(nullptr, nullptr);
    zeloFunc->call = [func](Interpreter* interpreter, const std::vector<Value>& arguments) -> Value {
        return func(arguments);
    };
    zeloFunc->arity = [arity]() -> int { return arity; };
    env->define(name, zeloFunc);
}

// 核心内置函数实现
Value BuiltinFunctions::print(const std::vector<Value>& args) {
    for (size_t i = 0; i < args.size(); i++) {
        if (i > 0) std::cout << " ";
        std::cout << valueToString(args[i]);
    }
    std::cout << std::endl;
    return Value();
}

Value BuiltinFunctions::type(const std::vector<Value>& args) {
    if (args.size() != 1) {
        throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                   "type() expects exactly 1 argument", 0);
    }
    
    const Value& value = args[0];
    if (std::holds_alternative<std::monostate>(value)) return "null";
    if (std::holds_alternative<int>(value)) return "int";
    if (std::holds_alternative<double>(value)) return "float";
    if (std::holds_alternative<bool>(value)) return "bool";
    if (std::holds_alternative<std::string>(value)) return "string";
    if (std::holds_alternative<std::shared_ptr<std::vector<Value>>>(value)) return "array";
    if (std::holds_alternative<std::shared_ptr<std::unordered_map<std::string, Value>>>(value)) return "dict";
    if (std::holds_alternative<std::shared_ptr<ZeloObject>>(value)) return "object";
    if (std::holds_alternative<std::shared_ptr<ZeloFunction>>(value)) return "function";
    if (std::holds_alternative<std::shared_ptr<ZeloClass>>(value)) return "class";
    
    return "unknown";
}

Value BuiltinFunctions::len(const std::vector<Value>& args) {
    if (args.size() != 1) {
        throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                   "len() expects exactly 1 argument", 0);
    }
    
    const Value& value = args[0];
    if (std::holds_alternative<std::shared_ptr<std::vector<Value>>>(value)) {
        return static_cast<int>(std::get<std::shared_ptr<std::vector<Value>>>(value)->size());
    }
    if (std::holds_alternative<std::shared_ptr<std::unordered_map<std::string, Value>>>(value)) {
        return static_cast<int>(std::get<std::shared_ptr<std::unordered_map<std::string, Value>>>(value)->size());
    }
    if (std::holds_alternative<std::string>(value)) {
        return static_cast<int>(std::get<std::string>(value).size());
    }
    
    throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                               "len() expects array, dictionary or string", 0);
}

Value BuiltinFunctions::typeof(const std::vector<Value>& args) {
    // typeof 返回轻量级类型句柄，这里简单返回类型字符串
    return type(args);
}

Value BuiltinFunctions::input(const std::vector<Value>& args) {
    if (args.size() > 1) {
        throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                   "input() expects 0 or 1 arguments", 0);
    }
    
    if (args.size() == 1) {
        std::cout << valueToString(args[0]);
    }
    
    std::string line;
    std::getline(std::cin, line);
    return line;
}

Value BuiltinFunctions::intFunc(const std::vector<Value>& args) {
    if (args.size() != 1) {
        throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                   "int() expects exactly 1 argument", 0);
    }
    
    const Value& value = args[0];
    if (std::holds_alternative<int>(value)) {
        return value;
    }
    if (std::holds_alternative<double>(value)) {
        return static_cast<int>(std::get<double>(value));
    }
    if (std::holds_alternative<bool>(value)) {
        return static_cast<int>(std::get<bool>(value));
    }
    if (std::holds_alternative<std::string>(value)) {
        try {
            return std::stoi(std::get<std::string>(value));
        } catch (...) {
            throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                       "Cannot convert string to integer", 0);
        }
    }
    
    throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                               "Cannot convert to integer", 0);
}

Value BuiltinFunctions::floatFunc(const std::vector<Value>& args) {
    if (args.size() != 1) {
        throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                   "float() expects exactly 1 argument", 0);
    }
    
    const Value& value = args[0];
    if (std::holds_alternative<double>(value)) {
        return value;
    }
    if (std::holds_alternative<int>(value)) {
        return static_cast<double>(std::get<int>(value));
    }
    if (std::holds_alternative<bool>(value)) {
        return static_cast<double>(std::get<bool>(value));
    }
    if (std::holds_alternative<std::string>(value)) {
        try {
            return std::stod(std::get<std::string>(value));
        } catch (...) {
            throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                       "Cannot convert string to float", 0);
        }
    }
    
    throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                               "Cannot convert to float", 0);
}

Value BuiltinFunctions::strFunc(const std::vector<Value>& args) {
    if (args.size() != 1) {
        throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                   "str() expects exactly 1 argument", 0);
    }
    
    return valueToString(args[0]);
}

Value BuiltinFunctions::boolFunc(const std::vector<Value>& args) {
    if (args.size() != 1) {
        throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                   "bool() expects exactly 1 argument", 0);
    }
    
    return isTruthy(args[0]);
}

Value BuiltinFunctions::exitFunc(const std::vector<Value>& args) {
    int code = 0;
    if (args.size() > 0) {
        if (std::holds_alternative<int>(args[0])) {
            code = std::get<int>(args[0]);
        } else {
            throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                       "exit() expects integer argument", 0);
        }
    }
    
    std::exit(code);
    return Value();
}

Value BuiltinFunctions::assertFunc(const std::vector<Value>& args) {
    if (args.size() < 1 || args.size() > 2) {
        throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                   "assert() expects 1 or 2 arguments", 0);
    }
    
    if (!isTruthy(args[0])) {
        std::string message = "Assertion failed";
        if (args.size() == 2) {
            message = valueToString(args[1]);
        }
        throw RuntimeError::fromCode(ErrorCode::RUNTIME_ERROR, message, 0);
    }
    
    return Value();
}

Value BuiltinFunctions::panicFunc(const std::vector<Value>& args) {
    if (args.size() != 1) {
        throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                   "panic() expects exactly 1 argument", 0);
    }
    
    throw RuntimeError::fromCode(ErrorCode::RUNTIME_ERROR, valueToString(args[0]), 0);
}

Value BuiltinFunctions::rangeFunc(const std::vector<Value>& args) {
    if (args.size() < 1 || args.size() > 3) {
        throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                   "range() expects 1 to 3 arguments", 0);
    }
    
    int start = 0;
    int stop = 0;
    int step = 1;
    
    if (args.size() == 1) {
        if (!std::holds_alternative<int>(args[0])) {
            throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                       "range() arguments must be integers", 0);
        }
        stop = std::get<int>(args[0]);
    } else {
        if (!std::holds_alternative<int>(args[0]) || !std::holds_alternative<int>(args[1])) {
            throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                       "range() arguments must be integers", 0);
        }
        start = std::get<int>(args[0]);
        stop = std::get<int>(args[1]);
        
        if (args.size() == 3) {
            if (!std::holds_alternative<int>(args[2])) {
                throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                           "range() arguments must be integers", 0);
            }
            step = std::get<int>(args[2]);
            if (step == 0) {
                throw RuntimeError::fromCode(ErrorCode::RUNTIME_ERROR, 
                                           "range() step cannot be zero", 0);
            }
        }
    }
    
    auto array = std::make_shared<std::vector<Value>>();
    if (step > 0) {
        for (int i = start; i < stop; i += step) {
            array->push_back(i);
        }
    } else {
        for (int i = start; i > stop; i += step) {
            array->push_back(i);
        }
    }
    
    return array;
}

Value BuiltinFunctions::chrFunc(const std::vector<Value>& args) {
    if (args.size() != 1) {
        throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                   "chr() expects exactly 1 argument", 0);
    }
    
    if (!std::holds_alternative<int>(args[0])) {
        throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                   "chr() argument must be integer", 0);
    }
    
    int code = std::get<int>(args[0]);
    if (code < 0 || code > 0x10FFFF) {
        throw RuntimeError::fromCode(ErrorCode::RUNTIME_ERROR, 
                                   "chr() code point out of range", 0);
    }
    
    return std::string(1, static_cast<char>(code));
}

Value BuiltinFunctions::ordFunc(const std::vector<Value>& args) {
    if (args.size() != 1) {
        throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                   "ord() expects exactly 1 argument", 0);
    }
    
    if (!std::holds_alternative<std::string>(args[0])) {
        throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                   "ord() argument must be string", 0);
    }
    
    const std::string& str = std::get<std::string>(args[0]);
    if (str.empty()) {
        throw RuntimeError::fromCode(ErrorCode::RUNTIME_ERROR, 
                                   "ord() argument must not be empty", 0);
    }
    if (str.size() > 1) {
        throw RuntimeError::fromCode(ErrorCode::RUNTIME_ERROR, 
                                   "ord() argument must be a single character", 0);
    }
    
    return static_cast<int>(str[0]);
}

Value BuiltinFunctions::reprFunc(const std::vector<Value>& args) {
    if (args.size() != 1) {
        throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                   "repr() expects exactly 1 argument", 0);
    }
    
    const Value& value = args[0];
    if (std::holds_alternative<std::string>(value)) {
        return "\"" + std::get<std::string>(value) + "\"";
    }
    if (std::holds_alternative<std::monostate>(value)) {
        return "null";
    }
    if (std::holds_alternative<bool>(value)) {
        return std::get<bool>(value) ? "true" : "false";
    }
    
    return valueToString(value);
}

Value BuiltinFunctions::isinstanceFunc(const std::vector<Value>& args) {
    if (args.size() != 2) {
        throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                   "isinstance() expects exactly 2 arguments", 0);
    }
    
    const Value& value = args[0];
    const Value& typeValue = args[1];
    
    if (!std::holds_alternative<std::string>(typeValue)) {
        throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                   "isinstance() second argument must be string", 0);
    }
    
    std::string typeStr = std::get<std::string>(typeValue);
    
    // 检查联合类型（如 "int|float"）
    size_t pos = typeStr.find('|');
    if (pos != std::string::npos) {
        std::string type1 = typeStr.substr(0, pos);
        std::string type2 = typeStr.substr(pos + 1);
        return isinstanceFunc({value, type1}) || isinstanceFunc({value, type2});
    }
    
    // 检查具体类型
    if (typeStr == "null") {
        return std::holds_alternative<std::monostate>(value);
    }
    if (typeStr == "int") {
        return std::holds_alternative<int>(value);
    }
    if (typeStr == "float") {
        return std::holds_alternative<double>(value);
    }
    if (typeStr == "bool") {
        return std::holds_alternative<bool>(value);
    }
    if (typeStr == "string") {
        return std::holds_alternative<std::string>(value);
    }
    if (typeStr == "array") {
        return std::holds_alternative<std::shared_ptr<std::vector<Value>>>(value);
    }
    if (typeStr == "dict") {
        return std::holds_alternative<std::shared_ptr<std::unordered_map<std::string, Value>>>(value);
    }
    if (typeStr == "object") {
        return std::holds_alternative<std::shared_ptr<ZeloObject>>(value);
    }
    if (typeStr == "function") {
        return std::holds_alternative<std::shared_ptr<ZeloFunction>>(value);
    }
    if (typeStr == "class") {
        return std::holds_alternative<std::shared_ptr<ZeloClass>>(value);
    }
    
    throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                               "Unknown type: " + typeStr, 0);
}

Value BuiltinFunctions::absFunc(const std::vector<Value>& args) {
    if (args.size() != 1) {
        throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                   "abs() expects exactly 1 argument", 0);
    }
    
    const Value& value = args[0];
    if (std::holds_alternative<int>(value)) {
        return std::abs(std::get<int>(value));
    }
    if (std::holds_alternative<double>(value)) {
        return std::abs(std::get<double>(value));
    }
    
    throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                               "abs() argument must be number", 0);
}

Value BuiltinFunctions::minFunc(const std::vector<Value>& args) {
    if (args.empty()) {
        throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                   "min() expects at least 1 argument", 0);
    }
    
    Value minValue = args[0];
    for (size_t i = 1; i < args.size(); i++) {
        const Value& current = args[i];
        
        if (std::holds_alternative<int>(minValue) && std::holds_alternative<int>(current)) {
            if (std::get<int>(current) < std::get<int>(minValue)) {
                minValue = current;
            }
        } else if (std::holds_alternative<double>(minValue) && std::holds_alternative<double>(current)) {
            if (std::get<double>(current) < std::get<double>(minValue)) {
                minValue = current;
            }
        } else if (std::holds_alternative<int>(minValue) && std::holds_alternative<double>(current)) {
            if (std::get<double>(current) < std::get<int>(minValue)) {
                minValue = current;
            }
        } else if (std::holds_alternative<double>(minValue) && std::holds_alternative<int>(current)) {
            if (std::get<int>(current) < std::get<double>(minValue)) {
                minValue = current;
            }
        } else {
            throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                       "min() arguments must be numbers", 0);
        }
    }
    
    return minValue;
}

Value BuiltinFunctions::maxFunc(const std::vector<Value>& args) {
    if (args.empty()) {
        throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                   "max() expects at least 1 argument", 0);
    }
    
    Value maxValue = args[0];
    for (size_t i = 1; i < args.size(); i++) {
        const Value& current = args[i];
        
        if (std::holds_alternative<int>(maxValue) && std::holds_alternative<int>(current)) {
            if (std::get<int>(current) > std::get<int>(maxValue)) {
                maxValue = current;
            }
        } else if (std::holds_alternative<double>(maxValue) && std::holds_alternative<double>(current)) {
            if (std::get<double>(current) > std::get<double>(maxValue)) {
                maxValue = current;
            }
        } else if (std::holds_alternative<int>(maxValue) && std::holds_alternative<double>(current)) {
            if (std::get<double>(current) > std::get<int>(maxValue)) {
                maxValue = current;
            }
        } else if (std::holds_alternative<double>(maxValue) && std::holds_alternative<int>(current)) {
            if (std::get<int>(current) > std::get<double>(maxValue)) {
                maxValue = current;
            }
        } else {
            throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                       "max() arguments must be numbers", 0);
        }
    }
    
    return maxValue;
}

Value BuiltinFunctions::hashFunc(const std::vector<Value>& args) {
    if (args.size() != 1) {
        throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                   "hash() expects exactly 1 argument", 0);
    }
    
    // 简单的哈希实现
    const Value& value = args[0];
    std::hash<std::string> hasher;
    std::string str = valueToString(value);
    return static_cast<int>(hasher(str));
}

// 容器操作函数实现
Value BuiltinFunctions::arrayPush(const std::vector<Value>& args) {
    if (args.size() != 2) {
        throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                   "array_push() expects exactly 2 arguments", 0);
    }
    
    if (!std::holds_alternative<std::shared_ptr<std::vector<Value>>>(args[0])) {
        throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                   "array_push() first argument must be array", 0);
    }
    
    auto array = std::get<std::shared_ptr<std::vector<Value>>>(args[0]);
    array->push_back(args[1]);
    return static_cast<int>(array->size());
}

Value BuiltinFunctions::arrayPop(const std::vector<Value>& args) {
    if (args.size() != 1) {
        throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                   "array_pop() expects exactly 1 argument", 0);
    }
    
    if (!std::holds_alternative<std::shared_ptr<std::vector<Value>>>(args[0])) {
        throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                   "array_pop() argument must be array", 0);
    }
    
    auto array = std::get<std::shared_ptr<std::vector<Value>>>(args[0]);
    if (array->empty()) {
        throw RuntimeError::fromCode(ErrorCode::RUNTIME_ERROR, 
                                   "Cannot pop from empty array", 0);
    }
    
    Value result = array->back();
    array->pop_back();
    return result;
}

Value BuiltinFunctions::arraySlice(const std::vector<Value>& args) {
    if (args.size() < 1 || args.size() > 4) {
        throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                   "array_slice() expects 1 to 4 arguments", 0);
    }
    
    if (!std::holds_alternative<std::shared_ptr<std::vector<Value>>>(args[0])) {
        throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                   "array_slice() first argument must be array", 0);
    }
    
    auto array = std::get<std::shared_ptr<std::vector<Value>>>(args[0]);
    int size = static_cast<int>(array->size());
    
    int start = 0;
    int stop = size;
    int step = 1;
    
    if (args.size() >= 2) {
        if (!std::holds_alternative<int>(args[1])) {
            throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                       "array_slice() start must be integer", 0);
        }
        start = std::get<int>(args[1]);
        if (start < 0) start = size + start;
    }
    
    if (args.size() >= 3) {
        if (!std::holds_alternative<int>(args[2])) {
            throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                       "array_slice() stop must be integer", 0);
        }
        stop = std::get<int>(args[2]);
        if (stop < 0) stop = size + stop;
    }
    
    if (args.size() >= 4) {
        if (!std::holds_alternative<int>(args[3])) {
            throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                       "array_slice() step must be integer", 0);
        }
        step = std::get<int>(args[3]);
        if (step == 0) {
            throw RuntimeError::fromCode(ErrorCode::RUNTIME_ERROR, 
                                       "array_slice() step cannot be zero", 0);
        }
    }
    
    start = std::max(0, std::min(start, size));
    stop = std::max(0, std::min(stop, size));
    
    auto result = std::make_shared<std::vector<Value>>();
    if (step > 0) {
        for (int i = start; i < stop; i += step) {
            result->push_back((*array)[i]);
        }
    } else {
        for (int i = start; i > stop; i += step) {
            result->push_back((*array)[i]);
        }
    }
    
    return result;
}

Value BuiltinFunctions::dictKeys(const std::vector<Value>& args) {
    if (args.size() != 1) {
        throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                   "dict_keys() expects exactly 1 argument", 0);
    }
    
    if (!std::holds_alternative<std::shared_ptr<std::unordered_map<std::string, Value>>>(args[0])) {
        throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                   "dict_keys() argument must be dictionary", 0);
    }
    
    auto dict = std::get<std::shared_ptr<std::unordered_map<std::string, Value>>>(args[0]);
    auto keys = std::make_shared<std::vector<Value>>();
    
    for (const auto& [key, value] : *dict) {
        keys->push_back(key);
    }
    
    return keys;
}

Value BuiltinFunctions::dictValues(const std::vector<Value>& args) {
    if (args.size() != 1) {
        throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                   "dict_values() expects exactly 1 argument", 0);
    }
    
    if (!std::holds_alternative<std::shared_ptr<std::unordered_map<std::string, Value>>>(args[0])) {
        throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                   "dict_values() argument must be dictionary", 0);
    }
    
    auto dict = std::get<std::shared_ptr<std::unordered_map<std::string, Value>>>(args[0]);
    auto values = std::make_shared<std::vector<Value>>();
    
    for (const auto& [key, value] : *dict) {
        values->push_back(value);
    }
    
    return values;
}

Value BuiltinFunctions::dictHasKey(const std::vector<Value>& args) {
    if (args.size() != 2) {
        throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                   "dict_has_key() expects exactly 2 arguments", 0);
    }
    
    if (!std::holds_alternative<std::shared_ptr<std::unordered_map<std::string, Value>>>(args[0])) {
        throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                   "dict_has_key() first argument must be dictionary", 0);
    }
    
    if (!std::holds_alternative<std::string>(args[1])) {
        throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                   "dict_has_key() second argument must be string", 0);
    }
    
    auto dict = std::get<std::shared_ptr<std::unordered_map<std::string, Value>>>(args[0]);
    std::string key = std::get<std::string>(args[1]);
    
    return dict->find(key) != dict->end();
}

// 对象操作函数实现
Value BuiltinFunctions::objectClone(const std::vector<Value>& args) {
    if (args.size() < 1 || args.size() > 2) {
        throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                   "object_clone() expects 1 or 2 arguments", 0);
    }
    
    if (!std::holds_alternative<std::shared_ptr<ZeloObject>>(args[0])) {
        throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                   "object_clone() first argument must be object", 0);
    }
    
    auto object = std::get<std::shared_ptr<ZeloObject>>(args[0]);
    std::string mode = "shallow";
    
    if (args.size() == 2) {
        if (!std::holds_alternative<std::string>(args[1])) {
            throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                       "object_clone() second argument must be string", 0);
        }
        mode = std::get<std::string>(args[1]);
    }
    
    // 查找对象的 __clone__ 方法
    auto cloneMethod = object->klass->findMethod("__clone__");
    if (cloneMethod) {
        // 绑定方法到对象
        auto boundClone = std::make_shared<ZeloFunction>(
            cloneMethod->declaration,
            std::make_shared<Environment>(cloneMethod->closure)
        );
        boundClone->closure->define("this", object);
        return boundClone->call(nullptr, {mode});
    }
    
    // 如果没有 __clone__ 方法，创建浅拷贝
    auto newObject = std::make_shared<ZeloObject>(object->klass);
    newObject->fields = object->fields;
    return newObject;
}

Value BuiltinFunctions::objectFields(const std::vector<Value>& args) {
    if (args.size() != 1) {
        throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                   "object_fields() expects exactly 1 argument", 0);
    }
    
    if (!std::holds_alternative<std::shared_ptr<ZeloObject>>(args[0])) {
        throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                   "object_fields() argument must be object", 0);
    }
    
    auto object = std::get<std::shared_ptr<ZeloObject>>(args[0]);
    auto fields = std::make_shared<std::vector<Value>>();
    
    for (const auto& [name, value] : object->fields) {
        fields->push_back(name);
    }
    
    return fields;
}

Value BuiltinFunctions::objectMethods(const std::vector<Value>& args) {
    if (args.size() != 1) {
        throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                   "object_methods() expects exactly 1 argument", 0);
    }
    
    if (!std::holds_alternative<std::shared_ptr<ZeloObject>>(args[0])) {
        throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                   "object_methods() argument must be object", 0);
    }
    
    auto object = std::get<std::shared_ptr<ZeloObject>>(args[0]);
    auto methods = std::make_shared<std::vector<Value>>();
    
    // 获取对象的所有方法
    for (const auto& [name, method] : object->klass->methods) {
        methods->push_back(name);
    }
    
    return methods;
}

// 类型检查函数实现
Value BuiltinFunctions::isInt(const std::vector<Value>& args) {
    if (args.size() != 1) {
        throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                   "is_int() expects exactly 1 argument", 0);
    }
    return std::holds_alternative<int>(args[0]);
}

Value BuiltinFunctions::isFloat(const std::vector<Value>& args) {
    if (args.size() != 1) {
        throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                   "is_float() expects exactly 1 argument", 0);
    }
    return std::holds_alternative<double>(args[0]);
}

Value BuiltinFunctions::isBool(const std::vector<Value>& args) {
    if (args.size() != 1) {
        throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                   "is_bool() expects exactly 1 argument", 0);
    }
    return std::holds_alternative<bool>(args[0]);
}

Value BuiltinFunctions::isString(const std::vector<Value>& args) {
    if (args.size() != 1) {
        throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                   "is_string() expects exactly 1 argument", 0);
    }
    return std::holds_alternative<std::string>(args[0]);
}

Value BuiltinFunctions::isArray(const std::vector<Value>& args) {
    if (args.size() != 1) {
        throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                   "is_array() expects exactly 1 argument", 0);
    }
    return std::holds_alternative<std::shared_ptr<std::vector<Value>>>(args[0]);
}

Value BuiltinFunctions::isDict(const std::vector<Value>& args) {
    if (args.size() != 1) {
        throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                   "is_dict() expects exactly 1 argument", 0);
    }
    return std::holds_alternative<std::shared_ptr<std::unordered_map<std::string, Value>>>(args[0]);
}

Value BuiltinFunctions::isObject(const std::vector<Value>& args) {
    if (args.size() != 1) {
        throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                   "is_object() expects exactly 1 argument", 0);
    }
    return std::holds_alternative<std::shared_ptr<ZeloObject>>(args[0]);
}

Value BuiltinFunctions::isFunction(const std::vector<Value>& args) {
    if (args.size() != 1) {
        throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                   "is_function() expects exactly 1 argument", 0);
    }
    return std::holds_alternative<std::shared_ptr<ZeloFunction>>(args[0]);
}

Value BuiltinFunctions::isNull(const std::vector<Value>& args) {
    if (args.size() != 1) {
        throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                   "is_null() expects exactly 1 argument", 0);
    }
    return std::holds_alternative<std::monostate>(args[0]);
}

// 容器克隆函数实现（内部使用）
Value BuiltinFunctions::__array_clone__(const std::vector<Value>& args) {
    if (args.size() != 2) {
        throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                   "__array_clone__ expects 2 arguments", 0);
    }
    
    if (!std::holds_alternative<std::shared_ptr<std::vector<Value>>>(args[0])) {
        throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                   "__array_clone__ first argument must be array", 0);
    }
    
    if (!std::holds_alternative<std::string>(args[1])) {
        throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                   "__array_clone__ second argument must be string", 0);
    }
    
    auto array = std::get<std::shared_ptr<std::vector<Value>>>(args[0]);
    std::string mode = std::get<std::string>(args[1]);
    
    auto result = std::make_shared<std::vector<Value>>();
    result->reserve(array->size());
    
    if (mode == "shallow") {
        // 浅拷贝：直接复制元素
        for (const auto& element : *array) {
            result->push_back(element);
        }
    } else if (mode == "deep") {
        // 深拷贝：递归克隆元素
        for (const auto& element : *array) {
            if (std::holds_alternative<std::shared_ptr<ZeloObject>>(element)) {
                auto obj = std::get<std::shared_ptr<ZeloObject>>(element);
                auto cloneMethod = obj->klass->findMethod("__clone__");
                if (cloneMethod) {
                    // 绑定方法到对象
                    auto boundClone = std::make_shared<ZeloFunction>(
                        cloneMethod->declaration,
                        std::make_shared<Environment>(cloneMethod->closure)
                    );
                    boundClone->closure->define("this", obj);
                    result->push_back(boundClone->call(nullptr, {"deep"}));
                } else {
                    result->push_back(element); // 没有克隆方法，使用引用
                }
            } else if (std::holds_alternative<std::shared_ptr<std::vector<Value>>>(element)) {
                // 递归克隆数组
                result->push_back(__array_clone__({element, "deep"}));
            } else if (std::holds_alternative<std::shared_ptr<std::unordered_map<std::string, Value>>>(element)) {
                // 递归克隆字典
                result->push_back(__dict_clone__({element, "deep"}));
            } else {
                result->push_back(element); // 基本类型，直接复制
            }
        }
    } else {
        throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                   "Invalid clone mode: " + mode, 0);
    }
    
    return result;
}

Value BuiltinFunctions::__dict_clone__(const std::vector<Value>& args) {
    if (args.size() != 2) {
        throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                   "__dict_clone__ expects 2 arguments", 0);
    }
    
    if (!std::holds_alternative<std::shared_ptr<std::unordered_map<std::string, Value>>>(args[0])) {
        throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                   "__dict_clone__ first argument must be dictionary", 0);
    }
    
    if (!std::holds_alternative<std::string>(args[1])) {
        throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                   "__dict_clone__ second argument must be string", 0);
    }
    
    auto dict = std::get<std::shared_ptr<std::unordered_map<std::string, Value>>>(args[0]);
    std::string mode = std::get<std::string>(args[1]);
    
    auto result = std::make_shared<std::unordered_map<std::string, Value>>();
    
    if (mode == "shallow") {
        // 浅拷贝：直接复制值
        for (const auto& [key, value] : *dict) {
            (*result)[key] = value;
        }
    } else if (mode == "deep") {
        // 深拷贝：递归克隆值
        for (const auto& [key, value] : *dict) {
            if (std::holds_alternative<std::shared_ptr<ZeloObject>>(value)) {
                auto obj = std::get<std::shared_ptr<ZeloObject>>(value);
                auto cloneMethod = obj->klass->findMethod("__clone__");
                if (cloneMethod) {
                    // 绑定方法到对象
                    auto boundClone = std::make_shared<ZeloFunction>(
                        cloneMethod->declaration,
                        std::make_shared<Environment>(cloneMethod->closure)
                    );
                    boundClone->closure->define("this", obj);
                    (*result)[key] = boundClone->call(nullptr, {"deep"});
                } else {
                    (*result)[key] = value; // 没有克隆方法，使用引用
                }
            } else if (std::holds_alternative<std::shared_ptr<std::vector<Value>>>(value)) {
                // 递归克隆数组
                (*result)[key] = __array_clone__({value, "deep"});
            } else if (std::holds_alternative<std::shared_ptr<std::unordered_map<std::string, Value>>>(value)) {
                // 递归克隆字典
                (*result)[key] = __dict_clone__({value, "deep"});
            } else {
                (*result)[key] = value; // 基本类型，直接复制
            }
        }
    } else {
        throw RuntimeError::fromCode(ErrorCode::TYPE_ERROR, 
                                   "Invalid clone mode: " + mode, 0);
    }
    
    return result;
}

} // namespace Zelo