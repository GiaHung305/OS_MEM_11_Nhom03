#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "models.h"

// Core (khai bao trong memory_manager.h). Controller chi giu con tro, khong
// can biet chi tiet thuat toan.
class MemoryManager;

// Controller: lop noi giua GUI va Core.
//  - Nhan Config + danh sach Event da validate tu CSV parser.
//  - Neu config.strategy == All: tao 3 run doc lap theo thu tu
//    FirstFit, BestFit, WorstFit. Nguoc lai: tao dung 1 run.
//  - Cac run chay "dong bo": moi lan xu ly cung mot Event tren tat ca run,
//    nen results.size() cua moi run luon bang nhau.
//  - Khong dung Qt. GUI goi ham roi doc lai runs() de ve.
//  - Khong tinh lai metrics/summary theo thuat toan; metrics lay tu Core,
//    summary chi dem tu StepResult.
class SimulationController {
public:
    SimulationController();
    ~SimulationController();

    // Controller giu unique_ptr<MemoryManager> nen khong copy/move.
    SimulationController(const SimulationController&) = delete;
    SimulationController& operator=(const SimulationController&) = delete;
    SimulationController(SimulationController&&) = delete;
    SimulationController& operator=(SimulationController&&) = delete;

    // ---- Nap kich ban ----

    // Kiem tra config + events roi tao run moi o step 0.
    // That bai: tra false, kich ban/trace cu giu nguyen, lastError() co ly do.
    bool loadScenario(const Config& config, const std::vector<Event>& events);

    // Dua moi run ve state ban dau (xoa history), giu nguyen kich ban.
    // Khong lam gi neu chua nap kich ban.
    void reset();

    // ---- Dieu khien chay ----

    // Xu ly event ke tiep tren tat ca run. Tra false neu chua nap kich ban
    // hoac da het event.
    bool stepForward();

    // Chay toi het event. Tra ve so event vua xu ly them.
    std::size_t runToEnd();

    // Dua ve trang thai "da xu ly processedCount event" (0 = state ban dau).
    // processedCount lon hon so event thi bi cat ve so event.
    // Neu lui ve truoc thi reset roi chay lai (Core la deterministic).
    void seek(std::size_t processedCount);

    // ---- Truy van (GUI chi doc) ----

    bool hasScenario() const;
    bool isFinished() const;                 // da xu ly het event
    std::size_t processedCount() const;      // so event da xu ly
    std::size_t totalEvents() const;

    // Config da import, giu nguyen strategy (co the la All).
    const Config& config() const;
    const std::vector<Event>& events() const;

    // 1 hoac 3 run theo thu tu FirstFit, BestFit, WorstFit.
    const std::vector<RunResult>& runs() const;

    // Run cua mot chien luoc cu the. nullptr neu chua co run do hoac
    // strategy == All (All khong phai mot run).
    const RunResult* runFor(Strategy strategy) const;

    // Snapshot doc lap sau khi da xu ly processedCount event cua run do.
    // 0 -> initialState (step 0). k > 0 -> state sau results[k - 1].
    // nullopt neu khong co run hoac processedCount chua duoc xu ly toi.
    std::optional<MemorySnapshot> snapshotAfter(Strategy strategy,
                                                std::size_t processedCount) const;

    // Ly do that bai gan nhat cua loadScenario; rong neu lan goi do thanh cong.
    const std::string& lastError() const;

private:
    // Tao cac run o state ban dau tu (config, events). Chi khi thanh cong moi
    // thay runs_/managers_/processed_; that bai (Core nem exception khi
    // tao) thi tra false, ghi lastError_ va khong dong vao state hien tai.
    // Khong doc/ghi config_ va events_ nen goi an toan tu reset().
    bool buildRuns(const Config& config, const std::vector<Event>& events);

    Config config_;
    std::vector<Event> events_;
    std::vector<RunResult> runs_;
    std::vector<std::unique_ptr<MemoryManager>> managers_;  // managers_[i] <-> runs_[i]
    std::size_t processed_ = 0;
    std::string lastError_;
};
