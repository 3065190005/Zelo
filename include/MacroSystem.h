#pragma once

#include "Lexer.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace Zelo {

class MacroSystem {
public:
    struct MacroDefinition {
        std::vector<Token> parameters;
        std::vector<Token> body;
        bool isFunctionLike;
    };

    void defineMacro(const std::string& name, const std::vector<Token>& parameters, 
                    const std::vector<Token>& body, bool isFunctionLike);
    bool isMacroDefined(const std::string& name) const;
    std::vector<Token> expandMacro(const std::string& name, const std::vector<std::vector<Token>>& arguments);
    
    std::vector<Token> process(const std::vector<Token>& tokens);

private:
    std::unordered_map<std::string, MacroDefinition> macros;
    
    std::vector<Token> expandMacroCall(const std::vector<Token>& tokens, size_t& index);
    std::vector<std::vector<Token>> parseMacroArguments(const std::vector<Token>& tokens, size_t& index);
    std::vector<Token> replaceMacroParameters(const std::vector<Token>& body, 
                                             const std::vector<std::string>& parameters,
                                             const std::vector<std::vector<Token>>& arguments);
};

} // namespace Zelo