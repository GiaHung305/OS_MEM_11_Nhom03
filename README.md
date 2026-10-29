# Mô phỏng cấp phát bộ nhớ – Nhóm 03

Đây là đồ án môn Hệ điều hành của nhóm 03, mô phỏng cách cấp phát và thu hồi bộ nhớ liên tục bằng ba thuật toán **First-Fit, Best-Fit và Worst-Fit**.

Chương trình được viết bằng **C++17**, sử dụng **Qt Widgets** để xây dựng giao diện desktop trên Windows. Nhóm dùng **VS Code** để viết code và build dự án, không bắt buộc sử dụng Qt Creator.

## Chương trình làm gì?

Chương trình hướng đến việc đọc dữ liệu từ CSV, mô phỏng từng thao tác cấp phát hoặc thu hồi, rồi hiển thị trạng thái bộ nhớ sau mỗi bước.

- Chạy riêng First-Fit, Best-Fit hoặc Worst-Fit.
- So sánh ba thuật toán trên cùng một bộ dữ liệu.
- Chạy từng bước hoặc chạy tự động.
- Hiển thị sơ đồ bộ nhớ, danh sách vùng nhớ và lịch sử thao tác.
- Theo dõi dung lượng trống và mức độ phân mảnh.
- Xuất kết quả ra CSV để kiểm tra lại.

Đây là mô phỏng, không tác động đến RAM thật của máy tính.

## Tiến độ hiện tại

Phần thuật toán đã có cấp phát, thu hồi, tách và gộp vùng nhớ, làm tròn theo đơn vị cấp phát, tính các chỉ số phân mảnh và bộ kiểm thử tự động.

Giao diện đã có bố cục chọn file, chọn chiến lược, sơ đồ bộ nhớ, bảng vùng nhớ và lịch sử thao tác. Compare All hiện cho phép xem bố cục ba thuật toán cạnh nhau.

Phần đọc/ghi CSV cơ bản đã có nhưng chưa hoàn thiện việc kiểm tra dữ liệu theo mô hình. GUI chưa kết nối với Core và CSV nên các nút xử lý hiện vẫn bị khóa. Các chức năng mô phỏng đầy đủ sẽ được tích hợp trong giai đoạn tiếp theo.

## Chuẩn bị môi trường

| Công cụ | Phiên bản |
|---|---|
| C++ | C++17 |
| Qt | 6.12.0 – MinGW 64-bit |
| MinGW | 13.1.0 – 64-bit |
| CMake | 3.30.5 |
| Ninja | 1.12.1 |
| Trình soạn thảo | VS Code |

Trong VS Code, cài hai extension **C/C++** và **CMake Tools** của Microsoft.

Các lệnh bên dưới giả định Qt và công cụ build được cài trong `C:\Qt`. Nếu máy bạn cài ở nơi khác, hãy sửa đường dẫn tương ứng.

## Tải mã nguồn

Mở terminal PowerShell và chạy:

```powershell
git clone https://github.com/GiaHung305/OS_MEM_11_Nhom03.git
cd OS_MEM_11_Nhom03
code .
```

Nếu đã tải mã nguồn, chỉ cần mở thư mục dự án trong VS Code. Thư mục được mở phải chứa file `CMakeLists.txt`.

## Build và chạy

Đóng chương trình nếu đang chạy, sau đó mở terminal PowerShell tại thư mục gốc dự án. Chạy từng bước và chỉ chuyển sang bước tiếp theo khi không có lỗi.

### 1. Thiết lập đường dẫn công cụ

```powershell
$env:Path = "C:\Qt\Tools\mingw1310_64\bin;C:\Qt\Tools\CMake_64\bin;C:\Qt\Tools\Ninja;C:\Qt\6.12.0\mingw_64\bin;$env:Path"
```

Lệnh này chỉ có hiệu lực trong terminal hiện tại. Khi mở terminal mới để build, bạn cần chạy lại.

### 2. Cấu hình dự án

```powershell
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH="C:/Qt/6.12.0/mingw_64"
```

### 3. Build

```powershell
cmake --build build
```

### 4. Chép thư viện cần thiết

```powershell
& "C:\Qt\6.12.0\mingw_64\bin\windeployqt.exe" --compiler-runtime --no-translations .\build\OS_MEM_11.exe
```

Lệnh này chép Qt DLL và các thư viện cần thiết vào thư mục build.

