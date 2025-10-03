#pragma once

#include "Value.h"
#include <memory>
#include <unordered_set>
#include <vector>

namespace Zelo {

class GarbageCollector {
public:
    static GarbageCollector& getInstance();
    
    void registerObject(const std::shared_ptr<ZeloObject>& object);
    void registerFunction(const std::shared_ptr<ZeloFunction>& function);
    void registerClass(const std::shared_ptr<ZeloClass>& klass);
    
    void collectGarbage(const std::shared_ptr<Environment>& roots);
    void collectGarbageIncremental(const std::shared_ptr<Environment>& roots);
    
    size_t getObjectCount() const { return objects.size(); }
    size_t getFunctionCount() const { return functions.size(); }
    size_t getClassCount() const { return classes.size(); }
    
    size_t getNewGenerationObjectCount() const { return newGenerationObjects.size(); }
    size_t getOldGenerationObjectCount() const { return oldGenerationObjects.size(); }
    
private:
    GarbageCollector();
    
    void mark(const Value& value);
    void markObjectReferences(const std::shared_ptr<ZeloObject>& object);
    void markEnvironment(const std::shared_ptr<Environment>& env);
    void markIncremental(size_t currentStep, size_t totalSteps);
    bool collectNewGeneration(const std::shared_ptr<Environment>& roots);
    void collectOldGeneration(const std::shared_ptr<Environment>& roots);
    void sweep();
    
    std::unordered_set<std::shared_ptr<ZeloObject>> objects;
    std::unordered_set<std::shared_ptr<ZeloFunction>> functions;
    std::unordered_set<std::shared_ptr<ZeloClass>> classes;
    
    std::unordered_set<std::shared_ptr<ZeloObject>> newGenerationObjects;
    std::unordered_set<std::shared_ptr<ZeloObject>> oldGenerationObjects;
    std::unordered_set<std::shared_ptr<ZeloFunction>> newGenerationFunctions;
    std::unordered_set<std::shared_ptr<ZeloFunction>> oldGenerationFunctions;
    std::unordered_set<std::shared_ptr<ZeloClass>> newGenerationClasses;
    std::unordered_set<std::shared_ptr<ZeloClass>> oldGenerationClasses;
    
    std::unordered_set<std::shared_ptr<ZeloObject>> markedObjects;
    std::unordered_set<std::shared_ptr<ZeloFunction>> markedFunctions;
    std::unordered_set<std::shared_ptr<ZeloClass>> markedClasses;
    
    static const size_t NEW_GENERATION_THRESHOLD = 1000;
    size_t incrementalStep = 0;
};

} // namespace Zelo