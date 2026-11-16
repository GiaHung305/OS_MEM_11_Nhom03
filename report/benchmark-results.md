# Kết quả benchmark Core

Đây là số liệu của một lần chạy bản Release trên Windows ngày 16/11/2026. Mã ở commit `2f25641` với các thay đổi cục bộ về test hồi quy; phần Core không có thay đổi cục bộ. File số liệu thô là [memory-benchmark.csv](memory-benchmark.csv).

## Phương pháp

- Chỉ đo thời gian xử lý các `Event` trong `MemoryManager::apply`, tính bằng micro giây. Không tính sinh workload, kiểm tra stress, GUI hoặc ghi CSV vào thời gian đo.
- Cấu hình mô phỏng có tổng bộ nhớ 1.536 KB và đơn vị cấp phát 4 KB. Ba workload `normal`, `churn`, `fragmentation` có lần lượt 100, 1.000 và 10.000 sự kiện. Mỗi tổ hợp có một lần warmup và năm lần đo; thứ tự chiến lược được xoay giữa các lần đo.
- Bảng dưới ghi median của năm lần đo. Min, max, từng lần đo, seed và số ALLOC thành công/thất bại nằm trong CSV gốc. Số liệu chỉ mô tả lần chạy này, không chứng minh một chiến lược luôn nhanh hơn.

## Median thời gian xử lý (µs)

| Workload | Số event | First-Fit | Best-Fit | Worst-Fit |
|---|---:|---:|---:|---:|
| normal | 100 | 82.300 | 76.500 | 76.000 |
| normal | 1000 | 717.300 | 712.800 | 715.500 |
| normal | 10000 | 6239.900 | 6136.500 | 6153.300 |
| churn | 100 | 80.200 | 78.600 | 80.100 |
| churn | 1000 | 995.900 | 1013.100 | 1058.500 |
| churn | 10000 | 10142.500 | 10639.700 | 10647.500 |
| fragmentation | 100 | 73.000 | 74.300 | 77.300 |
| fragmentation | 1000 | 690.400 | 688.400 | 696.800 |
| fragmentation | 10000 | 6795.400 | 6729.400 | 6771.400 |

Để tái lập: cấu hình CMake Release, build `test_memory_manager`, sau đó chạy `test_memory_manager.exe --benchmark` theo hướng dẫn trong README. Thời gian sẽ thay đổi theo máy và tải hệ thống.
