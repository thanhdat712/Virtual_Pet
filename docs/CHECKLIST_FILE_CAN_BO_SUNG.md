# Danh sách file cần thêm/hoàn thiện (xếp dọc)

## 1. Đã bổ sung trong bản ZIP này

1. `backend/evaluator/worker.py` — worker xử lý hàng đợi, biên dịch/chạy trong Docker, so sánh output, lưu kết quả, thưởng xu lần đầu đạt.
2. `backend/evaluator/README.md` — hướng dẫn cài đặt, chạy worker và giới hạn sandbox.
3. `backend/evaluator/requirements.txt` — xác nhận chỉ dùng thư viện chuẩn Python.
4. `backend/evaluator/testcases/1.json` — test mẫu bài Hello World.
5. `backend/evaluator/testcases/2.json` — test mẫu bài cộng hai số.
6. `backend/evaluator/testcases/3.json` — test mẫu chẵn/lẻ.
7. `backend/evaluator/testcases/4.json` — test mẫu tìm số lớn nhất.
8. `backend/evaluator/testcases/5.json` — test mẫu sắp xếp.
9. `backend/evaluator/tests/test_worker.py` — test normalize output và cấu hình test case.
10. `tests/tests/test_submission_models.cpp` — đã có sẵn; đăng ký vào CMake để chạy cùng bộ test.

## 2. File hiện có đã sửa để liên kết

1. `backend/src/Server/Database.cpp` — migration thêm bảng thưởng chống trùng và index hàng đợi.
2. `database/schema.sql` — khai báo `code_submission_rewards` và index hàng đợi.
3. `backend/src/server_main.cpp` — thông báo bài nộp đã xếp hàng, chờ worker.
4. `CMakeLists.txt` — thêm target/test `submission_models`, gồm `TestCase.cpp`.
5. `docs/api-contract.md` — cập nhật API và cách chạy worker.
6. `docs/BAO_CAO_PET_SUB.md` — cập nhật phạm vi, kiểm thử và giới hạn triển khai.

## 3. Trước khi đưa lên máy chủ thật vẫn cần

1. Cài Docker Engine, tải image `gcc:13-bookworm` và chạy thử worker với đúng đường dẫn database của backend.
2. Thay/duyệt bộ test mẫu theo đề bài thật; test ẩn phải được quản lý phía server.
3. Chạy kiểm thử tích hợp API + worker + SQLite trên Windows/môi trường triển khai mục tiêu.
4. Đánh giá bảo mật container cho mã không tin cậy; production nên cân nhắc runtime cách ly mạnh hơn như gVisor/Kata và giới hạn log/quan sát.
5. Thiết lập supervisor để tự khởi động worker và xử lý hàng đợi `evaluating` bị bỏ dở khi tiến trình chết.
