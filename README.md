# OS_MEM_11 – Nhóm 03

Đồ án Hệ điều hành: mô phỏng cấp phát/thu hồi vùng nhớ liên tục bằng First-Fit, Best-Fit và Worst-Fit.

## Thành viên
| Mã | Họ tên | GitHub |
|---|---|---|
| TV1 | Chưa điền | Chưa điền |
| TV2 | Chưa điền | Chưa điền |
| TV3 | Chưa điền | Chưa điền |
| TV4 | Chưa điền | Chưa điền |
| TV5 | Chưa điền | Chưa điền |

## Công nghệ và trạng thái
- Ngôn ngữ: C++.
- Sản phẩm mục tiêu: ứng dụng GUI chạy trên Windows.
- Thư viện GUI, compiler và công cụ build: chưa chốt.
- Hiện tại chỉ khởi tạo cấu trúc repository; chưa có ứng dụng chạy được.

## Cấu trúc
| Đường dẫn | Trách nhiệm |
|---|---|
| source/core/ | Chiến lược chọn vùng, cấp phát, thu hồi, gộp vùng và fragmentation |
| source/simulation/ | Thực thi sự kiện, bước chạy, snapshot và trace |
| source/data_io/ | Đọc, kiểm tra và xuất CSV |
| source/gui/ | Điều khiển GUI, memory map, bảng trace và so sánh |
| input/ | CSV mẫu và hướng dẫn định dạng |
| output/ | CSV kết quả/trace mẫu và hướng dẫn đối chiếu |
| tests/unit/ | Kiểm thử thuật toán và hàm xử lý dữ liệu |
| tests/integration/ | Kiểm thử luồng CSV → mô phỏng → xuất kết quả |
| tests/performance/ | Stress/performance test và phương pháp chạy |
| tests/expected/ | Oracle tính tay độc lập với GUI |
| report/weekly/ | Bằng chứng tiến độ 8 tuần |
| report/figures/ | Hình/sơ đồ cho báo cáo |
| report/test_report/ | Expected/actual, log và số liệu đo |
| report/demo/ | Kịch bản và link video công khai |

## Lấy mã nguồn
```bash
git clone https://github.com/GiaHung305/OS_MEM_11_Nhom03.git
cd OS_MEM_11_Nhom03
```

## Cài đặt, build, chạy và kiểm thử
Sẽ bổ sung lệnh chính xác và phiên bản công cụ sau khi nhóm chốt môi trường C++ và GUI.
Không có lệnh chạy ứng dụng ở giai đoạn khởi tạo.

## Dữ liệu và kết quả
Đọc input/README.md và output/README.md.
Định dạng CSV sẽ được chốt trước khi hiện thực reader/writer.
Chưa có CSV mẫu hoặc kết quả thực nghiệm.

## Báo cáo và demo
- Khung báo cáo: report/README.md.
- Tiến độ: report/weekly/.
- Bảng kiểm thử và số liệu: report/test_report/.
- Link video công khai: report/demo/README.md (chưa có video).
- Quy tắc cộng tác: CONTRIBUTING.md.
