#include "memory_manager.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace {
bool validPid(const std::string& pid) {
    return !pid.empty() && std::all_of(pid.begin(), pid.end(), [](unsigned char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
               (c >= '0' && c <= '9') || c == '_' || c == '-';
    });
}
void validateConfig(int total, int unit, Strategy strategy) {
    if (total <= 0 || unit <= 0 || unit > total)
        throw std::invalid_argument("Require total > 0 and 0 < allocation unit <= total.");
    if (strategy != Strategy::FirstFit && strategy != Strategy::BestFit &&
        strategy != Strategy::WorstFit)
        throw std::invalid_argument("MemoryManager requires one concrete strategy.");
}
} // namespace

MemoryManager::MemoryManager(int total, int unit, Strategy strategy)
    : totalMemoryKb_(total), allocationUnitKb_(unit), strategy_(strategy) {
    validateConfig(total, unit, strategy);
    reset();
}

StepResult MemoryManager::apply(const Event& event) {
    return applyWithStrategy(event, strategy_);
}
StepResult MemoryManager::allocate(int step, const std::string& pid, int requested) {
    return apply({step, OperationType::Allocate, pid, requested});
}
StepResult MemoryManager::deallocate(int step, const std::string& pid) {
    return apply({step, OperationType::Free, pid, 0});
}

StepResult MemoryManager::applyWithStrategy(const Event& event, Strategy strategy) {
    StepResult result;
    result.strategy = strategy;
    result.step = event.step;
    result.operation = event.operation;
    result.processId = event.processId;
    result.requestedSizeKb = event.requestedSizeKb;
    auto finish = [&](const char* message) {
        assert(validateState());
        result.message = message;
        result.metrics = getMetrics();
        result.blocks = memoryBlocks; // Independent snapshot, even on failure.
        return result;
    };
    if (event.step <= 0 || !validPid(event.processId)) return finish("INVALID_EVENT");
    auto active = std::find_if(memoryBlocks.begin(), memoryBlocks.end(),
        [&](const MemoryBlock& block) {
            return !block.isFree && block.processId == event.processId;
        });
    if (event.operation == OperationType::Free) {
        if (event.requestedSizeKb != 0) return finish("INVALID_FREE_SIZE");
        if (active == memoryBlocks.end()) return finish("PID_NOT_FOUND");
        active->isFree = true;
        active->processId.clear();
        active->requestedSizeKb = 0;
        coalesce();
        result.success = true;
        return finish("FREED");
    }
    if (event.operation != OperationType::Allocate || event.requestedSizeKb <= 0)
        return finish("INVALID_EVENT");
    if (active != memoryBlocks.end()) return finish("PID_EXISTS");

    // Widen before arithmetic: a valid int request can round beyond INT_MAX.
    const std::int64_t rounded =
        ((std::int64_t(event.requestedSizeKb) + allocationUnitKb_ - 1) /
         allocationUnitKb_) * allocationUnitKb_;
    if (rounded > std::numeric_limits<int>::max()) return finish("SIZE_OVERFLOW");
    const int size = static_cast<int>(rounded);
    std::size_t chosen = memoryBlocks.size();
    for (std::size_t i = 0; i < memoryBlocks.size(); ++i) {
        ++result.visitedBlocks;
        const auto& block = memoryBlocks[i];
        if (!block.isFree || block.allocatedSizeKb < size) continue;
        if (chosen == memoryBlocks.size() ||
            (strategy == Strategy::BestFit &&
             block.allocatedSizeKb < memoryBlocks[chosen].allocatedSizeKb) ||
            (strategy == Strategy::WorstFit &&
             block.allocatedSizeKb > memoryBlocks[chosen].allocatedSizeKb)) chosen = i;
        if (strategy == Strategy::FirstFit) break;
        // Strict comparisons retain the lowest address on equal-sized holes.
    }
    if (chosen == memoryBlocks.size())
        return finish(getMetrics().totalFreeKb < size ?
                      "NO_TOTAL_MEMORY" : "EXTERNAL_FRAGMENTATION");

    const auto hole = memoryBlocks[chosen];
    MemoryBlock allocated{hole.startAddressKb, size, event.requestedSizeKb,
                          event.processId, false};
    if (hole.allocatedSizeKb > size) {
        MemoryBlock remainder{hole.startAddressKb + size,
                              hole.allocatedSizeKb - size, 0, "", true};
        memoryBlocks.insert(memoryBlocks.begin() + chosen + 1, remainder);
    }
    memoryBlocks[chosen] = std::move(allocated);
    result.success = true;
    result.allocatedSizeKb = size;
    result.startAddressKb = hole.startAddressKb;
    return finish("ALLOCATED");
}

