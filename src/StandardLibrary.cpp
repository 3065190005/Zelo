#include "StandardLibrary.h"
#include "Interpreter.h"
#include "BuiltinFunctions.h"
#include <iostream>
#include <cmath>
#include <random>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>

namespace Zelo {

// 初始化模块加载器映射
std::unordered_map<std::string, std::function<Value()>> StandardLibrary::moduleLoaders = {
    {"io", ioModule},
    {"os", osModule},
    {"string", stringModule},
    {"math", mathModule},
    {"regex", regexModule},
    {"datetime", datetimeModule},
    {"random", randomModule},
    {"asynco", asyncoModule},
    {"json", jsonModule},
    {"base64", base64Module},
    {"http", httpModule},
    {"hash", hashModule},
    {"zip", zipModule},
    {"gzip", gzipModule},
    {"tar", tarModule},
    {"image", imageModule},
    {"audio", audioModule},
    {"video", videoModule},
    {"zeeui", zeeuiModule},
    {"zeewidget", zeewidgetModule}
};

void StandardLibrary::initialize() {
    // 标准库初始化
    // 这里可以添加全局标准库初始化代码
}

Value StandardLibrary::loadModule(const std::string& moduleName) {
    auto it = moduleLoaders.find(moduleName);
    if (it != moduleLoaders.end()) {
        return it->second();
    }
    throw RuntimeError("Standard library module not found: " + moduleName, 0);
}

void StandardLibrary::defineFunction(const std::shared_ptr<Environment>& env, 
                                   const std::string& name,
                                   std::function<Value(const std::vector<Value>&)> func,
                                   int arity) {
    auto zeloFunc = std::make_shared<ZeloFunction>(nullptr, nullptr);
    zeloFunc->call = [func](Interpreter* interpreter, const std::vector<Value>& arguments) -> Value {
        return func(arguments);
    };
    zeloFunc->arity = [arity]() -> int { return arity; };
    env->define(name, zeloFunc);
}

// IO 模块实现
Value StandardLibrary::ioModule() {
    auto env = std::make_shared<Environment>();
    
    defineFunction(env, "readFile", [](const std::vector<Value>& args) -> Value {
        if (args.size() != 1) {
            throw RuntimeError("readFile() expects exactly 1 argument", 0);
        }
        
        if (!std::holds_alternative<std::string>(args[0])) {
            throw RuntimeError("readFile() argument must be string", 0);
        }
        
        std::string filename = std::get<std::string>(args[0]);
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw RuntimeError("Cannot open file: " + filename, 0);
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }, 1);
    
    defineFunction(env, "writeFile", [](const std::vector<Value>& args) -> Value {
        if (args.size() != 2) {
            throw RuntimeError("writeFile() expects exactly 2 arguments", 0);
        }
        
        if (!std::holds_alternative<std::string>(args[0]) || !std::holds_alternative<std::string>(args[1])) {
            throw RuntimeError("writeFile() arguments must be strings", 0);
        }
        
        std::string filename = std::get<std::string>(args[0]);
        std::string content = std::get<std::string>(args[1]);
        
        std::ofstream file(filename);
        if (!file.is_open()) {
            throw RuntimeError("Cannot open file: " + filename, 0);
        }
        
        file << content;
        return Value();
    }, 2);
    
    defineFunction(env, "appendFile", [](const std::vector<Value>& args) -> Value {
        if (args.size() != 2) {
            throw RuntimeError("appendFile() expects exactly 2 arguments", 0);
        }
        
        if (!std::holds_alternative<std::string>(args[0]) || !std::holds_alternative<std::string>(args[1])) {
            throw RuntimeError("appendFile() arguments must be strings", 0);
        }
        
        std::string filename = std::get<std::string>(args[0]);
        std::string content = std::get<std::string>(args[1]);
        
        std::ofstream file(filename, std::ios_base::app);
        if (!file.is_open()) {
            throw RuntimeError("Cannot open file: " + filename, 0);
        }
        
        file << content;
        return Value();
    }, 2);
    
    defineFunction(env, "fileExists", [](const std::vector<Value>& args) -> Value {
        if (args.size() != 1) {
            throw RuntimeError("fileExists() expects exactly 1 argument", 0);
        }
        
        if (!std::holds_alternative<std::string>(args[0])) {
            throw RuntimeError("fileExists() argument must be string", 0);
        }
        
        std::string filename = std::get<std::string>(args[0]);
        std::ifstream file(filename);
        return file.is_open();
    }, 1);
    
    return env;
}

// OS 模块实现
Value StandardLibrary::osModule() {
    auto env = std::make_shared<Environment>();
    
    defineFunction(env, "getenv", [](const std::vector<Value>& args) -> Value {
        if (args.size() != 1) {
            throw RuntimeError("getenv() expects exactly 1 argument", 0);
        }
        
        if (!std::holds_alternative<std::string>(args[0])) {
            throw RuntimeError("getenv() argument must be string", 0);
        }
        
        std::string varName = std::get<std::string>(args[0]);
        const char* value = std::getenv(varName.c_str());
        
        if (value) {
            return std::string(value);
        }
        return Value();
    }, 1);
    
    defineFunction(env, "setenv", [](const std::vector<Value>& args) -> Value {
        if (args.size() != 2) {
            throw RuntimeError("setenv() expects exactly 2 arguments", 0);
        }
        
        if (!std::holds_alternative<std::string>(args[0]) || !std::holds_alternative<std::string>(args[1])) {
            throw RuntimeError("setenv() arguments must be strings", 0);
        }
        
        std::string varName = std::get<std::string>(args[0]);
        std::string value = std::get<std::string>(args[1]);
        
        #ifdef _WIN32
        return _putenv_s(varName.c_str(), value.c_str()) == 0;
        #else
        return setenv(varName.c_str(), value.c_str(), 1) == 0;
        #endif
    }, 2);
    
    defineFunction(env, "system", [](const std::vector<Value>& args) -> Value {
        if (args.size() != 1) {
            throw RuntimeError("system() expects exactly 1 argument", 0);
        }
        
        if (!std::holds_alternative<std::string>(args[0])) {
            throw RuntimeError("system() argument must be string", 0);
        }
        
        std::string command = std::get<std::string>(args[0]);
        return std::system(command.c_str());
    }, 1);
    
    defineFunction(env, "exit", [](const std::vector<Value>& args) -> Value {
        int code = 0;
        if (args.size() > 0) {
            if (std::holds_alternative<int>(args[0])) {
                code = std::get<int>(args[0]);
            } else {
                throw RuntimeError("exit() expects integer argument", 0);
            }
        }
        
        std::exit(code);
        return Value();
    }, -1);
    
    return env;
}

// String 模块实现
Value StandardLibrary::stringModule() {
    auto env = std::make_shared<Environment>();
    
    defineFunction(env, "length", [](const std::vector<Value>& args) -> Value {
        if (args.size() != 1) {
            throw RuntimeError("length() expects exactly 1 argument", 0);
        }
        
        if (!std::holds_alternative<std::string>(args[0])) {
            throw RuntimeError("length() argument must be string", 0);
        }
        
        return static_cast<int>(std::get<std::string>(args[0]).length());
    }, 1);
    
    defineFunction(env, "substring", [](const std::vector<Value>& args) -> Value {
        if (args.size() != 3) {
            throw RuntimeError("substring() expects exactly 3 arguments", 0);
        }
        
        if (!std::holds_alternative<std::string>(args[0]) || 
            !std::holds_alternative<int>(args[1]) || 
            !std::holds_alternative<int>(args[2])) {
            throw RuntimeError("substring() arguments must be string, int, int", 0);
        }
        
        std::string str = std::get<std::string>(args[0]);
        int start = std::get<int>(args[1]);
        int end = std::get<int>(args[2]);
        
        if (start < 0) start = 0;
        if (end > str.length()) end = str.length();
        if (start > end) return "";
        
        return str.substr(start, end - start);
    }, 3);
    
    defineFunction(env, "toUpper", [](const std::vector<Value>& args) -> Value {
        if (args.size() != 1) {
            throw RuntimeError("toUpper() expects exactly 1 argument", 0);
        }
        
        if (!std::holds_alternative<std::string>(args[0])) {
            throw RuntimeError("toUpper() argument must be string", 0);
        }
        
        std::string str = std::get<std::string>(args[0]);
        std::transform(str.begin(), str.end(), str.begin(), ::toupper);
        return str;
    }, 1);
    
    defineFunction(env, "toLower", [](const std::vector<Value>& args) -> Value {
        if (args.size() != 1) {
            throw RuntimeError("toLower() expects exactly 1 argument", 0);
        }
        
        if (!std::holds_alternative<std::string>(args[0])) {
            throw RuntimeError("toLower() argument must be string", 0);
        }
        
        std::string str = std::get<std::string>(args[0]);
        std::transform(str.begin(), str.end(), str.begin(), ::tolower);
        return str;
    }, 1);
    
    defineFunction(env, "trim", [](const std::vector<Value>& args) -> Value {
        if (args.size() != 1) {
            throw RuntimeError("trim() expects exactly 1 argument", 0);
        }
        
        if (!std::holds_alternative<std::string>(args[0])) {
            throw RuntimeError("trim() argument must be string", 0);
        }
        
        std::string str = std::get<std::string>(args[0]);
        
        // 去除左侧空白
        str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](unsigned char ch) {
            return !std::isspace(ch);
        }));
        
