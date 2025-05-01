const fs = require("fs");
const WebSocket = require("ws");

const wss = new WebSocket.Server({ port: 8080 });
console.log("WebSocket Server đang chạy trên cổng 8080...");

let esp32Clients = new Set(); // Lưu ESP32 clients
let browserClients = new Set(); // Lưu browser clients

wss.on("connection", (ws) => {
    console.log("Client đã kết nối!");

    ws.on("message", (message) => {
        try {
            const data = JSON.parse(message);
            console.log("Dữ liệu nhận được:", data);

            // Gán loại client nếu là gói đăng ký
            if (data.device === "ESP32") {
                esp32Clients.add(ws);
                console.log("ESP32 đã đăng ký.");
                return;
            } else if (data.device === "browser") {
                browserClients.add(ws);
                console.log("Browser đã đăng ký.");
                return;
            }

            // Nếu dữ liệu đến từ trình duyệt, chỉ gửi xuống ESP32
            if (browserClients.has(ws)) {
                esp32Clients.forEach((client) => {
                    if (client.readyState === WebSocket.OPEN) {
                        client.send(JSON.stringify(data));
                        console.log("Đã gửi dữ liệu xuống ESP32:", data);
                    }
                });
            }

            // Nếu dữ liệu đến từ ESP32, chỉ gửi đến trình duyệt
            if (esp32Clients.has(ws)) {
                browserClients.forEach((client) => {
                    if (client.readyState === WebSocket.OPEN) {
                        client.send(JSON.stringify(data));
                        console.log("Đã gửi dữ liệu đến trình duyệt:", data);
                    }
                });
            }

        } catch (error) {
            console.error("Lỗi khi xử lý dữ liệu:", error);
        }
    });

    ws.on("close", () => {
        console.log("Client đã ngắt kết nối.");
        esp32Clients.delete(ws);
        browserClients.delete(ws);
    });
});