MemoryMetrics MemoryManager::getMetrics() const {
    MemoryMetrics metrics;
    for (const auto& block : memoryBlocks) {
        if (block.isFree) {
            metrics.totalFreeKb += block.allocatedSizeKb;
            metrics.largestFreeBlockKb =
                std::max(metrics.largestFreeBlockKb, block.allocatedSizeKb);
            ++metrics.holeCount;
        } else {
            metrics.internalFragmentationKb += block.allocatedSizeKb - block.requestedSizeKb;
        }
    }
    metrics.externalFragmentationKb = metrics.totalFreeKb - metrics.largestFreeBlockKb;
    if (metrics.totalFreeKb != 0)
        metrics.externalFragmentationRatio =
            double(metrics.externalFragmentationKb) / metrics.totalFreeKb;
    return metrics;
}

void MemoryManager::coalesce() {
    std::size_t i = 0;
    while (i + 1 < memoryBlocks.size()) {
        auto& left = memoryBlocks[i];
        const auto& right = memoryBlocks[i + 1];
        if (left.isFree && right.isFree &&
            left.startAddressKb + left.allocatedSizeKb == right.startAddressKb) {
            left.allocatedSizeKb += right.allocatedSizeKb;
            memoryBlocks.erase(memoryBlocks.begin() + i + 1);
        } else ++i;
    }
}

bool MemoryManager::validateState() const {
    std::int64_t end = 0;
    bool previousFree = false;
    std::unordered_set<std::string> pids;
    for (const auto& block : memoryBlocks) {
        if (block.startAddressKb != end || block.allocatedSizeKb <= 0) return false;
        end += block.allocatedSizeKb;
        if (end > totalMemoryKb_) return false;
        if (block.isFree) {
            if (previousFree || !block.processId.empty() || block.requestedSizeKb != 0)
                return false;
        } else {
            if (!validPid(block.processId) || block.requestedSizeKb <= 0 ||
                block.requestedSizeKb > block.allocatedSizeKb ||
                block.allocatedSizeKb % allocationUnitKb_ != 0 ||
                !pids.insert(block.processId).second) return false;
        }
        previousFree = block.isFree;
    }
    return !memoryBlocks.empty() && end == totalMemoryKb_;
}

void MemoryManager::reset() { reset(totalMemoryKb_); }
void MemoryManager::reset(int total) {
    validateConfig(total, allocationUnitKb_, strategy_);
    std::vector<MemoryBlock> initial{{0, total, 0, "", true}};
    memoryBlocks.swap(initial);
    totalMemoryKb_ = total;
    assert(validateState());
}
std::vector<MemoryBlock> MemoryManager::getBlocks() const { return memoryBlocks; }
const std::vector<MemoryBlock>& MemoryManager::getMemoryBlocks() const {
    return memoryBlocks;
}

bool MemoryManager::allocateNumeric(int pid, int size, Strategy strategy) {
    if (pid < 0) return false;
    return applyWithStrategy({1, OperationType::Allocate, std::to_string(pid), size},
                             strategy).success;
}
bool MemoryManager::allocateFirstFit(int pid, int size) {
    return allocateNumeric(pid, size, Strategy::FirstFit);
}
bool MemoryManager::allocateBestFit(int pid, int size) {
    return allocateNumeric(pid, size, Strategy::BestFit);
}
bool MemoryManager::allocateWorstFit(int pid, int size) {
    return allocateNumeric(pid, size, Strategy::WorstFit);
}
bool MemoryManager::deallocate(int pid) {
    return pid >= 0 && deallocate(1, std::to_string(pid)).success;
}
void MemoryManager::displayMemory() const {
    for (const auto& block : memoryBlocks)
        std::cout << '[' << block.startAddressKb << ", "
                  << block.startAddressKb + block.allocatedSizeKb << ") "
                  << (block.isFree ? "FREE" : block.processId)
                  << " requested=" << block.requestedSizeKb
                  << " allocated=" << block.allocatedSizeKb << " KB\n";
}
