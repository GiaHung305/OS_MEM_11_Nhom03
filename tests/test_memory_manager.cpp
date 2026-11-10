#include "memory_manager.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <locale>
#include <random>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

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

namespace {

// Shared workloads for stress testing and timing.
constexpr int totalKb = 1536;
constexpr int unitKb = 4;
constexpr unsigned seed = 11003;
constexpr std::array eventCounts{100, 1000, 10000};
constexpr std::array strategies{Strategy::FirstFit, Strategy::BestFit, Strategy::WorstFit};
constexpr std::array strategyNames{"FIRST_FIT", "BEST_FIT", "WORST_FIT"};

enum class Workload { Normal, Churn, Fragmentation };
constexpr std::array workloads{Workload::Normal, Workload::Churn, Workload::Fragmentation};
constexpr std::array workloadNames{"normal", "churn", "fragmentation"};

std::vector<Event> generate(Workload workload, int count) {
    std::mt19937 rng(seed);
    std::vector<Event> events;
    events.reserve(count);
    int churnPid = 0;
    for (int i = 0; i < count; ++i) {
        const int position = i % 100;
        int pid = 0;
        int requestedKb = 0;
        switch (workload) {
        case Workload::Normal:
            pid = position % 50;
            if (position < 50) requestedKb = 8 + rng() % 29;
            break;
        case Workload::Churn:
            if (i < 24) {
                pid = i;
                requestedKb = 12 + rng() % 54;
            } else {
                if (i % 2 == 0) churnPid = rng() % 24;
                pid = churnPid;
                if (i % 2 != 0) requestedKb = 12 + rng() % 54;
            }
            break;
        case Workload::Fragmentation:
            // Fill memory with 24 pairs of 24 KB and 40 KB blocks.
            // Free the 24 KB blocks, try four 32 KB requests, then free the rest.
            if (position < 48) {
                pid = position;
                requestedKb = position % 2 == 0 ? 24 : 40;
            } else if (position < 72) {
                pid = 2 * (position - 48);
            } else if (position < 76) {
                pid = position;
                requestedKb = 32;
            } else {
                pid = 2 * (position - 76) + 1;
            }
            break;
        }
        const auto operation = requestedKb > 0 ? OperationType::Allocate : OperationType::Free;
        events.push_back({i + 1, operation, "P" + std::to_string(pid), requestedKb});
    }
    return events;
}

// Stress checks run after each event.
struct Allocation {
    int address;
    int requestedKb;
    int allocatedKb;
};

using Active = std::unordered_map<std::string, Allocation>;

void check(bool condition, const char* rule) {
    if (!condition) throw std::runtime_error(rule);
}

bool sameBlock(const MemoryBlock& a, const MemoryBlock& b) {
    return a.startAddressKb == b.startAddressKb && a.allocatedSizeKb == b.allocatedSizeKb &&
           a.requestedSizeKb == b.requestedSizeKb && a.processId == b.processId &&
           a.isFree == b.isFree;
}

void checkSnapshot(const StepResult& result, const Active& active) {
    int end = 0;
    int freeKb = 0;
    int largestKb = 0;
    int holes = 0;
    int internalKb = 0;
    std::size_t used = 0;
    bool previousFree = false;
    for (const auto& block : result.blocks) {
        check(block.startAddressKb == end && block.allocatedSizeKb > 0 &&
              block.allocatedSizeKb <= totalKb - end, "blocks must cover memory without gaps or overlaps");
        end += block.allocatedSizeKb;
        if (block.isFree) {
            check(!previousFree && block.processId.empty() && block.requestedSizeKb == 0,
                  "free blocks must be merged and carry no process data");
            freeKb += block.allocatedSizeKb;
            largestKb = std::max(largestKb, block.allocatedSizeKb);
            ++holes;
        } else {
            const auto allocation = active.find(block.processId);
            check(allocation != active.end(), "snapshot contains an unexpected PID");
            const auto& expected = allocation->second;
            check(block.startAddressKb == expected.address &&
                  block.requestedSizeKb == expected.requestedKb &&
                  block.allocatedSizeKb == expected.allocatedKb, "active allocation changed");
            internalKb += expected.allocatedKb - expected.requestedKb;
            ++used;
        }
        previousFree = block.isFree;
    }
    check(end == totalKb && used == active.size(), "snapshot must include all active allocations");
    const int externalKb = freeKb - largestKb;
    const double ratio = freeKb == 0 ? 0.0 : static_cast<double>(externalKb) / freeKb;
    const auto& metrics = result.metrics;
    check(metrics.totalFreeKb == freeKb && metrics.largestFreeBlockKb == largestKb &&
          metrics.holeCount == holes && metrics.internalFragmentationKb == internalKb &&
          metrics.externalFragmentationKb == externalKb &&
          std::abs(metrics.externalFragmentationRatio - ratio) < 1e-12, "incorrect fragmentation metrics");
}

void runStress(const std::vector<Event>& events, Strategy strategy, Workload workload) {
    MemoryManager manager(totalKb, unitKb, strategy);
    Active active;
    auto previousBlocks = manager.getBlocks();
    int largestKb = totalKb;
    int freeKb = totalKb;
    int fragmentationFailures = 0;
    for (const auto& event : events) {
        try {
            auto result = manager.apply(event);
            check(result.step == event.step && result.strategy == strategy &&
                  result.operation == event.operation && result.processId == event.processId &&
                  result.requestedSizeKb == event.requestedSizeKb && !result.message.empty(),
                  "result does not describe the input event");
            const bool allocate = event.operation == OperationType::Allocate;
            const bool exists = active.count(event.processId) != 0;
            const int rounded = unitKb * static_cast<int>(
                std::ceil(static_cast<double>(event.requestedSizeKb) / unitKb));
            const bool shouldSucceed = allocate ? !exists && rounded <= largestKb : exists;
            check(result.success == shouldSucceed, "unexpected success or failure");

            if (allocate && shouldSucceed) {
                check(result.startAddressKb.has_value() && result.allocatedSizeKb == rounded,
                      "successful allocation must report address and rounded size");
                const int address = *result.startAddressKb;
                check(std::any_of(previousBlocks.begin(), previousBlocks.end(), [&](const auto& block) {
                    return block.isFree && block.startAddressKb == address && block.allocatedSizeKb >= rounded;
                }), "allocation must start at a fitting free block");
                active.emplace(event.processId, Allocation{address, event.requestedSizeKb, rounded});
            } else {
                check(!result.startAddressKb && result.allocatedSizeKb == 0,
                      "FREE and rejected events must not report an allocation");
                if (shouldSucceed) active.erase(event.processId);
            }

            if (!shouldSucceed) {
                check(std::equal(previousBlocks.begin(), previousBlocks.end(),
                                 result.blocks.begin(), result.blocks.end(), sameBlock),
                      "rejected event changed memory");
                if (allocate && !exists && rounded <= freeKb) {
                    check(result.message == "EXTERNAL_FRAGMENTATION", "incorrect fragmentation failure");
                    ++fragmentationFailures;
                }
            }
            checkSnapshot(result, active);
            check(manager.validateState(), "Core state validation failed");
            check(std::equal(result.blocks.begin(), result.blocks.end(), manager.getMemoryBlocks().begin(),
                             manager.getMemoryBlocks().end(), sameBlock), "snapshot differs from Core state");
            largestKb = result.metrics.largestFreeBlockKb;
            freeKb = result.metrics.totalFreeKb;
            previousBlocks = std::move(result.blocks);
        } catch (const std::exception& error) {
            throw std::runtime_error("step " + std::to_string(event.step) + ": " + error.what());
        }
    }

    if (workload == Workload::Fragmentation) {
        check(fragmentationFailures == static_cast<int>(events.size()) / 100 * 4,
              "fragmentation workload must reject four allocations per cycle");
        check(active.empty() && previousBlocks.size() == 1, "fragmentation workload must fully coalesce");
    }
    manager.reset();
    const auto& resetBlocks = manager.getMemoryBlocks();
    check(resetBlocks.size() == 1 && resetBlocks.front().isFree &&
          resetBlocks.front().allocatedSizeKb == totalKb && manager.validateState(), "reset failed");
}

int runStressCases() {
    for (std::size_t workload = 0; workload < workloads.size(); ++workload) {
        for (const int count : eventCounts) {
            const auto events = generate(workloads[workload], count);
            for (std::size_t strategy = 0; strategy < strategies.size(); ++strategy) {
                try {
                    runStress(events, strategies[strategy], workloads[workload]);
                } catch (const std::exception& error) {
                    std::cerr << workloadNames[workload] << ", " << count << " events, "
                              << strategyNames[strategy] << ": " << error.what() << '\n';
                    return 1;
                }
            }
        }
    }
    std::cout << "Memory stress passed for all 27 workload/size/strategy combinations.\n";
    return 0;
}

// Benchmark timing excludes workload generation and stress checks.
constexpr std::size_t repetitions = 5;

struct Sample {
    double elapsedUs;
    int allocSuccesses;
    int allocFailures;
};

Sample measure(const std::vector<Event>& events, Strategy strategy) {
    MemoryManager manager(totalKb, unitKb, strategy);
    int successes = 0;
    int failures = 0;
    const auto start = std::chrono::steady_clock::now();
    for (const auto& event : events) {
        const auto result = manager.apply(event);
        if (event.operation == OperationType::Allocate) {
            successes += result.success;
            failures += !result.success;
        }
    }
    const auto stop = std::chrono::steady_clock::now();
    return {std::chrono::duration<double, std::micro>(stop - start).count(), successes, failures};
}

int runBenchmark() {
    std::cout.imbue(std::locale::classic());
    std::cout << "workload,event_count,total_kb,allocation_unit_kb,seed,strategy,repetition,"
                 "elapsed_us,min_us,median_us,max_us,alloc_successes,alloc_failures\n"
              << std::fixed << std::setprecision(3);
    for (std::size_t workload = 0; workload < workloads.size(); ++workload) {
        for (const int count : eventCounts) {
            const auto events = generate(workloads[workload], count);
            std::array<std::array<Sample, repetitions>, strategies.size()> samples{};
            for (const auto strategy : strategies) measure(events, strategy); // Warmup.
            for (std::size_t rep = 0; rep < repetitions; ++rep) {
                // Rotate the order so one strategy does not always run first.
                for (std::size_t i = 0; i < strategies.size(); ++i) {
                    const std::size_t strategy = (rep + i) % strategies.size();
                    samples[strategy][rep] = measure(events, strategies[strategy]);
                }
            }
            for (std::size_t strategy = 0; strategy < strategies.size(); ++strategy) {
                std::array<double, repetitions> times{};
                for (std::size_t i = 0; i < repetitions; ++i) times[i] = samples[strategy][i].elapsedUs;
                std::sort(times.begin(), times.end());
                for (std::size_t rep = 0; rep < repetitions; ++rep) {
                    const auto& sample = samples[strategy][rep];
                    std::cout << workloadNames[workload] << ',' << count << ',' << totalKb << ',' << unitKb << ','
                              << seed << ',' << strategyNames[strategy] << ',' << rep + 1 << ','
                              << sample.elapsedUs << ',' << times.front() << ',' << times[repetitions / 2] << ','
                              << times.back() << ',' << sample.allocSuccesses << ',' << sample.allocFailures << '\n';
                }
            }
        }
    }
    return std::cout ? 0 : 1;
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc == 2) {
        const std::string mode = argv[1];
        if (mode == "--stress") return runStressCases();
        if (mode == "--benchmark") return runBenchmark();
    }
    if (argc != 1) {
        std::cerr << "Usage: test_memory_manager [--stress | --benchmark]\n";
        return 2;
    }
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
