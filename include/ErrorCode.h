// ErrorCode.h
#pragma once

#include <string>
#include <unordered_map>

namespace Zelo {

class ErrorCode {
public:
    enum Code {
        // 语法错误 (100-199)
        SYNTAX_ERROR = 100,
        UNEXPECTED_TOKEN = 101,
        MISSING_SEMICOLON = 102,
        MISSING_PAREN = 103,
        MISSING_BRACE = 104,
        MISSING_BRACKET = 105,
        INVALID_SYNTAX = 106,
        
        // 类型错误 (200-299)
        TYPE_ERROR = 200,
        TYPE_MISMATCH = 201,
        UNDEFINED_VARIABLE = 202,
        UNDEFINED_PROPERTY = 203,
        UNDEFINED_FUNCTION = 204,
        INVALID_OPERATION = 205,
        INVALID_TYPE_ANNOTATION = 206,
        INVALID_UNION_TYPE = 207,
        INVALID_ARRAY_TYPE = 208,
        INVALID_DICT_TYPE = 209,
        
        // 运行时错误 (300-399)
        RUNTIME_ERROR = 300,
        DIVISION_BY_ZERO = 301,
        INDEX_OUT_OF_BOUNDS = 302,
        KEY_NOT_FOUND = 303,
        STACK_OVERFLOW = 304,
        OUT_OF_MEMORY = 305,
        INVALID_ARGUMENT = 306,
        INVALID_RETURN = 307,
        RECURSION_DEPTH_EXCEEDED = 308,
        
        // 模块错误 (400-499)
        MODULE_ERROR = 400,
        MODULE_NOT_FOUND = 401,
        IMPORT_ERROR = 402,
        EXPORT_ERROR = 403,
        CIRCULAR_IMPORT = 404,
        MODULE_LOAD_ERROR = 405,
        
        // 宏错误 (500-599)
        MACRO_ERROR = 500,
        MACRO_NOT_DEFINED = 501,
        MACRO_ARGUMENT_MISMATCH = 502,
        MACRO_RECURSION = 503,
        MACRO_EXPANSION_ERROR = 504,
        
        // 内部错误 (600-699)
        INTERNAL_ERROR = 600,
        NOT_IMPLEMENTED = 601,
        COMPILER_ERROR = 602,
        PARSER_ERROR = 603,
        
        // 垃圾回收错误 (700-799)
        GC_ERROR = 700,
        GC_MEMORY_LEAK = 701,
        GC_CYCLE_DETECTION_FAILED = 702
    };
    
    static std::string getMessage(Code code, const std::string& details = "") {
        static std::unordered_map<Code, std::string> messages = {
            // 语法错误
            {SYNTAX_ERROR, "Syntax error"},
            {UNEXPECTED_TOKEN, "Unexpected token"},
            {MISSING_SEMICOLON, "Missing semicolon"},
            {MISSING_PAREN, "Missing parenthesis"},
            {MISSING_BRACE, "Missing brace"},
            {MISSING_BRACKET, "Missing bracket"},
            {INVALID_SYNTAX, "Invalid syntax"},
            
            // 类型错误
            {TYPE_ERROR, "Type error"},
            {TYPE_MISMATCH, "Type mismatch"},
            {UNDEFINED_VARIABLE, "Undefined variable"},
            {UNDEFINED_PROPERTY, "Undefined property"},
            {UNDEFINED_FUNCTION, "Undefined function"},
            {INVALID_OPERATION, "Invalid operation"},
            {INVALID_TYPE_ANNOTATION, "Invalid type annotation"},
            {INVALID_UNION_TYPE, "Invalid union type"},
            {INVALID_ARRAY_TYPE, "Invalid array type"},
            {INVALID_DICT_TYPE, "Invalid dictionary type"},
            
            // 运行时错误
            {RUNTIME_ERROR, "Runtime error"},
            {DIVISION_BY_ZERO, "Division by zero"},
            {INDEX_OUT_OF_BOUNDS, "Index out of bounds"},
            {KEY_NOT_FOUND, "Key not found"},
            {STACK_OVERFLOW, "Stack overflow"},
            {OUT_OF_MEMORY, "Out of memory"},
            {INVALID_ARGUMENT, "Invalid argument"},
            {INVALID_RETURN, "Invalid return"},
            {RECURSION_DEPTH_EXCEEDED, "Recursion depth exceeded"},
            
            // 模块错误
            {MODULE_ERROR, "Module error"},
            {MODULE_NOT_FOUND, "Module not found"},
            {IMPORT_ERROR, "Import error"},
            {EXPORT_ERROR, "Export error"},
            {CIRCULAR_IMPORT, "Circular import detected"},
            {MODULE_LOAD_ERROR, "Module load error"},
            
            // 宏错误
            {MACRO_ERROR, "Macro error"},
            {MACRO_NOT_DEFINED, "Macro not defined"},
            {MACRO_ARGUMENT_MISMATCH, "Macro argument mismatch"},
            {MACRO_RECURSION, "Macro recursion detected"},
            {MACRO_EXPANSION_ERROR, "Macro expansion error"},
            
            // 内部错误
            {INTERNAL_ERROR, "Internal error"},
            {NOT_IMPLEMENTED, "Not implemented"},
            {COMPILER_ERROR, "Compiler error"},
            {PARSER_ERROR, "Parser error"},
            
            // 垃圾回收错误
            {GC_ERROR, "Garbage collection error"},
            {GC_MEMORY_LEAK, "Memory leak detected"},
            {GC_CYCLE_DETECTION_FAILED, "Cycle detection failed"}
        };
        
        auto it = messages.find(code);
        if (it != messages.end()) {
            return it->second + (details.empty() ? "" : ": " + details);
        }
        return "Unknown error: " + std::to_string(static_cast<int>(code));
    }
    
