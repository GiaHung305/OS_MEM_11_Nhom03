# OS_MEM_11 – Nhóm 03

Đồ án Hệ điều hành: mô phỏng cấp phát bộ nhớ bằng First-Fit, Best-Fit và Worst-Fit. Dùng C++17 và Qt Widgets trên Windows.

## Hiện có gì?

- Đã dựng giao diện: vùng chọn file, chọn chiến lược, sơ đồ bộ nhớ, bảng vùng nhớ, lịch sử và các chỉ số.
- Chọn **Compare All** để xem bố cục ba chiến lược cạnh nhau.
- Giao diện chưa nối với thuật toán và chưa đọc/ghi CSV. Các nút xử lý đang bị khóa, bảng chưa có dữ liệu.
- Phần thuật toán và đọc/ghi CSV có sẵn của nhóm được giữ riêng.

## Chạy chương trình

Mở PowerShell tại thư mục dự án rồi chạy:

```powershell
.\build\OS_MEM_11.exe
```

Nếu vừa tải repo về và chưa có thư mục `build`, làm bước bên dưới trước.

## Build lần đầu

Cần cài Qt 6.12.0 MinGW 64-bit, MinGW 13.1.0, CMake và Ninja tại `C:\Qt`. Nếu cài nơi khác, đổi đường dẫn trong lệnh.

Chạy lần lượt trong PowerShell tại thư mục dự án. Chỉ chạy bước tiếp theo khi bước trước thành công:

```powershell
$env:Path = "C:\Qt\Tools\mingw1310_64\bin;C:\Qt\Tools\CMake_64\bin;C:\Qt\Tools\Ninja;C:\Qt\6.12.0\mingw_64\bin;$env:Path"

cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH="C:/Qt/6.12.0/mingw_64"

cmake --build build

& "C:\Qt\6.12.0\mingw_64\bin\windeployqt.exe" --compiler-runtime --no-translations .\build\OS_MEM_11.exe
```

Lệnh cuối chép các thư viện cần thiết vào `build/`. Sau đó, mỗi lần mở chương trình chỉ cần chạy `.\build\OS_MEM_11.exe`, không cần đặt lại PATH.

Khi sửa mã nguồn, thiết lập PATH như trên và chạy lại `cmake --build build`. Đóng chương trình trước khi build. Nếu xóa `build/` hoặc đổi phiên bản Qt, làm lại bước build lần đầu.

## Kiểm thử

Sau khi build, chạy:

```powershell
ctest --test-dir build --output-on-failure
```

Hiện CTest kiểm tra phần thuật toán. Chưa có kiểm thử tự động cho giao diện.

## Thư mục chính

- `source/`: mã nguồn.
- `input/`: CSV đầu vào mẫu.
- `output/`: CSV kết quả mẫu.
- `tests/`: kiểm thử và kết quả mong đợi.
- `report/`: đặc tả và báo cáo. Xem [spec.md](report/spec.md) để biết quy tắc mô phỏng.
- `build/`: chương trình và file sinh khi build, không đưa lên Git.
