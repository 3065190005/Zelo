#pragma once

#include "Value.h"
#include <functional>
#include <memory>

namespace Zelo {

class BuiltinFunctions {
public:
    static void initialize(std::shared_ptr<Environment> globals);
    
private:
    // 核心内置函数
    static Value print(const std::vector<Value>& args);
    static Value type(const std::vector<Value>& args);
    static Value len(const std::vector<Value>& args);
    static Value typeof(const std::vector<Value>& args);
    static Value input(const std::vector<Value>& args);
    static Value intFunc(const std::vector<Value>& args);
    static Value floatFunc(const std::vector<Value>& args);
    static Value strFunc(const std::vector<Value>& args);
    static Value boolFunc(const std::vector<Value>& args);
    static Value exitFunc(const std::vector<Value>& args);
    static Value assertFunc(const std::vector<Value>& args);
    static Value panicFunc(const std::vector<Value>& args);
    static Value rangeFunc(const std::vector<Value>& args);
    static Value chrFunc(const std::vector<Value>& args);
    static Value ordFunc(const std::vector<Value>& args);
    static Value reprFunc(const std::vector<Value>& args);
    static Value isinstanceFunc(const std::vector<Value>& args);
    static Value absFunc(const std::vector<Value>& args);
    static Value minFunc(const std::vector<Value>& args);
    static Value maxFunc(const std::vector<Value>& args);
    static Value hashFunc(const std::vector<Value>& args);
    
    // 容器操作函数
    static Value arrayPush(const std::vector<Value>& args);
    static Value arrayPop(const std::vector<Value>& args);
    static Value arraySlice(const std::vector<Value>& args);
    static Value dictKeys(const std::vector<Value>& args);
    static Value dictValues(const std::vector<Value>& args);
    static Value dictHasKey(const std::vector<Value>& args);
    
    // 对象操作函数
    static Value objectClone(const std::vector<Value>& args);
    static Value objectFields(const std::vector<Value>& args);
    static Value objectMethods(const std::vector<Value>& args);
    
    // 类型检查函数
    static Value isInt(const std::vector<Value>& args);
    static Value isFloat(const std::vector<Value>& args);
    static Value isBool(const std::vector<Value>& args);
    static Value isString(const std::vector<Value>& args);
    static Value isArray(const std::vector<Value>& args);
    static Value isDict(const std::vector<Value>& args);
    static Value isObject(const std::vector<Value>& args);
    static Value isFunction(const std::vector<Value>& args);
    static Value isNull(const std::vector<Value>& args);
    
    // 容器克隆函数（内部使用）
    static Value __array_clone__(const std::vector<Value>& args);
    static Value __dict_clone__(const std::vector<Value>& args);
    
    // 工具函数
    static void defineFunction(std::shared_ptr<Environment> env, 
                              const std::string& name,
                              std::function<Value(const std::vector<Value>&)> func,
                              int arity = -1);
};

} // namespace Zelo