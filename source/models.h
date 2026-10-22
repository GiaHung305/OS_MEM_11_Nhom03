#pragma once

#include <optional>
#include <string>
#include <vector>

// Hop dong du lieu chung cho Core, Controller, CSV, GUI va Tests.
// File nay chi khai bao du lieu; khong chua thuat toan, Qt hay doc/ghi file.
// Dung int cho dung luong/dia chi KB; parser phai tu choi gia tri hoac phep
// lam tron vuot qua INT_MAX. Dia chi la khoang nua mo [start, start + size).

// All chi dung de yeu cau Controller chay ba chien luoc doc lap.
// Mot MemoryManager/StepResult/RunResult chi co mot chien luoc cu the.
enum class Strategy {
    FirstFit,
    BestFit,
    WorstFit,
    All
};

// Gia tri cot "operation": Allocate <-> ALLOC, Free <-> FREE.
enum class OperationType {
    Allocate,
    Free
};

// totalMemoryKb > 0; 0 < allocationUnitKb <= totalMemoryKb.
// Header config: total_memory_kb,allocation_unit_kb,strategy.
// strategy: FIRST_FIT, BEST_FIT, WORST_FIT hoac ALL.
// Gia tri mac dinh chi de khoi tao object, KHONG phai config hop le.
struct Config {
    int totalMemoryKb = 0;
    int allocationUnitKb = 0;
    Strategy strategy = Strategy::All;
};

// Header events: step,operation,pid,size_kb.
// Moi dong tao mot Event. step > 0, duy nhat va tang dan theo thu tu file.
// processId phan biet chu hoa/thuong; parser cat khoang trang dau/cuoi.
// PID chi gom chu, so, '_' va '-'.
// Allocate: requestedSizeKb > 0. Free: requestedSizeKb == 0.
// Parser phai validate truoc khi Controller nhan ca danh sach.
struct Event {
    int step = 0;
    OperationType operation = OperationType::Allocate;
    std::string processId;
    int requestedSizeKb = 0;
};

// startAddressKb >= 0, allocatedSizeKb > 0 trong mot state hop le.
// Vung trong: processId rong, requestedSizeKb == 0.
// Vung da cap: processId khong rong, 0 < requestedSizeKb <= allocatedSizeKb.
// end-exclusive = startAddressKb + allocatedSizeKb; khong luu trung dia chi cuoi.
// Trong mot state, blocks sap theo start, phu kin [0, totalMemoryKb), khong
// chong/gap, khong co hai hole lien ke, PID dang dung khong trung nhau.
struct MemoryBlock {
    int startAddressKb = 0;
    int allocatedSizeKb = 0;
    int requestedSizeKb = 0;
    std::string processId;
    bool isFree = true;
};

// Metrics cua mot snapshot, do Core/metrics tinh; GUI va CSV chi hien thi.
// externalFragmentationKb = totalFreeKb - largestFreeBlockKb.
// ratio = double(externalFragmentationKb) / totalFreeKb, thuoc [0, 1];
// bang 0 khi totalFreeKb == 0. Tinh bang so thuc, khong chia so nguyen.
struct MemoryMetrics {
    int totalFreeKb = 0;
    int largestFreeBlockKb = 0;
    int holeCount = 0;
    int internalFragmentationKb = 0;
    int externalFragmentationKb = 0;
    double externalFragmentationRatio = 0.0;
};

// Mot Event da duoc xu ly luon tao dung mot StepResult, ke ca khi that bai.
// blocks va metrics la snapshot SAU event; that bai thi state khong doi.
// step/operation/processId/requestedSizeKb phai copy tu Event tuong ung.
// FREE va ALLOC that bai: allocatedSizeKb = 0; ALLOC thanh cong: bang size
// da lam tron. message la noi dung trace/loi theo spec, khong de trong khi
// that bai. step 0 khong phai Event va dung MemorySnapshot rieng.
struct StepResult {
    Strategy strategy = Strategy::FirstFit;
    int step = 0;
    OperationType operation = OperationType::Allocate;
    std::string processId;
    int requestedSizeKb = 0;
    int allocatedSizeKb = 0;
    bool success = false;
    std::string message;
    MemoryMetrics metrics;
    std::vector<MemoryBlock> blocks;

    // Bo sung cho trace va oracle: khong dung dia chi 0 lam ky hieu that bai.
    // FREE va ALLOC that bai: startAddressKb = nullopt.
    // visitedBlocks la chi so bo sung: dem moi block da duyet khi tim hole,
    // ke ca block dang dung. PID trung bi chan truoc tim hole -> 0; FREE -> 0.
    std::optional<int> startAddressKb;
    int visitedBlocks = 0;
};

// State ban dau (step 0) hoac state luu de chon lai tren trace.
// blocks la ban sao doc lap; khong tro vao vector mutable cua Core.
// State ban dau: mot hole [0, totalMemoryKb), metrics tuong ung.
struct MemorySnapshot {
    int step = 0;
    MemoryMetrics metrics;
    std::vector<MemoryBlock> blocks;
};

// Thong ke cua mot lan chay; khong tinh lai trong GUI.
// allocAttempts = allocSuccesses + allocFailures.
struct RunSummary {
    int allocAttempts = 0;
    int allocSuccesses = 0;
    int allocFailures = 0;
    int freeSuccesses = 0;
    int freeFailures = 0;
    MemoryMetrics finalMetrics;
};

// Mot chien luoc co state va history rieng. Khi dang chay,
// results.size() <= events.size(); results[i] ung voi events[i].
// summary thong ke phan da xu ly; finalMetrics la metrics cua snapshot moi
// nhat (initialState neu results rong). Reset phai tao lai state ban dau.
// Compare All la ba RunResult doc lap voi cung config/events, theo thu tu
// FirstFit, BestFit, WorstFit; Strategy::All khong duoc gan vao RunResult.
// config.strategy giu gia tri da import; strategy la chien luoc thuc te cua
// run nay. Khong ngam thay config.strategy khi GUI chon chien luoc.
struct RunResult {
    Config config;
    Strategy strategy = Strategy::FirstFit;
    MemorySnapshot initialState;
    std::vector<Event> events;
    std::vector<StepResult> results;
    RunSummary summary;
};

// Loi validation CSV cho GUI hien file, dong, cot va ly do.
// line/column bat dau tu 1; bang 0 neu loi khong gan voi dong/cot cu the.
struct CsvError {
    std::string filePath;
    int line = 0;
    int column = 0;
    std::string message;
};

// Ket qua parse mot file. Thanh cong: value co gia tri, error rong.
// That bai: value rong, error co gia tri; khong tra du lieu nap mot phan.
// Hai optional phai loai tru nhau; caller chi doc value khi co value va
// khong co error. vector<Event> rong van co the la parse thanh cong khi
// parser/nhom quyet dinh chap nhan file chi co header.
template <typename T>
struct CsvParseResult {
    std::optional<T> value;
    std::optional<CsvError> error;
};
