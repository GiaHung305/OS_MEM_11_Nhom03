#pragma once

#include "models.h"

class MemoryManager {
public:
    // All belongs to the controller; one manager uses one concrete strategy.
    explicit MemoryManager(int totalMemoryKb, int allocationUnitKb = 4,
                           Strategy strategy = Strategy::FirstFit);
    StepResult apply(const Event& event);
    StepResult allocate(int step, const std::string& processId, int requestedSizeKb);
    StepResult deallocate(int step, const std::string& processId);
    void reset();
    // Retains unit/strategy; invalid sizes leave state intact.
    void reset(int totalMemoryKb);
    std::vector<MemoryBlock> getBlocks() const;
    MemoryMetrics getMetrics() const;
    bool validateState() const;

    // Compatibility entry points for numeric-PID callers. Default unit is now 4 KB.
    bool allocateFirstFit(int processId, int size);
    bool allocateBestFit(int processId, int size);
    bool allocateWorstFit(int processId, int size);
    bool deallocate(int processId);
    void coalesce();
    void displayMemory() const;
    // Uses shared model field names, replacing the old Block type.
    const std::vector<MemoryBlock>& getMemoryBlocks() const;

private:
    int totalMemoryKb_;
    int allocationUnitKb_;
    Strategy strategy_;
    std::vector<MemoryBlock> memoryBlocks;
    StepResult applyWithStrategy(const Event& event, Strategy strategy);
    bool allocateNumeric(int processId, int size, Strategy strategy);
};
