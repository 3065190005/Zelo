#include "ModuleSystem.h"
#include "Lexer.h"
#include "Parser.h"
#include "MacroSystem.h"
#include <fstream>
#include <sstream>
#include <iostream>

namespace Zelo {

ModuleSystem::ModuleSystem(Interpreter* interpreter) 
    : interpreter(interpreter) {
    // 设置基础路径（可以是当前目录或环境变量指定的路径）
    basePath = std::filesystem::current_path();
    
    // 检查环境变量 ZELO_PATH
    if (const char* zeloPath = std::getenv("ZELO_PATH")) {
        basePath = zeloPath;
    }
}

Value ModuleSystem::importModule(const std::string& modulePath, const std::vector<Token>& imports, const Token& alias) {
    std::string resolvedPath = resolveModulePath(modulePath);
    auto moduleEnv = loadModule(resolvedPath);
    
    // 获取模块的导出环境
    Value exportsValue;
    try {
        exportsValue = moduleEnv->get("__exports__");
    } catch (const RuntimeError&) {
        // 如果没有导出环境，则模块没有显式导出，使用整个模块环境
        exportsValue = moduleEnv;
    }
    
    if (!std::holds_alternative<std::shared_ptr<Environment>>(exportsValue)) {
        throw RuntimeError("Module does not have exports", 0);
    }
    
    auto exports = std::get<std::shared_ptr<Environment>>(exportsValue);
    
    // 如果没有指定导入项，导入所有导出项
    if (imports.empty()) {
        if (!alias.value.empty()) {
            interpreter->getGlobals()->define(alias.value, exports);
            return exports;
        } else {
            // 将所有导出项导入当前环境
            auto exportValues = exports->getValues();
            for (const auto& [name, value] : exportValues) {
                interpreter->getEnvironment()->define(name, value);
            }
            return Value();
        }
    }
    
    // 如果指定了别名，创建一个新的环境来存储导入的符号
    if (!alias.value.empty()) {
        auto importEnv = std::make_shared<Environment>();
        for (const auto& import : imports) {
            Value value = exports->get(import.value);
            importEnv->define(import.value, value);
        }
        interpreter->getGlobals()->define(alias.value, importEnv);
        return importEnv;
    }
    
    // 如果没有别名，直接将符号导入当前环境
    for (const auto& import : imports) {
        Value value = exports->get(import.value);
        interpreter->getEnvironment()->define(import.value, value);
    }
    
    return Value();
}

Value ModuleSystem::requireModule(const std::string& modulePath) {
    std::string resolvedPath = resolveModulePath(modulePath);
    
    // 检查缓存
    auto it = moduleCache.find(resolvedPath);
    if (it != moduleCache.end()) {
        return it->second;
    }
    
    // 加载模块
    auto moduleEnv = loadModule(resolvedPath);
    moduleCache[resolvedPath] = moduleEnv;
    
    return moduleEnv;
}

Value ModuleSystem::includeModule(const std::string& modulePath) {
    std::string resolvedPath = resolveModulePath(modulePath);
    
    // include 不检查缓存，每次都重新加载
    return loadModule(resolvedPath);
}

void ModuleSystem::exportSymbols(const std::vector<Token>& exports, const std::shared_ptr<Environment>& env) {
    // 创建一个新的环境来存储导出的符号
    auto exportEnv = std::make_shared<Environment>();
    
    for (const auto& exportItem : exports) {
        try {
            Value value = env->get(exportItem.value);
            exportEnv->define(exportItem.value, value);
        } catch (const RuntimeError& e) {
            throw RuntimeError("Cannot export undefined symbol: " + exportItem.value, 0);
        }
    }
    
    // 将导出环境存储到模块环境中
    env->define("__exports__", exportEnv);
}

std::string ModuleSystem::resolveModulePath(const std::string& modulePath) {
    std::filesystem::path path(modulePath);
    
    // 如果路径已经是绝对路径，直接返回
    if (path.is_absolute()) {
        return path.string();
    }
    
    // 检查文件扩展名
    if (path.extension().empty()) {
        path.replace_extension(".z");
    }
    
    // 首先在当前目录查找
    std::filesystem::path currentPath = std::filesystem::current_path() / path;
    if (std::filesystem::exists(currentPath)) {
        return currentPath.string();
    }
    
    // 在基础路径中查找
    std::filesystem::path baseModulePath = basePath / path;
    if (std::filesystem::exists(baseModulePath)) {
        return baseModulePath.string();
    }
    
    // 检查是否是标准库模块
    std::filesystem::path stdlibPath = basePath / "lib" / path;
    if (std::filesystem::exists(stdlibPath)) {
        return stdlibPath.string();
    }
    
    throw RuntimeError("Module not found: " + modulePath, 0);
}

std::shared_ptr<Environment> ModuleSystem::loadModule(const std::string& modulePath) {
    // 读取模块文件
    std::ifstream file(modulePath);
    if (!file.is_open()) {
        throw RuntimeError("Could not open module: " + modulePath, 0);
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();
    
    // 解析模块
    Lexer lexer(source);
    std::vector<Token> tokens = lexer.tokenize();
    
    // 宏处理
    MacroSystem macroSystem;
    std::vector<Token> processedTokens = macroSystem.process(tokens);
    
    Parser parser(processedTokens);
    std::vector<StmtPtr> statements = parser.parse();
    
    // 创建模块环境
    auto moduleEnv = std::make_shared<Environment>();
    
    // 执行模块代码
    interpreter->executeBlock(statements, moduleEnv);
    
    return moduleEnv;
}

} // namespace Zelo