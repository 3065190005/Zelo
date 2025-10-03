cmake_minimum_required(VERSION 3.12)
project(zelo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置编译器警告选项
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_options(/W4 /WX)
endif()

# 设置输出目录
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 包含目录
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# 源文件列表
set(ZELO_SOURCES
    src/main.cpp
    src/Lexer.cpp
    src/Parser.cpp
    src/Value.cpp
    src/Interpreter.cpp
    src/MacroSystem.cpp
    src/TypeSystem.cpp
    src/ModuleSystem.cpp
    src/Namespace.cpp
    src/BuiltinFunctions.cpp
    src/GarbageCollector.cpp
    src/StandardLibrary.cpp
)

# 头文件列表
set(ZELO_HEADERS
    include/Lexer.h
    include/Parser.h
    include/AST.h
    include/Value.h
    include/Interpreter.h
    include/MacroSystem.h
    include/TypeSystem.h
    include/ModuleSystem.h
    include/Namespace.h
    include/BuiltinFunctions.h
    include/GarbageCollector.h
    include/StandardLibrary.h
    include/ErrorCode.h
)

# 创建可执行文件
add_executable(zelo ${ZELO_SOURCES} ${ZELO_HEADERS})

# 设置可执行文件属性
set_target_properties(zelo PROPERTIES
    OUTPUT_NAME "zelo"
    VERSION 1.0.0
)

# 如果是Windows平台，添加子系统设置
if(WIN32)
    set_target_properties(zelo PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:CONSOLE"
    )
endif()

# 安装目标
install(TARGETS zelo
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# 安装头文件
install(DIRECTORY include/
    DESTINATION include/zelo
    FILES_MATCHING PATTERN "*.h"
)

# 测试目标（可选）
if(BUILD_TESTING)
    enable_testing()
    
    # 添加测试可执行文件
    add_executable(zelo_test tests/SelfCheck.cpp ${ZELO_SOURCES} ${ZELO_HEADERS})
    target_link_libraries(zelo_test)
    
    # 添加测试
    add_test(NAME zelo_self_check COMMAND zelo_test)
endif()

# 打包配置
set(CPACK_PACKAGE_NAME "zelo")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_CONTACT "zelo-dev@example.com")
set(CPACK_PACKAGE_DESCRIPTION "Zelo Programming Language Interpreter")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A dynamic scripting language with strong typing support")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Zelo Development Team")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6, libstdc++6")
set(CPACK_RPM_PACKAGE_REQUIRES "libc.so.6, libstdc++.so.6")
include(CPack)