#pragma once

#include "AST.h"
#include "Value.h"
#include <memory>
#include <vector>

namespace Zelo {

class TypeSystem {
public:
    static bool checkType(const Value& value, const TypeAnnotation& type);
    static Value castValue(const Value& value, const TypeAnnotation& type);
    
    static bool isTypeCompatible(const TypeAnnotation& source, const TypeAnnotation& target);
    static std::string typeAnnotationToString(const TypeAnnotation& type);
    
private:
    static bool checkBasicType(const Value& value, TokenType type);
    static bool checkUnionType(const Value& value, const std::vector<TokenType>& types);
    static bool checkArrayType(const Value& value, const TypeAnnotation& elementType);
    static bool checkDictType(const Value& value, const TypeAnnotation& keyType, const TypeAnnotation& valueType);
    
    static Value castToBasicType(const Value& value, TokenType targetType);
    static Value castToArrayType(const Value& value, const TypeAnnotation& elementType);
    static Value castToDictType(const Value& value, const TypeAnnotation& keyType, const TypeAnnotation& valueType);
    
    static bool isBasicTypeCompatible(TokenType source, TokenType target);
    static bool isArrayTypeCompatible(const TypeAnnotation& source, const TypeAnnotation& target);
    static bool isDictTypeCompatible(const TypeAnnotation& source, const TypeAnnotation& target);
};

} // namespace Zelo