        // 去除右侧空白
        str.erase(std::find_if(str.rbegin(), str.rend(), [](unsigned char ch) {
            return !std::isspace(ch);
        }).base(), str.end());
        
        return str;
    }, 1);
    
    defineFunction(env, "split", [](const std::vector<Value>& args) -> Value {
        if (args.size() < 1 || args.size() > 2) {
            throw RuntimeError("split() expects 1 or 2 arguments", 0);
        }
        
        if (!std::holds_alternative<std::string>(args[0])) {
            throw RuntimeError("split() first argument must be string", 0);
        }
        
        std::string str = std::get<std::string>(args[0]);
        std::string delimiter = " ";
        
        if (args.size() == 2) {
            if (!std::holds_alternative<std::string>(args[1])) {
                throw RuntimeError("split() second argument must be string", 0);
            }
            delimiter = std::get<std::string>(args[1]);
        }
        
        auto result = std::make_shared<std::vector<Value>>();
        size_t pos = 0;
        std::string token;
        
        while ((pos = str.find(delimiter)) != std::string::npos) {
            token = str.substr(0, pos);
            result->push_back(token);
            str.erase(0, pos + delimiter.length());
        }
        
        result->push_back(str);
        return result;
    }, -1);
    
    defineFunction(env, "join", [](const std::vector<Value>& args) -> Value {
        if (args.size() != 2) {
            throw RuntimeError("join() expects exactly 2 arguments", 0);
        }
        
        if (!std::holds_alternative<std::shared_ptr<std::vector<Value>>>(args[0]) || 
            !std::holds_alternative<std::string>(args[1])) {
            throw RuntimeError("join() arguments must be array and string", 0);
        }
        
        auto array = std::get<std::shared_ptr<std::vector<Value>>>(args[0]);
        std::string delimiter = std::get<std::string>(args[1]);
        
        std::string result;
        for (size_t i = 0; i < array->size(); i++) {
            if (i > 0) {
                result += delimiter;
            }
            result += valueToString((*array)[i]);
        }
        
        return result;
    }, 2);
    
    return env;
}

