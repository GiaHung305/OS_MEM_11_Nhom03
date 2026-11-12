#include "simulation_controller.h"

#include <cassert>
#include <exception>
#include <utility>

#include "memory_manager.h"

namespace {

// All -> ba chien luoc doc lap theo thu tu co dinh; con lai -> chinh no.
std::vector<Strategy> expandStrategy(Strategy strategy) {
    if (strategy == Strategy::All) {
        return {Strategy::FirstFit, Strategy::BestFit, Strategy::WorstFit};
    }
    return {strategy};
}

// Tra chuoi rong neu hop le, nguoc lai tra ly do. Parser da validate truoc,
// day chi la lop phong ve re tien de Core khong nhan du lieu vo nghia.
std::string validateScenario(const Config& config, const std::vector<Event>& events) {
    if (config.totalMemoryKb <= 0) {
        return "total_memory_kb phai > 0";
    }
    if (config.allocationUnitKb <= 0 ||
        config.allocationUnitKb > config.totalMemoryKb) {
        return "allocation_unit_kb phai thoa 0 < unit <= total_memory_kb";
    }

    int previousStep = 0;
    for (std::size_t i = 0; i < events.size(); ++i) {
        const Event& e = events[i];
        const std::string where = "event thu " + std::to_string(i + 1) + ": ";

        if (e.step <= 0) {
            return where + "step phai > 0";
        }
        if (e.step <= previousStep) {
            return where + "step phai duy nhat va tang dan";
        }
        previousStep = e.step;

        if (e.operation == OperationType::Allocate && e.requestedSizeKb <= 0) {
            return where + "ALLOC can size_kb > 0";
        }
        if (e.operation == OperationType::Free && e.requestedSizeKb != 0) {
            return where + "FREE phai co size_kb == 0";
        }
    }
    return {};
}

// Cap nhat thong ke theo dung mot StepResult vua tao.
void accumulate(RunSummary& summary, const StepResult& r) {
    if (r.operation == OperationType::Allocate) {
        ++summary.allocAttempts;
        if (r.success) {
            ++summary.allocSuccesses;
        } else {
            ++summary.allocFailures;
        }
    } else {
        if (r.success) {
            ++summary.freeSuccesses;
        } else {
            ++summary.freeFailures;
        }
    }
    // finalMetrics luon la metrics cua snapshot moi nhat.
    summary.finalMetrics = r.metrics;
}

}  // namespace

// Dinh nghia o day vi MemoryManager chi duoc khai bao truoc trong header.
SimulationController::SimulationController() = default;
SimulationController::~SimulationController() = default;

bool SimulationController::loadScenario(const Config& config,
                                        const std::vector<Event>& events) {
    const std::string error = validateScenario(config, events);
    if (!error.empty()) {
        lastError_ = error;  // state cu khong bi dong vao
        return false;
    }

    if (!buildRuns(config, events)) {
        return false;  // lastError_ da duoc ghi, state cu khong bi dong vao
    }
    config_ = config;
    events_ = events;
    lastError_.clear();
    return true;
}

void SimulationController::reset() {
    if (hasScenario()) {
        buildRuns(config_, events_);
    }
}

bool SimulationController::stepForward() {
    if (!hasScenario() || isFinished()) {
        return false;
    }

    // MemoryManager::apply khong nem exception: event loi (PID trung,
    // khong tim thay PID, het cho...) van tra StepResult voi success=false.
    const Event& event = events_[processed_];
    for (std::size_t i = 0; i < runs_.size(); ++i) {
        StepResult result = managers_[i]->apply(event);
        assert(result.step == event.step);
        assert(result.strategy == runs_[i].strategy);

        accumulate(runs_[i].summary, result);
        runs_[i].results.push_back(std::move(result));
    }
    ++processed_;
    return true;
}

std::size_t SimulationController::runToEnd() {
    std::size_t executed = 0;
    while (stepForward()) {
        ++executed;
    }
    return executed;
}

void SimulationController::seek(std::size_t processedCount) {
    if (!hasScenario()) {
        return;
    }
    if (processedCount > events_.size()) {
        processedCount = events_.size();
    }
    if (processedCount < processed_) {
        reset();
    }
    while (processed_ < processedCount && stepForward()) {
    }
}

bool SimulationController::hasScenario() const {
    return !runs_.empty();
}

bool SimulationController::isFinished() const {
    return hasScenario() && processed_ >= events_.size();
}

std::size_t SimulationController::processedCount() const {
    return processed_;
}

std::size_t SimulationController::totalEvents() const {
    return events_.size();
}

const Config& SimulationController::config() const {
    return config_;
}

const std::vector<Event>& SimulationController::events() const {
    return events_;
}

const std::vector<RunResult>& SimulationController::runs() const {
    return runs_;
}

const RunResult* SimulationController::runFor(Strategy strategy) const {
    for (const RunResult& run : runs_) {
        if (run.strategy == strategy) {
            return &run;
        }
    }
    return nullptr;
}

std::optional<MemorySnapshot> SimulationController::snapshotAfter(
    Strategy strategy, std::size_t processedCount) const {
    const RunResult* run = runFor(strategy);
    if (run == nullptr || processedCount > run->results.size()) {
        return std::nullopt;
    }
    if (processedCount == 0) {
        return run->initialState;  // step 0, ban sao doc lap
    }

    const StepResult& r = run->results[processedCount - 1];
    MemorySnapshot snapshot;
    snapshot.step = r.step;
    snapshot.metrics = r.metrics;
    snapshot.blocks = r.blocks;
    return snapshot;
}

const std::string& SimulationController::lastError() const {
    return lastError_;
}

bool SimulationController::buildRuns(const Config& config,
                                     const std::vector<Event>& events) {
    std::vector<RunResult> runs;
    std::vector<std::unique_ptr<MemoryManager>> managers;

    try {
        for (Strategy strategy : expandStrategy(config.strategy)) {
            // Moi manager chi dung mot chien luoc cu the (khong phai All).
            auto manager = std::make_unique<MemoryManager>(
                config.totalMemoryKb, config.allocationUnitKb, strategy);

            RunResult run;
            run.config = config;       // giu nguyen strategy da import (co the la All)
            run.strategy = strategy;   // chien luoc thuc te cua run nay
            run.events = events;
            run.results.reserve(events.size());

            // State ban dau (step 0): MemorySnapshot::step mac dinh la 0.
            run.initialState.metrics = manager->getMetrics();
            run.initialState.blocks = manager->getBlocks();
            run.summary.finalMetrics = run.initialState.metrics;

            runs.push_back(std::move(run));
            managers.push_back(std::move(manager));
        }
    } catch (const std::exception& ex) {
        lastError_ = ex.what();
        return false;
    }

    runs_ = std::move(runs);
    managers_ = std::move(managers);
    processed_ = 0;
    return true;
}
