// main.cpp
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "Lexer.h"
#include "Parser.h"
#include "Interpreter.h"
#include "MacroSystem.h"
#include "ErrorCode.h"

using namespace Zelo;

std::string readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + path);
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void printUsage() {
    std::cout << "Usage: zelo [options] [script]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -h, --help     Show this help message" << std::endl;
    std::cout << "  -v, --version  Show version information" << std::endl;
    std::cout << "  -c, --check    Check syntax without executing" << std::endl;
    std::cout << "  -e, --eval     Execute one line of script" << std::endl;
}

void printVersion() {
    std::cout << "Zelo v1.0.0" << std::endl;
    std::cout << "A dynamic scripting language implementation" << std::endl;
    std::cout << "Built with C++17 standard library" << std::endl;
}

int runFile(const std::string& filename, bool checkOnly = false) {
    try {
        std::string source = readFile(filename);
        
        // 词法分析
        Lexer lexer(source);
        std::vector<Token> tokens = lexer.tokenize();
        
        // 宏处理
        MacroSystem macroSystem;
        std::vector<Token> processedTokens = macroSystem.process(tokens);
        
        if (checkOnly) {
            std::cout << "Syntax check passed for " << filename << std::endl;
            return 0;
        }
        
        // 语法分析
        Parser parser(processedTokens);
        std::vector<StmtPtr> statements = parser.parse();
        
        // 解释执行
        Interpreter interpreter;
        interpreter.interpret(statements);
        
        return 0;
    } catch (const RuntimeError& error) {
        std::cerr << "Runtime error (" << ErrorCode::getCodeName(error.getCode()) 
                  << "): " << error.what() << " at line " << error.getLine() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}

int runRepl() {
    std::cout << "Zelo REPL v1.0.0 (Type 'exit' to quit, 'help' for help)" << std::endl;
    
    Interpreter interpreter;
    std::string line;
    int lineNumber = 1;
    
    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, line)) {
            break;
        }
        
        if (line.empty()) {
            continue;
        }
        
        if (line == "exit" || line == "quit") {
            break;
        }
        
        if (line == "help") {
            std::cout << "REPL commands:" << std::endl;
            std::cout << "  exit, quit  - Exit the REPL" << std::endl;
            std::cout << "  help        - Show this help" << std::endl;
            std::cout << "  clear       - Clear the screen" << std::endl;
            continue;
        }
        
        if (line == "clear") {
            // 简单的清屏（可能不适用于所有终端）
            std::cout << "\033[2J\033[1;1H";
            continue;
        }
        
        try {
            // 词法分析
            Lexer lexer(line);
            std::vector<Token> tokens = lexer.tokenize();
            
            // 宏处理
            MacroSystem macroSystem;
            std::vector<Token> processedTokens = macroSystem.process(tokens);
            
            // 语法分析
            Parser parser(processedTokens);
            std::vector<StmtPtr> statements = parser.parse();
            
            // 解释执行
            interpreter.interpret(statements);
            
        } catch (const RuntimeError& error) {
            std::cerr << "Runtime error (" << ErrorCode::getCodeName(error.getCode()) 
                      << "): " << error.what() << " at line " << error.getLine() << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
        
        lineNumber++;
    }
    
    return 0;
}

int evalString(const std::string& code) {
    try {
        // 词法分析
        Lexer lexer(code);
        std::vector<Token> tokens = lexer.tokenize();
        
        // 宏处理
        MacroSystem macroSystem;
        std::vector<Token> processedTokens = macroSystem.process(tokens);
        
        // 语法分析
        Parser parser(processedTokens);
        std::vector<StmtPtr> statements = parser.parse();
        
        // 解释执行
        Interpreter interpreter;
        interpreter.interpret(statements);
        
        return 0;
    } catch (const RuntimeError& error) {
        std::cerr << "Runtime error (" << ErrorCode::getCodeName(error.getCode()) 
                  << "): " << error.what() << " at line " << error.getLine() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}

int main(int argc, char* argv[]) {
    if (argc == 1) {
        // 没有参数，启动 REPL
        return runRepl();
    }
    
    std::vector<std::string> args(argv + 1, argv + argc);
    
    if (args[0] == "-h" || args[0] == "--help") {
        printUsage();
        return 0;
    }
    
    if (args[0] == "-v" || args[0] == "--version") {
        printVersion();
        return 0;
    }
    
    if (args[0] == "-c" || args[0] == "--check") {
        if (args.size() < 2) {
            std::cerr << "Error: No file specified for syntax check" << std::endl;
            return 1;
        }
        return runFile(args[1], true);
    }
    
    if (args[0] == "-e" || args[0] == "--eval") {
        if (args.size() < 2) {
            std::cerr << "Error: No code specified for evaluation" << std::endl;
            return 1;
        }
        return evalString(args[1]);
    }
    
    // 默认情况下，第一个参数是脚本文件
    return runFile(args[0]);
}