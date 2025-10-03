#include "GarbageCollector.h"
#include "Interpreter.h"
#include <algorithm>

namespace Zelo {

GarbageCollector& GarbageCollector::getInstance() {
    static GarbageCollector instance;
    return instance;
}

GarbageCollector::GarbageCollector() {
    // 初始化垃圾回收器
}

void GarbageCollector::registerObject(const std::shared_ptr<ZeloObject>& object) {
    // 新对象首先放入新生代
    newGenerationObjects.insert(object);
    objects.insert(object);
}

void GarbageCollector::registerFunction(const std::shared_ptr<ZeloFunction>& function) {
    // 新函数首先放入新生代
    newGenerationFunctions.insert(function);
    functions.insert(function);
}

void GarbageCollector::registerClass(const std::shared_ptr<ZeloClass>& klass) {
    // 新类首先放入新生代
    newGenerationClasses.insert(klass);
    classes.insert(klass);
}

void GarbageCollector::collectGarbage(const std::shared_ptr<Environment>& roots) {
    // 分带式收集：先收集新生代，如果不足再收集老生代
    if (!collectNewGeneration(roots)) {
        collectOldGeneration(roots);
    }
}

void GarbageCollector::collectGarbageIncremental(const std::shared_ptr<Environment>& roots) {
    // 增量式收集：将垃圾回收分成多个小步骤
    const size_t stepsPerCycle = 10; // 每个周期10步
    
    if (incrementalStep == 0) {
        // 第一步：标记根对象
        markedObjects.clear();
        markedFunctions.clear();
        markedClasses.clear();
        markEnvironment(roots);
        incrementalStep++;
    } else if (incrementalStep < stepsPerCycle) {
        // 中间步骤：标记一部分对象
        markIncremental(incrementalStep, stepsPerCycle);
        incrementalStep++;
    } else {
        // 最后一步：清除未标记的对象
        sweep();
        incrementalStep = 0;
    }
}

bool GarbageCollector::collectNewGeneration(const std::shared_ptr<Environment>& roots) {
    // 标记新生代对象
    markedObjects.clear();
    markedFunctions.clear();
    markedClasses.clear();
    
    markEnvironment(roots);
    
    // 清除未标记的新生代对象
    auto objectIt = newGenerationObjects.begin();
    while (objectIt != newGenerationObjects.end()) {
        if (markedObjects.find(*objectIt) == markedObjects.end()) {
            // 对象未被引用，清除
            objects.erase(*objectIt);
            objectIt = newGenerationObjects.erase(objectIt);
        } else {
            // 存活下来的对象晋升到老生代
            oldGenerationObjects.insert(*objectIt);
            objectIt = newGenerationObjects.erase(objectIt);
        }
    }
    
    // 清除未标记的新生代函数
    auto functionIt = newGenerationFunctions.begin();
    while (functionIt != newGenerationFunctions.end()) {
        if (markedFunctions.find(*functionIt) == markedFunctions.end()) {
            functions.erase(*functionIt);
            functionIt = newGenerationFunctions.erase(functionIt);
        } else {
            oldGenerationFunctions.insert(*functionIt);
            functionIt = newGenerationFunctions.erase(functionIt);
        }
    }
    
    // 清除未标记的新生代类
    auto classIt = newGenerationClasses.begin();
    while (classIt != newGenerationClasses.end()) {
        if (markedClasses.find(*classIt) == markedClasses.end()) {
            classes.erase(*classIt);
            classIt = newGenerationClasses.erase(classIt);
        } else {
            oldGenerationClasses.insert(*classIt);
            classIt = newGenerationClasses.erase(classIt);
        }
    }
    
    // 返回是否成功回收足够内存
    return newGenerationObjects.size() < NEW_GENERATION_THRESHOLD;
}

void GarbageCollector::collectOldGeneration(const std::shared_ptr<Environment>& roots) {
    // 标记所有对象
    markedObjects.clear();
    markedFunctions.clear();
    markedClasses.clear();
    
    markEnvironment(roots);
    
    // 清除所有未标记的对象
    sweep();
}

void GarbageCollector::mark(const Value& value) {
    // 标记对象
    if (std::holds_alternative<std::shared_ptr<ZeloObject>>(value)) {
        auto object = std::get<std::shared_ptr<ZeloObject>>(value);
        if (markedObjects.find(object) == markedObjects.end()) {
            markedObjects.insert(object);
            // 递归标记对象的所有字段
            for (const auto& [name, fieldValue] : object->fields) {
                mark(fieldValue);
            }
        }
    }
    // 标记函数
    else if (std::holds_alternative<std::shared_ptr<ZeloFunction>>(value)) {
        auto function = std::get<std::shared_ptr<ZeloFunction>>(value);
        if (markedFunctions.find(function) == markedFunctions.end()) {
            markedFunctions.insert(function);
            // 标记函数的闭包环境
            if (function->closure) {
                markEnvironment(function->closure);
            }
        }
    }
    // 标记类
    else if (std::holds_alternative<std::shared_ptr<ZeloClass>>(value)) {
        auto klass = std::get<std::shared_ptr<ZeloClass>>(value);
        if (markedClasses.find(klass) == markedClasses.end()) {
            markedClasses.insert(klass);
            // 标记类的所有方法
            for (const auto& [name, method] : klass->methods) {
                mark(method);
            }
            // 标记父类
            if (klass->superclass) {
                mark(klass->superclass);
            }
        }
    }
    // 标记数组元素
    else if (std::holds_alternative<std::shared_ptr<std::vector<Value>>>(value)) {
        auto array = std::get<std::shared_ptr<std::vector<Value>>>(value);
        for (const auto& element : *array) {
            mark(element);
        }
    }
    // 标记字典值
    else if (std::holds_alternative<std::shared_ptr<std::unordered_map<std::string, Value>>>(value)) {
        auto dict = std::get<std::shared_ptr<std::unordered_map<std::string, Value>>>(value);
        for (const auto& [key, val] : *dict) {
            mark(val);
        }
    }
}

void GarbageCollector::markObjectReferences(const std::shared_ptr<ZeloObject>& object) {
    if (markedObjects.find(object) == markedObjects.end()) {
        markedObjects.insert(object);
        // 递归标记对象的所有字段
        for (const auto& [name, fieldValue] : object->fields) {
            mark(fieldValue);
        }
    }
}

void GarbageCollector::markEnvironment(const std::shared_ptr<Environment>& env) {
    if (!env) return;
    
    // 标记环境中的所有值
    auto values = env->getValues();
    for (const auto& [name, value] : values) {
        mark(value);
    }
    
    // 递归标记父环境
    markEnvironment(env->getEnclosing());
}

void GarbageCollector::markIncremental(size_t currentStep, size_t totalSteps) {
    // 将标记过程分成多个步骤
    size_t totalObjects = objects.size();
    size_t objectsPerStep = totalObjects / totalSteps;
    size_t startIdx = currentStep * objectsPerStep;
    size_t endIdx = std::min(startIdx + objectsPerStep, totalObjects);
    
    auto it = objects.begin();
    std::advance(it, startIdx);
    
    for (size_t i = startIdx; i < endIdx && it != objects.end(); i++, ++it) {
        // 标记对象及其引用
        markObjectReferences(*it);
    }
}

void GarbageCollector::sweep() {
    // 清除未标记的对象
    auto objectIt = objects.begin();
    while (objectIt != objects.end()) {
        if (markedObjects.find(*objectIt) == markedObjects.end()) {
            objectIt = objects.erase(objectIt);
        } else {
            ++objectIt;
        }
    }
    
    // 清除未标记的函数
    auto functionIt = functions.begin();
    while (functionIt != functions.end()) {
        if (markedFunctions.find(*functionIt) == markedFunctions.end()) {
            functionIt = functions.erase(functionIt);
        } else {
            ++functionIt;
        }
    }
    
    // 清除未标记的类
    auto classIt = classes.begin();
    while (classIt != classes.end()) {
        if (markedClasses.find(*classIt) == markedClasses.end()) {
            classIt = classes.erase(classIt);
        } else {
            ++classIt;
        }
    }
    
    // 清空标记集合
    markedObjects.clear();
    markedFunctions.clear();
    markedClasses.clear();
}

} // namespace Zelo