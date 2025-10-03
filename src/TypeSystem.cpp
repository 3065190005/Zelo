#include "TypeSystem.h"
#include "Interpreter.h"
#include <algorithm>
#include <sstream>

namespace Zelo {

bool TypeSystem::checkType(const Value& value, const TypeAnnotation& type) {
    // 处理任意类型
    if (type.types.size() == 1 && type.types[0] == TokenType::ELLIPSIS) {
        return true;
    }
    
    // 处理联合类型
    if (!type.types.empty()) {
        return checkUnionType(value, type.types);
    }
    
    // 处理数组类型
    if (type.isArray && type.valueType) {
        return checkArrayType(value, *type.valueType);
    }
    
    // 处理字典类型
    if (type.isDict && type.keyType && type.valueType) {
        return checkDictType(value, *type.keyType, *type.valueType);
    }
    
    return false;
}

Value TypeSystem::castValue(const Value& value, const TypeAnnotation& type) {
    // 处理任意类型
    if (type.types.size() == 1 && type.types[0] == TokenType::ELLIPSIS) {
        return value;
    }
    
    // 处理联合类型 - 尝试转换为第一个兼容的类型
    if (!type.types.empty()) {
        for (TokenType targetType : type.types) {
            try {
                return castToBasicType(value, targetType);
            } catch (const RuntimeError&) {
                // 尝试下一个类型
                continue;
            }
        }
        throw RuntimeError("Cannot cast value to any of the union types", 0);
    }
    
    // 处理数组类型
    if (type.isArray && type.valueType) {
        return castToArrayType(value, *type.valueType);
    }
    
    // 处理字典类型
    if (type.isDict && type.keyType && type.valueType) {
        return castToDictType(value, *type.keyType, *type.valueType);
    }
    
    throw RuntimeError("Cannot cast value to target type", 0);
}

bool TypeSystem::isTypeCompatible(const TypeAnnotation& source, const TypeAnnotation& target) {
    // 任意类型与任何类型都兼容
    if (target.types.size() == 1 && target.types[0] == TokenType::ELLIPSIS) {
        return true;
    }
    
    // 处理联合类型兼容性
    if (!source.types.empty() && !target.types.empty()) {
        for (TokenType sourceType : source.types) {
            for (TokenType targetType : target.types) {
                if (isBasicTypeCompatible(sourceType, targetType)) {
                    return true;
                }
            }
        }
        return false;
    }
    
    // 处理数组类型兼容性
    if (source.isArray && target.isArray) {
        return isArrayTypeCompatible(source, target);
    }
    
    // 处理字典类型兼容性
    if (source.isDict && target.isDict) {
        return isDictTypeCompatible(source, target);
    }
    
    // 其他情况不兼容
    return false;
}

std::string TypeSystem::typeAnnotationToString(const TypeAnnotation& type) {
    std::stringstream ss;
    
    if (type.isArray) {
        ss << "[";
        if (type.valueType) {
            ss << typeAnnotationToString(*type.valueType);
        } else {
            ss << "...";
        }
        ss << "]";
    } else if (type.isDict) {
        ss << "{";
        if (type.keyType) {
            ss << typeAnnotationToString(*type.keyType);
        } else {
            ss << "...";
        }
        ss << ":";
        if (type.valueType) {
            ss << typeAnnotationToString(*type.valueType);
        } else {
            ss << "...";
        }
        ss << "}";
    } else if (!type.types.empty()) {
        for (size_t i = 0; i < type.types.size(); i++) {
            if (i > 0) ss << "|";
            
            switch (type.types[i]) {
                case TokenType::TYPE_INT: ss << "int"; break;
                case TokenType::TYPE_FLOAT: ss << "float"; break;
                case TokenType::TYPE_BOOL: ss << "bool"; break;
                case TokenType::TYPE_STRING: ss << "string"; break;
                case TokenType::ELLIPSIS: ss << "..."; break;
                default: ss << "unknown"; break;
            }
        }
    } else {
        ss << "any";
    }
    
    return ss.str();
}

bool TypeSystem::checkBasicType(const Value& value, TokenType type) {
    switch (type) {
        case TokenType::TYPE_INT:
            return std::holds_alternative<int>(value);
        case TokenType::TYPE_FLOAT:
            return std::holds_alternative<double>(value);
        case TokenType::TYPE_BOOL:
            return std::holds_alternative<bool>(value);
        case TokenType::TYPE_STRING:
            return std::holds_alternative<std::string>(value);
        case TokenType::ELLIPSIS:
            return true; // 任意类型
        default:
            return false;
    }
}

bool TypeSystem::checkUnionType(const Value& value, const std::vector<TokenType>& types) {
    for (TokenType type : types) {
        if (checkBasicType(value, type)) {
            return true;
        }
    }
    return false;
}

bool TypeSystem::checkArrayType(const Value& value, const TypeAnnotation& elementType) {
    if (!std::holds_alternative<std::shared_ptr<std::vector<Value>>>(value)) {
        return false;
    }
    
    auto array = std::get<std::shared_ptr<std::vector<Value>>>(value);
    for (const auto& element : *array) {
        if (!checkType(element, elementType)) {
            return false;
        }
    }
    
    return true;
}

bool TypeSystem::checkDictType(const Value& value, const TypeAnnotation& keyType, const TypeAnnotation& valueType) {
    if (!std::holds_alternative<std::shared_ptr<std::unordered_map<std::string, Value>>>(value)) {
        return false;
    }
    
    auto dict = std::get<std::shared_ptr<std::unordered_map<std::string, Value>>>(value);
    
    // 检查空字典
    if (dict->empty()) {
        return true;
    }
    
    // 检查键类型（字典键总是字符串）
    if (!keyType.types.empty() && 
        std::find(keyType.types.begin(), keyType.types.end(), TokenType::TYPE_STRING) == keyType.types.end()) {
        return false;
    }
    
    // 检查值类型
    for (const auto& [key, val] : *dict) {
        if (!checkType(val, valueType)) {
            return false;
        }
    }
    
    return true;
}

Value TypeSystem::castToBasicType(const Value& value, TokenType targetType) {
    switch (targetType) {
        case TokenType::TYPE_INT:
            if (std::holds_alternative<int>(value)) {
                return value;
            } else if (std::holds_alternative<double>(value)) {
                return static_cast<int>(std::get<double>(value));
            } else if (std::holds_alternative<bool>(value)) {
                return static_cast<int>(std::get<bool>(value));
            } else if (std::holds_alternative<std::string>(value)) {
                try {
                    return std::stoi(std::get<std::string>(value));
                } catch (...) {
                    throw RuntimeError("Cannot convert string to integer", 0);
                }
            }
            break;
            
        case TokenType::TYPE_FLOAT:
            if (std::holds_alternative<double>(value)) {
                return value;
            } else if (std::holds_alternative<int>(value)) {
                return static_cast<double>(std::get<int>(value));
            } else if (std::holds_alternative<bool>(value)) {
                return static_cast<double>(std::get<bool>(value));
            } else if (std::holds_alternative<std::string>(value)) {
                try {
                    return std::stod(std::get<std::string>(value));
                } catch (...) {
                    throw RuntimeError("Cannot convert string to float", 0);
                }
            }
            break;
            
        case TokenType::TYPE_BOOL:
            if (std::holds_alternative<bool>(value)) {
                return value;
            } else if (std::holds_alternative<int>(value)) {
                return static_cast<bool>(std::get<int>(value));
            } else if (std::holds_alternative<double>(value)) {
                return static_cast<bool>(std::get<double>(value));
            } else if (std::holds_alternative<std::string>(value)) {
                std::string str = std::get<std::string>(value);
                return !str.empty() && str != "false" && str != "0";
            }
            break;
            
        case TokenType::TYPE_STRING:
            if (std::holds_alternative<std::string>(value)) {
                return value;
            } else {
                // 使用内置的 valueToString 函数
                return valueToString(value);
            }
            break;
            
        case TokenType::ELLIPSIS:
            return value; // 任意类型，直接返回
            
        default:
            break;
    }
    
    throw RuntimeError("Cannot convert value to target type", 0);
}

Value TypeSystem::castToArrayType(const Value& value, const TypeAnnotation& elementType) {
    if (std::holds_alternative<std::shared_ptr<std::vector<Value>>>(value)) {
        auto array = std::get<std::shared_ptr<std::vector<Value>>>(value);
        auto result = std::make_shared<std::vector<Value>>();
        
        for (const auto& element : *array) {
            result->push_back(castValue(element, elementType));
        }
        
        return result;
    }
    
    // 如果不是数组，尝试创建单元素数组
    auto result = std::make_shared<std::vector<Value>>();
    result->push_back(castValue(value, elementType));
    return result;
}

Value TypeSystem::castToDictType(const Value& value, const TypeAnnotation& keyType, const TypeAnnotation& valueType) {
    if (std::holds_alternative<std::shared_ptr<std::unordered_map<std::string, Value>>>(value)) {
        auto dict = std::get<std::shared_ptr<std::unordered_map<std::string, Value>>>(value);
        auto result = std::make_shared<std::unordered_map<std::string, Value>>();
        
        for (const auto& [key, val] : *dict) {
            // 转换键（字典键总是字符串）
            Value keyValue = key;
            Value convertedKey = castValue(keyValue, keyType);
            
            if (!std::holds_alternative<std::string>(convertedKey)) {
                throw RuntimeError("Dictionary key must be a string after conversion", 0);
            }
            
            std::string newKey = std::get<std::string>(convertedKey);
            result->emplace(newKey, castValue(val, valueType));
        }
        
        return result;
    }
    
    throw RuntimeError("Cannot convert value to dictionary type", 0);
}

bool TypeSystem::isBasicTypeCompatible(TokenType source, TokenType target) {
    // 任意类型与任何类型都兼容
    if (target == TokenType::ELLIPSIS) {
        return true;
    }
    
    // 相同类型兼容
    if (source == target) {
        return true;
    }
    
    // 数值类型之间的兼容性
    if ((source == TokenType::TYPE_INT && target == TokenType::TYPE_FLOAT) ||
        (source == TokenType::TYPE_FLOAT && target == TokenType::TYPE_INT)) {
        return true;
    }
    
    // 布尔类型可以转换为数值类型
    if (source == TokenType::TYPE_BOOL && 
        (target == TokenType::TYPE_INT || target == TokenType::TYPE_FLOAT)) {
        return true;
    }
    
    // 任何类型都可以转换为字符串
    if (target == TokenType::TYPE_STRING) {
        return true;
    }
    
    return false;
}

bool TypeSystem::isArrayTypeCompatible(const TypeAnnotation& source, const TypeAnnotation& target) {
    // 检查两个数组的元素类型是否兼容
    if (source.valueType && target.valueType) {
        return isTypeCompatible(*source.valueType, *target.valueType);
    }
    
    // 如果目标数组元素类型是任意类型，则兼容
    if (target.valueType && target.valueType->types.size() == 1 && 
        target.valueType->types[0] == TokenType::ELLIPSIS) {
        return true;
    }
    
    return false;
}

bool TypeSystem::isDictTypeCompatible(const TypeAnnotation& source, const TypeAnnotation& target) {
    // 检查键类型是否兼容
    if (source.keyType && target.keyType && 
        !isTypeCompatible(*source.keyType, *target.keyType)) {
        return false;
    }
    
    // 检查值类型是否兼容
    if (source.valueType && target.valueType && 
        !isTypeCompatible(*source.valueType, *target.valueType)) {
        return false;
    }
    
    // 如果目标字典的键或值类型是任意类型，则兼容
    if ((target.keyType && target.keyType->types.size() == 1 && 
         target.keyType->types[0] == TokenType::ELLIPSIS) ||
        (target.valueType && target.valueType->types.size() == 1 && 
         target.valueType->types[0] == TokenType::ELLIPSIS)) {
        return true;
    }
    
    return true;
}

} // namespace Zelo