#include "Namespace.h"
#include "Interpreter.h"
#include "RuntimeError.h"
#include <algorithm>

namespace Zelo {

NamespaceManager& NamespaceManager::getInstance() {
    static NamespaceManager instance;
    return instance;
}

void NamespaceManager::enterNamespace(const std::string& name) {
    namespaceStack.push_back(name);
    
    std::string fullName = getCurrentNamespace();
    
    // 如果这个命名空间环境还不存在，创建它
    if (namespaceEnvironments.find(fullName) == namespaceEnvironments.end()) {
        namespaceEnvironments[fullName] = std::make_shared<Environment>();
    }
}

void NamespaceManager::exitNamespace() {
    if (!namespaceStack.empty()) {
        namespaceStack.pop_back();
    }
}

std::string NamespaceManager::getCurrentNamespace() const {
    std::string result;
    for (const auto& part : namespaceStack) {
        if (!result.empty()) result += ".";
        result += part;
    }
    return result;
}

std::string NamespaceManager::qualifyName(const std::string& name) const {
    std::string currentNs = getCurrentNamespace();
    if (currentNs.empty()) return name;
    return currentNs + "." + name;
}

void NamespaceManager::define(const std::string& name, const Value& value) {
    std::string fullName = getCurrentNamespace();
    
    if (namespaceEnvironments.find(fullName) != namespaceEnvironments.end()) {
        namespaceEnvironments[fullName]->define(name, value);
    } else {
        throw RuntimeError("Namespace environment not found: " + fullName, 0);
    }
}

Value NamespaceManager::get(const std::string& name) const {
    std::string fullName = getCurrentNamespace();
    
    // 首先在当前命名空间中查找
    if (!fullName.empty()) {
        auto envIt = namespaceEnvironments.find(fullName);
        if (envIt != namespaceEnvironments.end()) {
            try {
                return envIt->second->get(name);
            } catch (const RuntimeError&) {
                // 不在当前命名空间中，继续查找
            }
        }
    }
    
    // 在全局环境中查找
    // 这里需要访问Interpreter的全局环境
    // 由于这是一个复杂的依赖关系，需要适当的设计
    
    throw RuntimeError("Undefined identifier in namespace: " + name, 0);
}

void NamespaceManager::setAlias(const std::string& alias, const std::string& fullNamespace) {
    namespaceAliases[alias] = fullNamespace;
}

std::string NamespaceManager::getNamespaceByAlias(const std::string& alias) const {
    auto it = namespaceAliases.find(alias);
    if (it != namespaceAliases.end()) {
        return it->second;
    }
    return "";
}

void NamespaceManager::clear() {
    namespaceStack.clear();
    namespaceEnvironments.clear();
    namespaceAliases.clear();
}

// NamespaceResolver 实现
NamespaceResolver::NamespaceResolver(std::shared_ptr<Environment> globalEnv)
    : globalEnvironment(globalEnv), namespaceManager(NamespaceManager::getInstance()) {}

void NamespaceResolver::resolveNamespaceDecl(const std::shared_ptr<NamespaceDeclStmt>& stmt) {
    // 进入命名空间
    enterNamespace(stmt->name.value);
    
    // 解析命名空间体内的所有语句
    // 这里需要访问Interpreter来执行语句
    // 由于这是一个复杂的依赖关系，需要适当的设计
    
    // 退出命名空间
    exitNamespace();
}

void NamespaceResolver::enterNamespace(const std::string& name) {
    namespaceManager.enterNamespace(name);
}

void NamespaceResolver::exitNamespace() {
    namespaceManager.exitNamespace();
}

void NamespaceResolver::resolveNamespaceAlias(const std::string& alias, const std::string& fullNamespace) {
    namespaceManager.setAlias(alias, fullNamespace);
}

Value NamespaceResolver::resolveSymbol(const std::string& name) const {
    // 检查是否是限定名（包含命名空间）
    size_t dotPos = name.find('.');
    if (dotPos != std::string::npos) {
        std::string namespacePart = name.substr(0, dotPos);
        std::string symbolName = name.substr(dotPos + 1);
        
        // 检查是否是别名
        std::string fullNamespace = namespaceManager.getNamespaceByAlias(namespacePart);
        if (fullNamespace.empty()) {
            fullNamespace = namespacePart;
        }
        
        // 保存当前命名空间状态
        std::vector<std::string> savedStack = namespaceManager.namespaceStack;
        
        // 进入目标命名空间
        namespaceManager.clear();
        size_t start = 0;
        size_t end = fullNamespace.find('.');
        while (end != std::string::npos) {
            namespaceManager.enterNamespace(fullNamespace.substr(start, end - start));
            start = end + 1;
            end = fullNamespace.find('.', start);
        }
        namespaceManager.enterNamespace(fullNamespace.substr(start));
        
        // 获取符号
        Value result;
        try {
            result = namespaceManager.get(symbolName);
        } catch (const RuntimeError& e) {
            // 恢复命名空间状态
            namespaceManager.clear();
            for (const auto& ns : savedStack) {
                namespaceManager.enterNamespace(ns);
            }
            throw e;
        }
        
        // 恢复命名空间状态
        namespaceManager.clear();
        for (const auto& ns : savedStack) {
            namespaceManager.enterNamespace(ns);
        }
        
        return result;
    }
    
    // 非限定名，在当前命名空间或全局环境中查找
    return namespaceManager.get(name);
}

std::shared_ptr<Environment> NamespaceResolver::getCurrentEnvironment() const {
    std::string currentNs = namespaceManager.getCurrentNamespace();
    
    if (currentNs.empty()) {
        return globalEnvironment;
    }
    
    auto it = namespaceManager.namespaceEnvironments.find(currentNs);
    if (it != namespaceManager.namespaceEnvironments.end()) {
        return it->second;
    }
    
    return globalEnvironment;
}

} // namespace Zelo