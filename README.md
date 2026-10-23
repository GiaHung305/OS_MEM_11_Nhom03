# OS_MEM_11 – Nhóm 03

Mô phỏng cấp phát và thu hồi vùng nhớ bằng First-Fit, Best-Fit và Worst-Fit.

## Công nghệ đã chốt

- Ngôn ngữ: C++17
- Giao diện: Qt Widgets
- Công cụ biên dịch dự án: CMake
- Hệ điều hành cần chạy: Windows

Hiện tại GUI vẫn là bộ khung kiểm tra môi trường. Core đã có First/Best/Worst-Fit, cấp phát/thu hồi, làm tròn theo allocation unit, metrics và snapshot từng event; có test chạy bằng CTest. CSV mới đọc/ghi bảng thô, chưa có validation theo model. Controller và sơ đồ bộ nhớ chưa triển khai.

## Core và kiểm thử

`MemoryManager(totalMemoryKb, allocationUnitKb = 4, strategy = Strategy::FirstFit)`
dùng các kiểu trong `source/models.h`. Mỗi manager chạy một strategy cụ thể;
`Strategy::All` phải được Controller tách thành ba manager độc lập.

- Gọi `apply(Event)` hoặc `allocate(step, pid, requestedSizeKb)` / `deallocate(step, pid)` để nhận `StepResult` có snapshot và metrics, kể cả event thất bại.
- Unit `1 KB` cấp đúng kích thước nguyên KB yêu cầu; unit `4 KB` làm tròn `5 KB` thành `8 KB`, internal fragmentation `3 KB`. Hole dư không phải internal fragmentation.
- `getBlocks()` trả bản sao; `getMetrics()` tính các chỉ số của state hiện tại; `reset()` giữ config và phục hồi một hole ban đầu.
- Các wrapper PID số `allocateFirstFit/BestFit/WorstFit` và `deallocate(int)` vẫn có, dùng cùng logic Core. Constructor một tham số nay dùng unit mặc định `4 KB`; truyền unit `1` nếu cần cách cấp đúng kích thước cũ.
- `Block` riêng đã được thay bằng `MemoryBlock`: dùng `startAddressKb`, `allocatedSizeKb`, `requestedSizeKb`, `processId` chuỗi và `isFree`. Caller bên ngoài repo dùng field cũ cần cập nhật; hiện GUI/Controller trong repo chưa gọi API cũ.

Sau bước configure/build bên dưới, chạy trong cùng terminal:

```powershell
ctest --test-dir build --output-on-failure
```

Test Core dùng expected tính tay cho lựa chọn ba chiến lược, tie-break, làm tròn,
phân mảnh, merge, lỗi không đổi state, reset, snapshot độc lập và biên số nguyên.
Chưa có test tích hợp CSV/Controller/GUI hay benchmark.

## Cấu trúc

- `source/`: mã nguồn C++.
- `input/`: CSV đầu vào mẫu.
- `output/`: CSV kết quả và nhật ký mẫu.
- `tests/`: mã kiểm thử và kết quả tính tay.
- `report/`: báo cáo, bảng kiểm thử và bằng chứng tiến độ.
- `CMakeLists.txt`: hướng dẫn CMake biên dịch chương trình.

## Đặc tả và dữ liệu mẫu

- [Đặc tả mô phỏng](report/spec.md): phạm vi, giả định, cách chọn/tách/gộp vùng, lỗi và cách nối các module.
- [Cấu hình mẫu](input/config_sample.csv): tổng bộ nhớ 1024 KB, đơn vị cấp phát 4 KB và chạy cả ba chiến lược.
- [Dữ liệu bình thường](input/events_normal.csv): cấp phát, thu hồi và tạo các vùng trống khác nhau.
- [Dữ liệu tie](input/events_tie.csv): tạo hai vùng trống bằng nhau để kiểm tra chọn địa chỉ thấp nhất.
- [Dữ liệu biên](input/events_boundary.csv): cấp đúng toàn bộ bộ nhớ và thu hồi để gộp lại.
- [Dữ liệu thiếu vùng liên tục](input/events_out_of_memory.csv): tổng bộ nhớ trống đủ nhưng không có một vùng đủ lớn.
- [Dữ liệu không hợp lệ](input/events_invalid.csv): kích thước âm, thao tác sai, thiếu PID và FREE có size.

## Môi trường chung của nhóm trên Windows

| Thành phần | Phiên bản / lựa chọn thống nhất |
| --- | --- |
| Hệ điều hành | Windows 64-bit |
| Ngôn ngữ | C++17 |
| Qt | Qt 6.12.0, bộ **MinGW 64-bit**, module Widgets |
| Compiler | MinGW 13.1.0 64-bit đi kèm bộ Qt, thư mục `mingw1310_64` |
| CMake | 3.30.5 |
| Công cụ build | Ninja 1.12.1 |
| Thư mục build | `build/` bên trong thư mục gốc repo |

### Cài công cụ

