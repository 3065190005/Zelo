#pragma once

#include "AST.h"
#include "Interpreter.h"
#include <unordered_map>
#include <filesystem>
#include <memory>

namespace Zelo {

class ModuleSystem {
public:
    ModuleSystem(Interpreter* interpreter);
    
    Value importModule(const std::string& modulePath, const std::vector<Token>& imports, const Token& alias);
    Value requireModule(const std::string& modulePath);
    Value includeModule(const std::string& modulePath);
    
    void exportSymbols(const std::vector<Token>& exports, const std::shared_ptr<Environment>& env);
    
    std::string resolveModulePath(const std::string& modulePath);
    std::shared_ptr<Environment> loadModule(const std::string& modulePath);
    
private:
    Interpreter* interpreter;
    std::unordered_map<std::string, std::shared_ptr<Environment>> moduleCache;
    std::filesystem::path basePath;
};

} // namespace Zelo