    static std::string getCodeName(Code code) {
        static std::unordered_map<Code, std::string> names = {
            // 语法错误
            {SYNTAX_ERROR, "SYNTAX_ERROR"},
            {UNEXPECTED_TOKEN, "UNEXPECTED_TOKEN"},
            {MISSING_SEMICOLON, "MISSING_SEMICOLON"},
            {MISSING_PAREN, "MISSING_PAREN"},
            {MISSING_BRACE, "MISSING_BRACE"},
            {MISSING_BRACKET, "MISSING_BRACKET"},
            {INVALID_SYNTAX, "INVALID_SYNTAX"},
            
            // 类型错误
            {TYPE_ERROR, "TYPE_ERROR"},
            {TYPE_MISMATCH, "TYPE_MISMATCH"},
            {UNDEFINED_VARIABLE, "UNDEFINED_VARIABLE"},
            {UNDEFINED_PROPERTY, "UNDEFINED_PROPERTY"},
            {UNDEFINED_FUNCTION, "UNDEFINED_FUNCTION"},
            {INVALID_OPERATION, "INVALID_OPERATION"},
            {INVALID_TYPE_ANNOTATION, "INVALID_TYPE_ANNOTATION"},
            {INVALID_UNION_TYPE, "INVALID_UNION_TYPE"},
            {INVALID_ARRAY_TYPE, "INVALID_ARRAY_TYPE"},
            {INVALID_DICT_TYPE, "INVALID_DICT_TYPE"},
            
            // 运行时错误
            {RUNTIME_ERROR, "RUNTIME_ERROR"},
            {DIVISION_BY_ZERO, "DIVISION_BY_ZERO"},
            {INDEX_OUT_OF_BOUNDS, "INDEX_OUT_OF_BOUNDS"},
            {KEY_NOT_FOUND, "KEY_NOT_FOUND"},
            {STACK_OVERFLOW, "STACK_OVERFLOW"},
            {OUT_OF_MEMORY, "OUT_OF_MEMORY"},
            {INVALID_ARGUMENT, "INVALID_ARGUMENT"},
            {INVALID_RETURN, "INVALID_RETURN"},
            {RECURSION_DEPTH_EXCEEDED, "RECURSION_DEPTH_EXCEEDED"},
            
            // 模块错误
            {MODULE_ERROR, "MODULE_ERROR"},
            {MODULE_NOT_FOUND, "MODULE_NOT_FOUND"},
            {IMPORT_ERROR, "IMPORT_ERROR"},
            {EXPORT_ERROR, "EXPORT_ERROR"},
            {CIRCULAR_IMPORT, "CIRCULAR_IMPORT"},
            {MODULE_LOAD_ERROR, "MODULE_LOAD_ERROR"},
            
            // 宏错误
            {MACRO_ERROR, "MACRO_ERROR"},
            {MACRO_NOT_DEFINED, "MACRO_NOT_DEFINED"},
            {MACRO_ARGUMENT_MISMATCH, "MACRO_ARGUMENT_MISMATCH"},
            {MACRO_RECURSION, "MACRO_RECURSION"},
            {MACRO_EXPANSION_ERROR, "MACRO_EXPANSION_ERROR"},
            
            // 内部错误
            {INTERNAL_ERROR, "INTERNAL_ERROR"},
            {NOT_IMPLEMENTED, "NOT_IMPLEMENTED"},
            {COMPILER_ERROR, "COMPILER_ERROR"},
            {PARSER_ERROR, "PARSER_ERROR"},
            
            // 垃圾回收错误
            {GC_ERROR, "GC_ERROR"},
            {GC_MEMORY_LEAK, "GC_MEMORY_LEAK"},
            {GC_CYCLE_DETECTION_FAILED, "GC_CYCLE_DETECTION_FAILED"}
        };
        
        auto it = names.find(code);
        if (it != names.end()) {
            return it->second;
        }
        return "UNKNOWN_ERROR";
    }
};

} // namespace Zelo