// Math 模块实现
Value StandardLibrary::mathModule() {
    auto env = std::make_shared<Environment>();
    
    // 数学常量
    env->define("PI", 3.14159265358979323846);
    env->define("E", 2.71828182845904523536);
    
    // 数学函数
    defineFunction(env, "sin", [](const std::vector<Value>& args) -> Value {
        if (args.size() != 1) {
            throw RuntimeError("sin() expects exactly 1 argument", 0);
        }
        
        if (std::holds_alternative<int>(args[0])) {
            return std::sin(std::get<int>(args[0]));
        } else if (std::holds_alternative<double>(args[0])) {
            return std::sin(std::get<double>(args[0]));
        } else {
            throw RuntimeError("sin() argument must be number", 0);
        }
    }, 1);
    
    defineFunction(env, "cos", [](const std::vector<Value>& args) -> Value {
        if (args.size() != 1) {
            throw RuntimeError("cos() expects exactly 1 argument", 0);
        }
        
        if (std::holds_alternative<int>(args[0])) {
            return std::cos(std::get<int>(args[0]));
        } else if (std::holds_alternative<double>(args[0])) {
            return std::cos(std::get<double>(args[0]));
        } else {
            throw RuntimeError("cos() argument must be number", 0);
        }
    }, 1);
    
    defineFunction(env, "tan", [](const std::vector<Value>& args) -> Value {
        if (args.size() != 1) {
            throw RuntimeError("tan() expects exactly 1 argument", 0);
        }
        
        if (std::holds_alternative<int>(args[0])) {
            return std::tan(std::get<int>(args[0]));
        } else if (std::holds_alternative<double>(args[0])) {
            return std::tan(std::get<double>(args[0]));
        } else {
            throw RuntimeError("tan() argument must be number", 0);
        }
    }, 1);
    
    defineFunction(env, "sqrt", [](const std::vector<Value>& args) -> Value {
        if (args.size() != 1) {
            throw RuntimeError("sqrt() expects exactly 1 argument", 0);
        }
        
        if (std::holds_alternative<int>(args[0])) {
            int value = std::get<int>(args[0]);
            if (value < 0) {
                throw RuntimeError("sqrt() argument must be non-negative", 0);
            }
            return std::sqrt(value);
        } else if (std::holds_alternative<double>(args[0])) {
            double value = std::get<double>(args[0]);
            if (value < 0) {
                throw RuntimeError("sqrt() argument must be non-negative", 0);
            }
            return std::sqrt(value);
        } else {
            throw RuntimeError("sqrt() argument must be number", 0);
        }
    }, 1);
    
    defineFunction(env, "pow", [](const std::vector<Value>& args) -> Value {
        if (args.size() != 2) {
            throw RuntimeError("pow() expects exactly 2 arguments", 0);
        }
        
        double base, exponent;
        
        if (std::holds_alternative<int>(args[0])) {
            base = std::get<int>(args[0]);
        } else if (std::holds_alternative<double>(args[0])) {
            base = std::get<double>(args[0]);
        } else {
            throw RuntimeError("pow() first argument must be number", 0);
        }
        
        if (std::holds_alternative<int>(args[1])) {
            exponent = std::get<int>(args[1]);
        } else if (std::holds_alternative<double>(args[1])) {
            exponent = std::get<double>(args[1]);
        } else {
            throw RuntimeError("pow() second argument must be number", 0);
        }
        
        return std::pow(base, exponent);
    }, 2);
    
    defineFunction(env, "log", [](const std::vector<Value>& args) -> Value {
        if (args.size() != 1) {
            throw RuntimeError("log() expects exactly 1 argument", 0);
        }
        
        if (std::holds_alternative<int>(args[0])) {
            int value = std::get<int>(args[0]);
            if (value <= 0) {
                throw RuntimeError("log() argument must be positive", 0);
            }
            return std::log(value);
        } else if (std::holds_alternative<double>(args[0])) {
            double value = std::get<double>(args[0]);
            if (value <= 0) {
                throw RuntimeError("log() argument must be positive", 0);
            }
            return std::log(value);
        } else {
            throw RuntimeError("log() argument must be number", 0);
        }
    }, 1);
    
    defineFunction(env, "exp", [](const std::vector<Value>& args) -> Value {
        if (args.size() != 1) {
            throw RuntimeError("exp() expects exactly 1 argument", 0);
        }
        
        if (std::holds_alternative<int>(args[0])) {
            return std::exp(std::get<int>(args[0]));
        } else if (std::holds_alternative<double>(args[0])) {
            return std::exp(std::get<double>(args[0]));
        } else {
            throw RuntimeError("exp() argument must be number", 0);
        }
    }, 1);
    
    defineFunction(env, "abs", [](const std::vector<Value>& args) -> Value {
        if (args.size() != 1) {
            throw RuntimeError("abs() expects exactly 1 argument", 0);
        }
        
        if (std::holds_alternative<int>(args[0])) {
            return std::abs(std::get<int>(args[0]));
        } else if (std::holds_alternative<double>(args[0])) {
            return std::abs(std::get<double>(args[0]));
        } else {
            throw RuntimeError("abs() argument must be number", 0);
        }
    }, 1);
    
    defineFunction(env, "round", [](const std::vector<Value>& args) -> Value {
        if (args.size() != 1) {
            throw RuntimeError("round() expects exactly 1 argument", 0);
        }
        
        if (std::holds_alternative<int>(args[0])) {
            return std::get<int>(args[0]);
        } else if (std::holds_alternative<double>(args[0])) {
            return std::round(std::get<double>(args[0]));
        } else {
            throw RuntimeError("round() argument must be number", 0);
        }
    }, 1);
    
    defineFunction(env, "floor", [](const std::vector<Value>& args) -> Value {
        if (args.size() != 1) {
            throw RuntimeError("floor() expects exactly 1 argument", 0);
        }
        
        if (std::holds_alternative<int>(args[0])) {
            return std::get<int>(args[0]);
        } else if (std::holds_alternative<double>(args[0])) {
            return std::floor(std::get<double>(args[0]));
        } else {
            throw RuntimeError("floor() argument must be number", 0);
        }
    }, 1);
    
    defineFunction(env, "ceil", [](const std::vector<Value>& args) -> Value {
        if (args.size() != 1) {
            throw RuntimeError("ceil() expects exactly 1 argument", 0);
        }
        
        if (std::holds_alternative<int>(args[0])) {
            return std::get<int>(args[0]);
        } else if (std::holds_alternative<double>(args[0])) {
            return std::ceil(std::get<double>(args[0]));
        } else {
            throw RuntimeError("ceil() argument must be number", 0);
        }
    }, 1);
    
    return env;
}