1. Dùng Qt Online Installer, cài vào `C:\Qt` để thuận tiện làm theo hướng dẫn chung. Nếu cài ở nơi khác, thay đường dẫn tương ứng ở các bước dưới.
2. Chọn **Qt 6.12.0 → MinGW 64-bit**. Trong phần công cụ, chọn **MinGW 13.1.0 64-bit**, **CMake** và **Ninja**.
3. Kiểm tra có các file sau trong thư mục cài Qt:
   - `6.12.0\mingw_64\bin\qmake.exe`
   - `Tools\mingw1310_64\bin\c++.exe`
   - `Tools\CMake_64\bin\cmake.exe`
   - `Tools\Ninja\ninja.exe`
4. Mỗi thành viên clone repo và tự cấu hình, build trên máy mình. Không sao chép thư mục `build/` từ máy người khác vì cache chứa đường dẫn riêng của máy đó.

Cài Qt chỉ cài bộ công cụ. Thư mục `build/` được CMake tạo khi cấu hình dự án. Nhóm thống nhất dùng `build/`, không cần tạo `build2/`.

### Cấu hình và chạy bằng PowerShell

Mở PowerShell tại thư mục gốc repo, nơi có `CMakeLists.txt`. Chạy phần thiết lập sau mỗi khi mở terminal mới; các biến này chỉ áp dụng cho terminal hiện tại, không sửa PATH của Windows.

```powershell
# Đổi qtRoot nếu Qt được cài ở vị trí khác.
$qtRoot = 'C:\Qt'
$qtDir = "$qtRoot\6.12.0\mingw_64"
$mingwBin = "$qtRoot\Tools\mingw1310_64\bin"
$env:PATH = "$qtDir\bin;$mingwBin;$qtRoot\Tools\CMake_64\bin;$qtRoot\Tools\Ninja;$env:PATH"

# Kiểm tra phiên bản trước khi cấu hình.
qmake -query QT_VERSION
c++ --version
cmake --version
ninja --version

# Cấu hình dự án. Dừng xử lý lỗi ở bước này trước khi chạy build.
cmake -S . -B build -G Ninja `
    "-DCMAKE_CXX_COMPILER=$mingwBin\c++.exe" `
    "-DCMAKE_MAKE_PROGRAM=$qtRoot\Tools\Ninja\ninja.exe" `
    "-DCMAKE_PREFIX_PATH=$qtDir"
if ($LASTEXITCODE -ne 0) { throw 'CMake cấu hình thất bại.' }

cmake --build build
if ($LASTEXITCODE -ne 0) { throw 'Biên dịch thất bại.' }
```

Sau khi build thành công, chạy ứng dụng trong chính terminal đã thiết lập môi trường:

```powershell
.\build\OS_MEM_11.exe
```

Khi chạy đúng, cửa sổ có tiêu đề **OS_MEM_11 - Memory Allocation Simulator** sẽ hiện ra. Bấm nút **Đóng** để kiểm tra chương trình thoát bình thường.

Các lần sau, thiết lập môi trường trong terminal mới rồi chạy `cmake --build build` và lệnh mở ứng dụng. Khi thay đổi bộ Qt/compiler, chạy lại bước cấu hình.

### Xử lý lỗi môi trường

- **Không tìm thấy `cmake`, `ninja` hoặc `c++`:** kiểm tra đường dẫn cài Qt và chạy lại phần thiết lập PowerShell ở trên.
- **Không tìm thấy Qt6:** kiểm tra `qtDir` trỏ đúng thư mục `6.12.0\mingw_64`, rồi chạy lại lệnh cấu hình có `CMAKE_PREFIX_PATH`.
- **Compiler không chạy hoặc CMake báo cache/generator cũ:** kiểm tra đã thêm thư mục MinGW vào PATH của terminal. Sau đó chạy lại nguyên lệnh cấu hình ở trên với `cmake --fresh -S . -B build ...` (giữ đủ các tham số còn lại), rồi build lại. `--fresh` tạo lại cache trong `build/`; không cần đổi sang `build2/`.
- **Mở `.exe` báo thiếu DLL Qt hoặc MinGW:** chạy từ terminal đã thiết lập PATH như trên. File `.exe` trong `build/` chưa phải bộ cài độc lập để gửi sang máy khác.

## Quy tắc làm việc nhóm

- Mỗi công việc dùng một nhánh, ví dụ: `feat/QT02-01-first-fit`.
- Nội dung commit phải có mã việc, ví dụ: `feat: implement first-fit [QT02-01]`.
- Không đưa mật khẩu, token, thư mục `build/` hoặc file cấu hình cá nhân lên Git.
- `build/` đã được bỏ qua trong `.gitignore`. Chỉ chia sẻ mã nguồn và cấu hình chung; không commit `CMakeCache.txt` hoặc cấu hình IDE cá nhân.
- Người làm gửi pull request; người kiểm tra chạy lại trước khi xác nhận hoàn thành.

## Kiểm tra QT01-01

TV3 tải repo về máy Windows và làm theo README:

- [ ] CMake cấu hình thành công.
- [ ] Chương trình biên dịch thành công.
- [ ] Cửa sổ mở được.
- [ ] Nút **Đóng** hoạt động.
- [ ] Ghi ngày kiểm tra và kết quả Đạt/Cần sửa vào bảng tiến độ.

## Thành viên và demo

Thông tin 5 thành viên và link video công khai sẽ bổ sung sau.
