#pragma once

#include <string>
#include <vector>

// Chiến lược cấp phát bộ nhớ.
// All chỉ dùng ở file cấu hình/controller để yêu cầu chạy cả ba chiến lược.
// MemoryManager chỉ nhận FirstFit, BestFit hoặc WorstFit cho một lần mô phỏng.
enum class Strategy {
    FirstFit,
    BestFit,
    WorstFit,
    All
};

// Loại thao tác đọc từ events_*.csv.
enum class OperationType {
    Allocate,
    Free
};

// Dữ liệu một dòng trong config_sample.csv.
struct Config {
    int totalMemoryKb = 0;
    int allocationUnitKb = 0;
    Strategy strategy = Strategy::All;
};

// Dữ liệu một dòng trong events_*.csv.
// Với thao tác Free, requestedSizeKb phải bằng 0.
struct Event {
    int step = 0;
    OperationType operation = OperationType::Allocate;
    std::string processId;
    int requestedSizeKb = 0;
};

// Một vùng liên tục trên Memory Map.
// Vùng trống phải có isFree = true, processId rỗng và requestedSizeKb = 0.
struct MemoryBlock {
    int startAddressKb = 0;
    int allocatedSizeKb = 0;
    int requestedSizeKb = 0;
    std::string processId;
    bool isFree = true;
};

// Các chỉ số được tính sau mỗi thao tác.
struct MemoryMetrics {
    int totalFreeKb = 0;
    int largestFreeBlockKb = 0;
    int holeCount = 0;
    int internalFragmentationKb = 0;
    int externalFragmentationKb = 0;
    double externalFragmentationRatio = 0.0;
};

// Kết quả của một bước mô phỏng.
// MemoryManager tạo đối tượng này; controller lưu lại; GUI hiển thị;
// CsvHandler dùng nó để ghi trace.csv và blocks.csv.
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
};