// Regex 模块实现（占位符）
Value StandardLibrary::regexModule() {
    auto env = std::make_shared<Environment>();
    
    defineFunction(env, "match", [](const std::vector<Value>& args) -> Value {
        throw RuntimeError("regex.match() not implemented yet", 0);
    }, 2);
    
    defineFunction(env, "search", [](const std::vector<Value>& args) -> Value {
        throw RuntimeError("regex.search() not implemented yet", 0);
    }, 2);
    
    defineFunction(env, "replace", [](const std::vector<Value>& args) -> Value {
        throw RuntimeError("regex.replace() not implemented yet", 0);
    }, 3);
    
    return env;
}

// Datetime 模块实现
Value StandardLibrary::datetimeModule() {
    auto env = std::make_shared<Environment>();
    
    defineFunction(env, "now", [](const std::vector<Value>& args) -> Value {
        auto now = std::chrono::system_clock::now();
        auto now_time = std::chrono::system_clock::to_time_t(now);
        return static_cast<int>(now_time);
    }, 0);
    
    defineFunction(env, "format", [](const std::vector<Value>& args) -> Value {
        if (args.size() != 2) {
            throw RuntimeError("datetime.format() expects exactly 2 arguments", 0);
        }
        
        if (!std::holds_alternative<int>(args[0]) || !std::holds_alternative<std::string>(args[1])) {
            throw RuntimeError("datetime.format() arguments must be int and string", 0);
        }
        
        int timestamp = std::get<int>(args[0]);
        std::string format = std::get<std::string>(args[1]);
        
        std::time_t time = static_cast<std::time_t>(timestamp);
        std::tm* tm = std::localtime(&time);
        
        std::stringstream ss;
        ss << std::put_time(tm, format.c_str());
        return ss.str();
    }, 2);
    
    defineFunction(env, "parse", [](const std::vector<Value>& args) -> Value {
        if (args.size() != 2) {
            throw RuntimeError("datetime.parse() expects exactly 2 arguments", 0);
        }
        
        if (!std::holds_alternative<std::string>(args[0]) || !std::holds_alternative<std::string>(args[1])) {
            throw RuntimeError("datetime.parse() arguments must be string and string", 0);
        }
        
        std::string dateStr = std::get<std::string>(args[0]);
        std::string format = std::get<std::string>(args[1]);
        
        std::tm tm = {};
        std::istringstream ss(dateStr);
        ss >> std::get_time(&tm, format.c_str());
        
        if (ss.fail()) {
            throw RuntimeError("Failed to parse date string", 0);
        }
        
        return static_cast<int>(std::mktime(&tm));
    }, 2);
    
    return env;
}

