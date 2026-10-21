# OS_MEM_11 – Nhóm 03

Mô phỏng cấp phát và thu hồi vùng nhớ bằng First-Fit, Best-Fit và Worst-Fit.

## Công nghệ đã chốt

- Ngôn ngữ: C++17
- Giao diện: Qt Widgets
- Công cụ biên dịch dự án: CMake
- Hệ điều hành cần chạy: Windows

Hiện tại repo có bộ khung giao diện để kiểm tra môi trường. Thuật toán, CSV và sơ đồ bộ nhớ sẽ được làm ở các công việc tiếp theo.

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

## Cài đặt trên Windows

1. Cài Qt phiên bản 6 bằng Qt Online Installer.
2. Trong trình cài đặt, chọn Qt 6 cho MinGW 64-bit, Qt Creator và CMake.
3. Mở Qt Creator, chọn **Open Project** và mở file `CMakeLists.txt`.
4. Chọn bộ Qt MinGW 64-bit mà Qt Creator tìm thấy.
5. Bấm **Configure Project**, sau đó bấm **Run**.

Khi chạy đúng, cửa sổ có tiêu đề **OS_MEM_11 - Memory Allocation Simulator** sẽ hiện ra.

## Biên dịch bằng dòng lệnh

Mở terminal đã có môi trường Qt:

```bash
cmake -S . -B build
cmake --build build
```

File chạy nằm trong thư mục `build/`. Đường dẫn cụ thể có thể khác tùy bộ biên dịch.

## Quy tắc làm việc nhóm

- Mỗi công việc dùng một nhánh, ví dụ: `feat/QT02-01-first-fit`.
- Nội dung commit phải có mã việc, ví dụ: `feat: implement first-fit [QT02-01]`.
- Không đưa mật khẩu, token, thư mục `build/` hoặc file cấu hình cá nhân lên Git.
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
