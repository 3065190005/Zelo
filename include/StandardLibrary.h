#ifndef STANDARDLIBRARY_H
#define STANDARDLIBRARY_H

#include "Value.h"
#include <memory>
#include <string>
#include <functional>

namespace Zelo {

class StandardLibrary {
public:
    static void initialize();
    static Value loadModule(const std::string& moduleName);
    
private:
    // 标准库模块实现
    static Value ioModule();
    static Value osModule();
    static Value stringModule();
    static Value mathModule();
    static Value regexModule();
    static Value datetimeModule();
    static Value randomModule();
    static Value asyncoModule();
    static Value jsonModule();
    static Value base64Module();
    static Value httpModule();
    static Value hashModule();
    static Value zipModule();
    static Value gzipModule();
    static Value tarModule();
    static Value imageModule();
    static Value audioModule();
    static Value videoModule();
    static Value zeeuiModule();
    static Value zeewidgetModule();
    
    // 工具函数
    static void defineFunction(const std::shared_ptr<Environment>& env, 
                              const std::string& name,
                              std::function<Value(const std::vector<Value>&)> func,
                              int arity = -1);
    
    // 模块加载器映射
    static std::unordered_map<std::string, std::function<Value()>> moduleLoaders;
};

} // namespace Zelo

#endif // STANDARDLIBRARY_H