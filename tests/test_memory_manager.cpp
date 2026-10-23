#include "memory_manager.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

// Checks remain active in Release builds (unlike assert).
#define CHECK(condition) do { if (!(condition)) throw std::runtime_error( \
    std::string(__func__) + ":" + std::to_string(__LINE__) + " " #condition); } while (false)

bool sameBlocks(const std::vector<MemoryBlock>& a, const std::vector<MemoryBlock>& b) {
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i)
        if (a[i].startAddressKb != b[i].startAddressKb ||
            a[i].allocatedSizeKb != b[i].allocatedSizeKb ||
            a[i].requestedSizeKb != b[i].requestedSizeKb ||
            a[i].processId != b[i].processId || a[i].isFree != b[i].isFree) return false;
    return true;
}
void unchanged(MemoryManager& m, const Event& event, const std::string& reason) {
    const auto before = m.getBlocks();
    const auto r = m.apply(event);
    CHECK(!r.success && r.message == reason);
    CHECK(r.allocatedSizeKb == 0 && !r.startAddressKb.has_value());
    CHECK(r.step == event.step && r.processId == event.processId);
    CHECK(r.operation == event.operation && r.requestedSizeKb == event.requestedSizeKb);
    CHECK(sameBlocks(before, r.blocks) && sameBlocks(before, m.getBlocks()));
    CHECK(m.validateState());
}
void rounding(Strategy strategy) {
    MemoryManager m(20, 4, strategy);
    const auto first = m.allocate(1, "P1", 5);
    CHECK(first.success && first.strategy == strategy);
    CHECK(first.startAddressKb == 0 && first.allocatedSizeKb == 8);
    CHECK(first.blocks[0].requestedSizeKb == 5);
    CHECK(first.blocks[1].startAddressKb == 8 && first.blocks[1].allocatedSizeKb == 12);
    CHECK(first.metrics.totalFreeKb == 12 && first.metrics.internalFragmentationKb == 3);
    CHECK(m.allocate(2, "P2", 9).allocatedSizeKb == 12);
    CHECK(m.getBlocks().size() == 2); // Exact fit leaves no zero-sized hole.
    const auto full = m.getMetrics();
    CHECK(full.internalFragmentationKb == 6 && full.totalFreeKb == 0);
    CHECK(full.holeCount == 0 && full.largestFreeBlockKb == 0);
    CHECK(full.externalFragmentationKb == 0 && full.externalFragmentationRatio == 0.0);
    unchanged(m, {3, OperationType::Allocate, "P1", 1}, "PID_EXISTS");
    unchanged(m, {4, OperationType::Allocate, "P3", 1}, "NO_TOTAL_MEMORY");
    const auto freed = m.deallocate(5, "P1");
    CHECK(freed.success && freed.allocatedSizeKb == 0 && !freed.startAddressKb);
    CHECK(freed.metrics.internalFragmentationKb == 3);
    CHECK(freed.blocks[0].processId.empty() && freed.blocks[0].requestedSizeKb == 0);
    CHECK(m.deallocate(6, "P2").metrics.internalFragmentationKb == 0);
    CHECK(m.getBlocks().size() == 1 && m.getBlocks()[0].allocatedSizeKb == 20);
    CHECK(first.blocks[0].processId == "P1" && !first.blocks[0].isFree);
    CHECK(m.allocate(7, "P1", 5).success); // Reuse after FREE.
    m.reset();
    CHECK(m.getBlocks().size() == 1 && m.getMetrics().totalFreeKb == 20);
    CHECK(m.getMetrics().internalFragmentationKb == 0 && m.validateState());
}
void oracle(Strategy strategy, int expectedStart, int largest, int visited) {
    MemoryManager m(200, 1, strategy);
    const int sizes[] = {20, 50, 20, 30, 20};
    for (int i = 0; i < 5; ++i) {
        CHECK(m.allocate(i + 1, "P" + std::to_string(i + 1), sizes[i]).success);
        CHECK(m.validateState());
    }
    CHECK(m.deallocate(6, "P2").success);
    CHECK(m.deallocate(7, "P4").success);
    const auto r = m.allocate(8, "P6", 25);
    CHECK(r.success && r.startAddressKb == expectedStart);
    CHECK(r.metrics.totalFreeKb == 115 && r.metrics.largestFreeBlockKb == largest);
    CHECK(r.metrics.holeCount == 3 && r.metrics.internalFragmentationKb == 0);
    CHECK(r.visitedBlocks == visited && m.validateState());
}
void tiesAndMerge(Strategy strategy) {
    MemoryManager m(100, 1, strategy);
    for (int i = 0; i < 5; ++i) CHECK(m.allocate(i + 1, std::to_string(i), 20).success);
    CHECK(m.deallocate(6, "0").success);
    CHECK(m.deallocate(7, "2").success);
    CHECK(m.allocate(8, "tie", 10).startAddressKb == 0);
    CHECK(m.deallocate(9, "tie").success); // Merge right.
    CHECK(m.deallocate(10, "1").success); // Merge both sides.
    CHECK(m.getBlocks()[0].allocatedSizeKb == 60);
    CHECK(m.deallocate(11, "3").success); // Merge left.
    CHECK(m.deallocate(12, "4").success);
    CHECK(m.getBlocks().size() == 1 && m.validateState());
}
void externalFragmentation(Strategy strategy) {
    MemoryManager m(100, 1, strategy);
    CHECK(m.allocate(1, "A", 20).success);
    CHECK(m.allocate(2, "B", 30).success);
    CHECK(m.allocate(3, "C", 20).success);
    CHECK(m.deallocate(4, "B").success);
    unchanged(m, {5, OperationType::Allocate, "D", 40}, "EXTERNAL_FRAGMENTATION");
    const auto metrics = m.getMetrics();
    CHECK(metrics.totalFreeKb == 60 && metrics.largestFreeBlockKb == 30);
    CHECK(metrics.externalFragmentationKb == 30 && metrics.holeCount == 2);
    CHECK(std::abs(metrics.externalFragmentationRatio - 0.5) < 1e-12);
}
void roundedHoleSelection(Strategy strategy) {
    MemoryManager m(30, 4, strategy);
    for (int i = 1; i <= 3; ++i)
        CHECK(m.allocate(i, "P" + std::to_string(i), 8).success);
    CHECK(m.deallocate(4, "P1").success);
    // Holes: 8 KB at 0 and 6 KB at 24. A 5 KB request needs an 8 KB hole.
    const auto r = m.allocate(5, "rounded", 5);
    CHECK(r.success && r.startAddressKb == 0 && r.allocatedSizeKb == 8);
    CHECK(r.metrics.totalFreeKb == 6 && r.metrics.internalFragmentationKb == 3);
    CHECK(m.validateState());
    MemoryManager independent(30, 4, strategy);
    CHECK(independent.getMetrics().totalFreeKb == 30);
    m.reset();
    CHECK(r.blocks[0].processId == "rounded");
}
void validation() {
    for (const auto c : {Config{0, 4, Strategy::FirstFit}, Config{10, 0, Strategy::FirstFit},
                         Config{10, 11, Strategy::FirstFit}, Config{10, 1, Strategy::All}}) {
        bool threw = false;
        try { MemoryManager m(c.totalMemoryKb, c.allocationUnitKb, c.strategy); }
        catch (const std::invalid_argument&) { threw = true; }
        CHECK(threw);
    }
    MemoryManager m(10, 4);
    CHECK(m.allocate(1, "P1", 5).allocatedSizeKb == 8);
    // A 2 KB tail cannot hold even a 1 KB request rounded to 4 KB.
    unchanged(m, {2, OperationType::Allocate, "P2", 1}, "NO_TOTAL_MEMORY");
    unchanged(m, {3, OperationType::Free, "missing", 0}, "PID_NOT_FOUND");
    unchanged(m, {4, OperationType::Free, "P1", 1}, "INVALID_FREE_SIZE");
    unchanged(m, {5, OperationType::Allocate, "", 1}, "INVALID_EVENT");
    unchanged(m, {6, OperationType::Allocate, "bad pid", 1}, "INVALID_EVENT");
    unchanged(m, {7, OperationType::Allocate, "P2", 0}, "INVALID_EVENT");
    unchanged(m, {8, OperationType::Allocate, "P2", -1}, "INVALID_EVENT");
    unchanged(m, {0, OperationType::Allocate, "P2", 1}, "INVALID_EVENT");
    unchanged(m, {9, static_cast<OperationType>(99), "P2", 1}, "INVALID_EVENT");
    const auto before = m.getBlocks();
    bool threw = false;
    try { m.reset(3); } catch (const std::invalid_argument&) { threw = true; }
    CHECK(threw && sameBlocks(before, m.getBlocks()));
    m.reset(24);
    CHECK(m.allocate(1, "new", 5).allocatedSizeKb == 8);
    MemoryManager big(std::numeric_limits<int>::max(), 4);
    unchanged(big, {1, OperationType::Allocate, "large", std::numeric_limits<int>::max()},
              "SIZE_OVERFLOW");
    MemoryManager exact(std::numeric_limits<int>::max(), 1);
    CHECK(exact.allocate(1, "large", std::numeric_limits<int>::max()).success);
    CHECK(exact.deallocate(2, "large").success && exact.validateState());
    MemoryManager caseSensitive(8, 1);
    CHECK(caseSensitive.allocate(1, "P1", 1).success);
    CHECK(caseSensitive.allocate(2, "p1", 1).success);
    MemoryManager legacy(20);
    CHECK(legacy.allocateFirstFit(1, 5));
    CHECK(legacy.getMemoryBlocks()[0].allocatedSizeKb == 8);
    CHECK(!legacy.allocateBestFit(1, 1) && !legacy.allocateWorstFit(-1, 1));
    CHECK(legacy.deallocate(1));
    CHECK(legacy.allocateBestFit(2, 5) && legacy.allocateWorstFit(3, 5));
    CHECK(legacy.validateState());
}
int main() {
    try {
        for (const auto strategy : {Strategy::FirstFit, Strategy::BestFit, Strategy::WorstFit}) {
            rounding(strategy);
            tiesAndMerge(strategy);
            externalFragmentation(strategy);
            roundedHoleSelection(strategy);
        }
        oracle(Strategy::FirstFit, 20, 60, 2);
        oracle(Strategy::BestFit, 90, 60, 6);
        oracle(Strategy::WorstFit, 140, 50, 6);
        validation();
        std::cout << "Core regression checks passed (rounding, metrics, strategies, failures, merge, reset).\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
