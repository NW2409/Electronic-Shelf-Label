Đề tài bảng giá điện tử sử dụng BLE gồm:
•	Website quản lý sản phẩm:
Đây là giao diện web cho người dùng (quản lý) nhập hoặc cập nhật thông tin sản phẩm.
Web sử dụng WebSocket để giao tiếp hai chiều theo thời gian thực với ESP32 Gateway.

•	Gateway bảng giá điện tử : 
Là trung tâm giao tiếp giữa Web và các thiết bị BLE.
Kết nối với Web thông qua WebSocket để nhận thông tin sản phẩm ( tên, giá, khuyến mãi…).
Sau đó, gửi thông tin này qua BLE cho node.
Các thành phần chính:
o	Nút nhấn
o	LED: dùng để báo hiệu trạng thái (kết nối, gửi thành công…).
o	Màn LCD 16x2 hiển thị thông tin
o	Khối nguồn: cấp điện cho gateway.

•	Node bảng giá điện tử  :
Nhận thông tin từ gateway qua BLE.
Các thành phần chính:
o	Loadcell: cảm biến đo khối lượng (đặt sản phẩm lên cân).
o	Màn hình epaper : hiển thị thông tin sản phẩm như tên, giá, khuyến mãi… 