### 5. Chạy ứng dụng

```powershell
.\build\OS_MEM_11.exe
```

Sau lần build đầu tiên, nếu chỉ sửa code, bạn thường chỉ cần thiết lập lại PATH, build và chạy. Nếu tạo lại thư mục build hoặc đổi phiên bản Qt, thực hiện lại đầy đủ các bước trên.

## Chạy kiểm thử

Sau khi build, chạy:

```powershell
& "C:\Qt\Tools\CMake_64\bin\ctest.exe" --test-dir build --output-on-failure
```

Bộ test hiện tại kiểm tra Core, gồm lựa chọn vùng nhớ, làm tròn, phân mảnh, gộp vùng, xử lý lỗi, Reset và tính độc lập của snapshot.

Kiểm thử CSV, Controller và GUI sẽ được bổ sung trong các giai đoạn tiếp theo.

## Dữ liệu đầu vào

Các file mẫu nằm trong thư mục `input/`.

### Cấu hình bộ nhớ

File `config_sample.csv`:

```csv
total_memory_kb,allocation_unit_kb,strategy
1024,4,ALL
```

- `total_memory_kb`: tổng dung lượng bộ nhớ mô phỏng.
- `allocation_unit_kb`: đơn vị làm tròn khi cấp phát.
- `strategy`: `FIRST_FIT`, `BEST_FIT`, `WORST_FIT` hoặc `ALL`.

Ví dụ, tiến trình yêu cầu 5 KB với đơn vị cấp phát 4 KB sẽ được cấp 8 KB. Phần dư 3 KB là phân mảnh trong.

### Danh sách thao tác

```csv
step,operation,pid,size_kb
1,ALLOC,P1,100
2,ALLOC,P2,250
3,FREE,P1,
4,ALLOC,P3,80
```

- `step`: thứ tự thao tác, là số nguyên dương và tăng dần.
- `operation`: `ALLOC` để cấp phát hoặc `FREE` để thu hồi.
- `pid`: mã tiến trình, phân biệt chữ hoa và chữ thường.
- `size_kb`: dung lượng yêu cầu khi ALLOC; để trống khi FREE.

Một PID đang giữ bộ nhớ không được cấp phát thêm lần nữa. Sau khi FREE thành công, PID đó có thể được sử dụng lại.

Quy tắc chi tiết được trình bày trong [đặc tả mô phỏng](report/spec.md).

## Cấu trúc dự án

| Thư mục/file | Nội dung |
|---|---|
| `source/` | Mã nguồn Core, CSV, Controller và GUI |
| `input/` | Các bộ CSV đầu vào mẫu |
| `output/` | Nơi lưu kết quả xuất |
| `tests/` | Mã kiểm thử và kết quả mong đợi |
| `report/` | Đặc tả và tài liệu đồ án |
| `CMakeLists.txt` | Cấu hình build và kiểm thử |
| `build/` | File sinh ra khi build, không đưa lên Git |

Phần thuật toán được tách khỏi giao diện. GUI chỉ gửi lệnh và hiển thị kết quả, không tự thực hiện logic First-Fit, Best-Fit hoặc Worst-Fit.

## Bản chạy Windows từ GitHub

GitHub Actions được cấu hình để build, chạy test và đóng gói ứng dụng trên Windows.

Để tải bản chạy sẵn:

1. Mở tab **Actions** của repository.
2. Chọn **Windows CI and package**.
3. Mở một lần chạy thành công.
4. Tải artifact **OS_MEM_11-Windows-x64**.
5. Giải nén toàn bộ và mở `OS_MEM_11.exe`.

Gói này có kèm DLL nên không cần cài Qt để chạy. Chức năng của gói phụ thuộc vào tiến độ mã nguồn tại lần build đó.

CI hiện dùng Qt 6.8.3 và MinGW 13.1, khác với phiên bản Qt trong hướng dẫn build trên máy cá nhân.

## Lưu ý

- Đóng ứng dụng trước khi build lại.
- Không đưa thư mục build hoặc file tạm lên GitHub.
- Nếu thay phiên bản Qt hoặc compiler, nên cấu hình trong một thư mục build mới để tránh dùng lại cache cũ.
- Trao đổi với các thành viên liên quan trước khi thay đổi định dạng dữ liệu hoặc API dùng chung.