// Random 模块实现
Value StandardLibrary::randomModule() {
    auto env = std::make_shared<Environment>();
    
    static std::random_device rd;
    static std::mt19937 gen(rd());
    
    defineFunction(env, "random", [](const std::vector<Value>& args) -> Value {
        std::uniform_real_distribution<> dis(0.0, 1.0);
        return dis(gen);
    }, 0);
    
    defineFunction(env, "randint", [](const std::vector<Value>& args) -> Value {
        if (args.size() != 2) {
            throw RuntimeError("randint() expects exactly 2 arguments", 0);
        }
        
        if (!std::holds_alternative<int>(args[0]) || !std::holds_alternative<int>(args[1])) {
            throw RuntimeError("randint() arguments must be integers", 0);
        }
        
        int min = std::get<int>(args[0]);
        int max = std::get<int>(args[1]);
        
        if (min > max) {
            throw RuntimeError("randint() min must be less than or equal to max", 0);
        }
        
        std::uniform_int_distribution<> dis(min, max);
        return dis(gen);
    }, 2);
    
    defineFunction(env, "choice", [](const std::vector<Value>& args) -> Value {
        if (args.size() != 1) {
            throw RuntimeError("choice() expects exactly 1 argument", 0);
        }
        
        if (!std::holds_alternative<std::shared_ptr<std::vector<Value>>>(args[0])) {
            throw RuntimeError("choice() argument must be array", 0);
        }
        
        auto array = std::get<std::shared_ptr<std::vector<Value>>>(args[0]);
        if (array->empty()) {
            throw RuntimeError("choice() array must not be empty", 0);
        }
        
        std::uniform_int_distribution<> dis(0, array->size() - 1);
        return (*array)[dis(gen)];
    }, 1);
    
    defineFunction(env, "shuffle", [](const std::vector<Value>& args) -> Value {
        if (args.size() != 1) {
            throw RuntimeError("shuffle() expects exactly 1 argument", 0);
        }
        
        if (!std::holds_alternative<std::shared_ptr<std::vector<Value>>>(args[0])) {
            throw RuntimeError("shuffle() argument must be array", 0);
        }
        
        auto array = std::get<std::shared_ptr<std::vector<Value>>>(args[0]);
        std::shuffle(array->begin(), array->end(), gen);
        return array;
    }, 1);
    
    return env;
}

