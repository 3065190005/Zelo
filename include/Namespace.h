#pragma once

#include "AST.h"
#include "Value.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace Zelo {

class NamespaceManager {
public:
    static NamespaceManager& getInstance();
    
    // 进入命名空间
    void enterNamespace(const std::string& name);
    
    // 退出命名空间
    void exitNamespace();
    
    // 获取当前命名空间全名
    std::string getCurrentNamespace() const;
    
    // 获取限定名（添加命名空间前缀）
    std::string qualifyName(const std::string& name) const;
    
    // 在命名空间中定义符号
    void define(const std::string& name, const Value& value);
    
    // 从命名空间中获取符号
    Value get(const std::string& name) const;
    
    // 设置命名空间别名
    void setAlias(const std::string& alias, const std::string& fullNamespace);
    
    // 通过别名获取命名空间全名
    std::string getNamespaceByAlias(const std::string& alias) const;
    
    // 清除所有命名空间状态（用于测试或重置）
    void clear();
    
private:
    NamespaceManager() = default;
    ~NamespaceManager() = default;
    
    // 禁止拷贝和赋值
    NamespaceManager(const NamespaceManager&) = delete;
    NamespaceManager& operator=(const NamespaceManager&) = delete;
    
    // 命名空间栈，表示当前所在的命名空间层次
    std::vector<std::string> namespaceStack;
    
    // 命名空间环境映射：命名空间全名 -> 环境
    std::unordered_map<std::string, std::shared_ptr<Environment>> namespaceEnvironments;
    
    // 命名空间别名映射：别名 -> 命名空间全名
    std::unordered_map<std::string, std::string> namespaceAliases;
};

// 命名空间解析器（用于解析命名空间相关的AST节点）
class NamespaceResolver {
public:
    explicit NamespaceResolver(std::shared_ptr<Environment> globalEnv);
    
    // 解析命名空间声明
    void resolveNamespaceDecl(const std::shared_ptr<NamespaceDeclStmt>& stmt);
    
    // 解析命名空间使用（进入命名空间）
    void enterNamespace(const std::string& name);
    
    // 解析命名空间使用（退出命名空间）
    void exitNamespace();
    
    // 解析命名空间别名
    void resolveNamespaceAlias(const std::string& alias, const std::string& fullNamespace);
    
    // 解析符号引用（处理命名空间限定名）
    Value resolveSymbol(const std::string& name) const;
    
    // 获取当前环境（考虑命名空间）
    std::shared_ptr<Environment> getCurrentEnvironment() const;
    
private:
    std::shared_ptr<Environment> globalEnvironment;
    NamespaceManager& namespaceManager;
};

} // namespace Zelo