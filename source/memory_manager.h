#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <cstddef>
#include <vector>

// Cấu trúc đại diện cho một khối nhớ (Memory Block)
struct Block {
    int id;               // Định danh khối nhớ
    int startAddress;     // Địa chỉ bắt đầu
    int size;             // Kích thước khối nhớ
    bool isFree;          // Trạng thái: true nếu trống, false nếu đã cấp phát
    int processId;        // ID tiến trình đang chiếm giữ (-1 nếu trống)

    Block(int bId, int start, int sz, bool free, int pId = -1)
        : id(bId), startAddress(start), size(sz), isFree(free), processId(pId) {}
};

// Lớp quản lý bộ nhớ chính
class MemoryManager {
private:
    std::vector<Block> memoryBlocks;

    bool isValidAllocation(int processId, int size) const;
    bool allocateAt(int processId, int size, std::size_t blockIndex);

public:
    // Ném std::invalid_argument nếu tổng dung lượng không dương.
    MemoryManager(int totalSize);

    // 3 thuật toán cấp phát bộ nhớ
    bool allocateFirstFit(int processId, int size);
    bool allocateBestFit(int processId, int size);
    bool allocateWorstFit(int processId, int size);

    // Thu hồi bộ nhớ theo Process ID
    bool deallocate(int processId);

    // Gộp các khối nhớ trống kề nhau (Coalescing)
    void coalesce();

    // Các hàm tiện ích phục vụ GUI và Debug
    void displayMemory() const;
    const std::vector<Block>& getMemoryBlocks() const;

    // Ném std::invalid_argument nếu tổng dung lượng không dương.
    void reset(int totalSize);
};

#endif // MEMORY_MANAGER_H