// Asynco 模块实现（占位符）
Value StandardLibrary::asyncoModule() {
    auto env = std::make_shared<Environment>();
    
    defineFunction(env, "sleep", [](const std::vector<Value>& args) -> Value {
        throw RuntimeError("asynco.sleep() not implemented yet", 0);
    }, 1);
    
    defineFunction(env, "run", [](const std::vector<Value>& args) -> Value {
        throw RuntimeError("asynco.run() not implemented yet", 0);
    }, 1);
    
    defineFunction(env, "all", [](const std::vector<Value>& args) -> Value {
        throw RuntimeError("asynco.all() not implemented yet", 0);
    }, 1);
    
    return env;
}

// 其他标准库模块（占位符实现）
Value StandardLibrary::jsonModule() {
    auto env = std::make_shared<Environment>();
    // JSON 模块占位符
    return env;
}

Value StandardLibrary::base64Module() {
    auto env = std::make_shared<Environment>();
    // Base64 模块占位符
    return env;
}

Value StandardLibrary::httpModule() {
    auto env = std::make_shared<Environment>();
    // HTTP 模块占位符
    return env;
}

Value StandardLibrary::hashModule() {
    auto env = std::make_shared<Environment>();
    // Hash 模块占位符
    return env;
}

Value StandardLibrary::zipModule() {
    auto env = std::make_shared<Environment>();
    // Zip 模块占位符
    return env;
}

Value StandardLibrary::gzipModule() {
    auto env = std::make_shared<Environment>();
    // Gzip 模块占位符
    return env;
}

Value StandardLibrary::tarModule() {
    auto env = std::make_shared<Environment>();
    // Tar 模块占位符
    return env;
}

Value StandardLibrary::imageModule() {
    auto env = std::make_shared<Environment>();
    // Image 模块占位符
    return env;
}

Value StandardLibrary::audioModule() {
    auto env = std::make_shared<Environment>();
    // Audio 模块占位符
    return env;
}

Value StandardLibrary::videoModule() {
    auto env = std::make_shared<Environment>();
    // Video 模块占位符
    return env;
}

Value StandardLibrary::zeeuiModule() {
    auto env = std::make_shared<Environment>();
    // ZeeUI 模块占位符
    return env;
}

Value StandardLibrary::zeewidgetModule() {
    auto env = std::make_shared<Environment>();
    // ZeeWidget 模块占位符
    return env;
}

} // namespace Zelo