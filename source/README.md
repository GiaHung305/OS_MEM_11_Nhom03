# Mã nguồn C++

- core/: thuật toán độc lập với GUI.
- simulation/: chạy sự kiện, snapshot và trace.
- data_io/: nhập/xuất và kiểm tra CSV.
- gui/: giao diện desktop.

File .h/.hpp và .cpp sẽ nằm trong module tương ứng. Chốt giao diện hàm và cấu trúc dữ liệu trước khi nối các module. GUI gọi core thông qua simulation; không đặt thuật toán chọn vùng trong xử lý nút bấm.
