# Kiểm thử

unit/: kiểm thử hàm riêng.
integration/: luồng nhập CSV, mô phỏng và xuất kết quả.
performance/: stress/performance test trên nhiều kích thước.
expected/: oracle tính tay và kết quả chuẩn.

Phải có trường hợp thường, biên, tie, thiếu vùng đủ lớn, thu hồi/gộp vùng, dữ liệu rỗng và không hợp lệ. Ghi expected/actual độc lập với GUI. Framework và lệnh chạy sẽ chốt cùng môi trường build C++.
