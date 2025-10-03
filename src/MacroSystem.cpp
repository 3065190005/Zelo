#include "MacroSystem.h"
#include <algorithm>
#include <stdexcept>

namespace Zelo {

void MacroSystem::defineMacro(const std::string& name, const std::vector<Token>& parameters, 
                             const std::vector<Token>& body, bool isFunctionLike) {
    macros[name] = {parameters, body, isFunctionLike};
}

bool MacroSystem::isMacroDefined(const std::string& name) const {
    return macros.find(name) != macros.end();
}

std::vector<Token> MacroSystem::expandMacro(const std::string& name, const std::vector<std::vector<Token>>& arguments) {
    auto it = macros.find(name);
    if (it == macros.end()) {
        return {};
    }
    
    const auto& macro = it->second;
    if (macro.isFunctionLike && macro.parameters.size() != arguments.size()) {
        throw std::runtime_error("Macro argument count mismatch");
    }
    
    return replaceMacroParameters(macro.body, macro.parameters, arguments);
}

std::vector<Token> MacroSystem::process(const std::vector<Token>& tokens) {
    std::vector<Token> result;
    result.reserve(tokens.size());
    
    for (size_t i = 0; i < tokens.size(); i++) {
        if (tokens[i].type == TokenType::MACRO) {
            // 宏定义
            if (i + 1 >= tokens.size() || tokens[i+1].type != TokenType::IDENTIFIER) {
                throw std::runtime_error("Expected macro name after 'macro'");
            }
            
            std::string macroName = tokens[i+1].value;
            i += 2;
            
            // 解析参数
            std::vector<Token> parameters;
            bool isFunctionLike = false;
            
            if (i < tokens.size() && tokens[i].type == TokenType::LPAREN) {
                isFunctionLike = true;
                i++;
                
                while (i < tokens.size() && tokens[i].type != TokenType::RPAREN) {
                    if (tokens[i].type == TokenType::IDENTIFIER) {
                        parameters.push_back(tokens[i]);
                    } else if (tokens[i].type == TokenType::COMMA) {
                        // 跳过逗号
                    } else {
                        throw std::runtime_error("Expected identifier in macro parameter list");
                    }
                    i++;
                }
                
                if (i >= tokens.size() || tokens[i].type != TokenType::RPAREN) {
                    throw std::runtime_error("Expected ')' after macro parameters");
                }
                i++;
            }
            
            // 解析宏体
            std::vector<Token> body;
            while (i < tokens.size() && tokens[i].type != TokenType::SEMICOLON) {
                body.push_back(tokens[i]);
                i++;
            }
            
            defineMacro(macroName, parameters, body, isFunctionLike);
        } else if (tokens[i].type == TokenType::IDENTIFIER && isMacroDefined(tokens[i].value)) {
            // 宏调用
            auto expanded = expandMacroCall(tokens, i);
            result.insert(result.end(), expanded.begin(), expanded.end());
        } else {
            result.push_back(tokens[i]);
        }
    }
    
    return result;
}

std::vector<Token> MacroSystem::expandMacroCall(const std::vector<Token>& tokens, size_t& index) {
    std::string macroName = tokens[index].value;
    index++;
    
    std::vector<std::vector<Token>> arguments;
    if (index < tokens.size() && tokens[index].type == TokenType::LPAREN) {
        // 函数式宏调用
        index++;
        arguments = parseMacroArguments(tokens, index);
        
        if (index >= tokens.size() || tokens[index].type != TokenType::RPAREN) {
            throw std::runtime_error("Expected ')' after macro arguments");
        }
        index++;
    }
    
    return expandMacro(macroName, arguments);
}

std::vector<std::vector<Token>> MacroSystem::parseMacroArguments(const std::vector<Token>& tokens, size_t& index) {
    std::vector<std::vector<Token>> arguments;
    std::vector<Token> currentArg;
    int parenLevel = 0;
    int braceLevel = 0;
    int bracketLevel = 0;
    
    while (index < tokens.size()) {
        if (tokens[index].type == TokenType::LPAREN) {
            parenLevel++;
        } else if (tokens[index].type == TokenType::RPAREN) {
            if (parenLevel == 0) {
                break;
            }
            parenLevel--;
        } else if (tokens[index].type == TokenType::LBRACE) {
            braceLevel++;
        } else if (tokens[index].type == TokenType::RBRACE) {
            braceLevel--;
        } else if (tokens[index].type == TokenType::LBRACKET) {
            bracketLevel++;
        } else if (tokens[index].type == TokenType::RBRACKET) {
            bracketLevel--;
        } else if (tokens[index].type == TokenType::COMMA && 
                  parenLevel == 0 && braceLevel == 0 && bracketLevel == 0) {
            // 只有在所有括号层级为0时，逗号才分隔参数
            if (!currentArg.empty()) {
                arguments.push_back(currentArg);
                currentArg.clear();
            }
            index++;
            continue;
        }
        
        currentArg.push_back(tokens[index]);
        index++;
    }
    
    if (!currentArg.empty()) {
        arguments.push_back(currentArg);
    }
    
    return arguments;
}

std::vector<Token> MacroSystem::replaceMacroParameters(const std::vector<Token>& body, 
                                                      const std::vector<std::string>& parameters,
                                                      const std::vector<std::vector<Token>>& arguments) {
    std::vector<Token> result;
    
    for (const auto& token : body) {
        if (token.type == TokenType::IDENTIFIER) {
            // 检查是否是宏参数
            auto it = std::find(parameters.begin(), parameters.end(), token.value);
            if (it != parameters.end()) {
                size_t index = it - parameters.begin();
                if (index < arguments.size()) {
                    // 替换为实际参数
                    result.insert(result.end(), arguments[index].begin(), arguments[index].end());
                    continue;
                }
            }
        }
        result.push_back(token);
    }
    
    return result;
}

} // namespace Zelo