#include "memory_manager.h"

#include <iostream>
#include <stdexcept>
#include <string>

// Constructor khởi tạo bộ nhớ ban đầu gồm một khối lớn duy nhất hoàn toàn trống
MemoryManager::MemoryManager(int totalSize) {
    if (totalSize <= 0) {
        throw std::invalid_argument("Total memory size must be positive.");
    }
    memoryBlocks.emplace_back(1, 0, totalSize, true, -1);
}

bool MemoryManager::isValidAllocation(int processId, int size) const {
    if (processId < 0 || size <= 0) {
        return false;
    }

    for (const Block& block : memoryBlocks) {
        if (!block.isFree && block.processId == processId) {
            return false;
        }
    }
    return true;
}

bool MemoryManager::allocateAt(int processId, int size, std::size_t blockIndex) {
    const int remainingSize = memoryBlocks[blockIndex].size - size;
    const int freeBlockStart = memoryBlocks[blockIndex].startAddress + size;

    if (remainingSize > 0) {
        memoryBlocks.emplace(
            memoryBlocks.begin() + blockIndex + 1,
            -1,
            freeBlockStart,
            remainingSize,
            true,
            -1
        );
    }

    Block& allocatedBlock = memoryBlocks[blockIndex];
    allocatedBlock.isFree = false;
    allocatedBlock.processId = processId;
    allocatedBlock.size = size;
    return true;
}

// 1. Thuật toán First-Fit: Tìm khối trống đầu tiên đủ điều kiện
bool MemoryManager::allocateFirstFit(int processId, int size) {
    if (!isValidAllocation(processId, size)) {
        return false;
    }

    for (std::size_t i = 0; i < memoryBlocks.size(); ++i) {
        if (memoryBlocks[i].isFree && memoryBlocks[i].size >= size) {
            return allocateAt(processId, size, i);
        }
    }
    return false; // Không tìm đủ chỗ cấp phát
}

// 2. Thuật toán Best-Fit: Tìm khối trống có kích thước phù hợp nhất (dư thừa ít nhất)
bool MemoryManager::allocateBestFit(int processId, int size) {
    if (!isValidAllocation(processId, size)) {
        return false;
    }

    std::size_t bestIndex = memoryBlocks.size();

    for (std::size_t i = 0; i < memoryBlocks.size(); ++i) {
        if (memoryBlocks[i].isFree && memoryBlocks[i].size >= size) {
            if (bestIndex == memoryBlocks.size() ||
                memoryBlocks[i].size < memoryBlocks[bestIndex].size) {
                bestIndex = i;
            }
        }
    }

    return bestIndex != memoryBlocks.size() &&
           allocateAt(processId, size, bestIndex);
}

// 3. Thuật toán Worst-Fit: Tìm khối trống lớn nhất để cấp phát
bool MemoryManager::allocateWorstFit(int processId, int size) {
    if (!isValidAllocation(processId, size)) {
        return false;
    }

    std::size_t worstIndex = memoryBlocks.size();

    for (std::size_t i = 0; i < memoryBlocks.size(); ++i) {
        if (memoryBlocks[i].isFree && memoryBlocks[i].size >= size) {
            if (worstIndex == memoryBlocks.size() ||
                memoryBlocks[i].size > memoryBlocks[worstIndex].size) {
                worstIndex = i;
            }
        }
    }

    return worstIndex != memoryBlocks.size() &&
           allocateAt(processId, size, worstIndex);
}

// Thu hồi bộ nhớ theo Process ID
bool MemoryManager::deallocate(int processId) {
    if (processId < 0) {
        return false;
    }

    for (Block& block : memoryBlocks) {
        if (!block.isFree && block.processId == processId) {
            block.isFree = true;
            block.processId = -1;
            coalesce();
            return true;
        }
    }
    return false;
}

// Cơ chế gộp các khối trống liền kề nhau để giảm phân mảnh
void MemoryManager::coalesce() {
    std::size_t i = 0;
    while (i + 1 < memoryBlocks.size()) {
        if (memoryBlocks[i].isFree && memoryBlocks[i + 1].isFree) {
            memoryBlocks[i].size += memoryBlocks[i + 1].size;
            memoryBlocks.erase(memoryBlocks.begin() + i + 1);
        } else {
            ++i;
        }
    }
}

// Hàm in trạng thái bộ nhớ ra màn hình Console (hữu ích cho việc test độc lập)
void MemoryManager::displayMemory() const {
    std::cout << "--- MEMORY MAP STATUS ---\n";
    for (std::size_t i = 0; i < memoryBlocks.size(); ++i) {
        std::cout << "Block " << i
                  << " | Start: " << memoryBlocks[i].startAddress
                  << " | Size: " << memoryBlocks[i].size
                  << " | Status: "
                  << (memoryBlocks[i].isFree
                          ? "FREE"
                          : "ALLOCATED (P" + std::to_string(memoryBlocks[i].processId) + ")")
                  << "\n";
    }
    std::cout << "-------------------------\n";
}

// Trả về danh sách các khối nhớ để truyền dữ liệu lên giao diện GUI
const std::vector<Block>& MemoryManager::getMemoryBlocks() const {
    return memoryBlocks;
}

// Đặt lại trạng thái ban đầu của bộ nhớ
void MemoryManager::reset(int totalSize) {
    if (totalSize <= 0) {
        throw std::invalid_argument("Total memory size must be positive.");
    }

    std::vector<Block> initialBlocks;
    initialBlocks.emplace_back(1, 0, totalSize, true, -1);
    memoryBlocks.swap(initialBlocks);
}
