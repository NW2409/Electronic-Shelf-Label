#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <ArduinoWebsockets.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <EEPROM.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <queue>

#define SERVICE_UUID        "12345678-1234-1234-1234-1234567890ab"
#define CHARACTERISTIC_UUID "abcdefab-1234-5678-1234-abcdefabcdef"

BLECharacteristic *pCharacteristic;
BLEServer *pServer;
BLEAdvertising *pAdvertising;

LiquidCrystal_I2C lcd(0x38, 16, 2); 

bool deviceConnected = false;
bool dataReceivedFlag = false;
String connectedMac = "";
String targetMac = "";
uint16_t lastConnId = 0;

String jsonBuffer = "";
String latestJsonData = "";

std::vector<std::string> jsonChunks;
const int CHUNK_SIZE = 20;

std::queue<std::pair<String, std::vector<std::string>>> jsonQueue;

std::map<String, String> macNameMap = {
  {"01", "D0:EF:76:34:75:4A"},
  {"02", "D0:EF:76:31:AA:B6"},
  {"03", "A0:A3:B3:1E:E6:4A"},
  {"04", "A0:A3:B3:1D:48:16"}
};

#define TIMEZONE "ICT-7"

TaskHandle_t wifiTaskHandle;
TaskHandle_t bleTaskHandle;
TaskHandle_t webSocketTaskHandle;
void WiFiTask(void *pvParameters);
void WebSocketTask(void *pvParameters);
void BLETask(void *pvParameters);

String ID = "";
String Product = "";
String Address = "";
int Price ;
int Sale ;
String Time ="";
int new_price;
String saleStartTime ="";
String saleEndTime ="";


#define LED_XANH 22  // Chân LED Xanh
#define LED_DO 21    // Chân LED Đỏ
#define BUTTON_AP 18 // Chân GPIO của nút nhấn chuyen sang AP

#define AP_SSID "BLE_GATEWAY"
#define AP_PASSWORD "12345678"
const char* ws_server = "ws://192.168.0.103:8080"; // Đổi theo địa chỉ WebSocket ServeR


#define EEPROM_SIZE 256
bool bleTaskStarted = false;

WebServer server(80);
using namespace websockets;
WebsocketsClient client;
String lastData ;

void reconnectWebSocket() {
    int retry = 0;
    client.close(); // Đảm bảo WebSocket được đóng trước khi kết nối lại

    while (!client.connect(ws_server)) {
        Serial.printf("Thử lại kết nối WebSocket lần %d...\n", retry + 1);
        retry++;
        if (retry > 20) {
            Serial.println("Không thể kết nối WebSocket , bỏ qua.");
            lcd.clear();
            lcd.setCursor(0, 0); 
            lcd.print("Unable to connect to WebSocket");
            return;
        }
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
    Serial.println("WebSocket đã kết nối lại!");
    client.send("{\"device\":\"ESP32\"}"); // Gửi đăng ký lại sau khi kết nối
}

void setup() {
    Serial.begin(115200);
    Wire.begin(17, 16);
    lcd.begin(16, 2);
    lcd.backlight();
    lcd.setCursor(0, 0);  // Đặt con trỏ ở dòng 0, cột 0
    lcd.print("BLE GATEWAY");
    pinMode(LED_XANH, OUTPUT);
    pinMode(LED_DO, OUTPUT);
    // digitalWrite(LED_XANH, LOW);
    // digitalWrite(LED_DO, LOW);
    pinMode(BUTTON_AP, INPUT_PULLUP); 

    // Chỉ chạy WebSocketTask khi WiFi đã kết nối
    xTaskCreatePinnedToCore(WiFiTask, "WiFiTask", 4096, NULL, 2, &wifiTaskHandle, 1);
    xTaskCreatePinnedToCore(BLETask, "BLETask", 4096, NULL, 1, &bleTaskHandle, 0);
}

// Hàm lấy và hiển thị thời gian hiện tại
void printLocalTime() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Không thể lấy thời gian SNTP");
        return;
    }
    Serial.printf("Thời gian hiện tại: %02d:%02d:%02d %02d/%02d/%04d\n",
                  timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec,
                  timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900);
}

// Task đồng bộ thời gian qua SNTP
void SNTPTask(void *pvParameters) {
    Serial.println("Đang đồng bộ thời gian SNTP...");
    configTime(7 * 3600, 0, "pool.ntp.org", "time.nist.gov");
    vTaskDelay(pdMS_TO_TICKS(5000)); // Đợi 5 giây để đồng bộ
    while (true) {
        printLocalTime();  // In thời gian hiện tại
        vTaskDelay(pdMS_TO_TICKS(3600000)); // Cập nhật lại sau 1 giờ (3600 giây)
    }
}

//Xu ly viec doc ghi Wifi tư EEPROM
void saveWiFiCredentials(const String& ssid, const String& password, const String& wsip) {
    EEPROM.begin(EEPROM_SIZE);
    EEPROM.writeString(0, ssid);
    EEPROM.writeString(64, password);
    EEPROM.writeString(128, wsip);
    EEPROM.commit();
    EEPROM.end();
}

void loadWiFiCredentials(String& ssid, String& password, String& wsip) {
    EEPROM.begin(EEPROM_SIZE);
    ssid = EEPROM.readString(0);
    password = EEPROM.readString(64);
    wsip = EEPROM.readString(128);
    EEPROM.end();
}

// hàm xóa(chauw sử dụng)
void clearWiFiCredentials() {
    EEPROM.begin(EEPROM_SIZE);
    for (int i = 0; i < EEPROM_SIZE; i++) {
        EEPROM.write(i, 0xFF);  // Ghi giá trị 0xFF vào toàn bộ bộ nhớ EEPROM
    }
    EEPROM.commit();
    EEPROM.end();
}

bool checkInternet() {
    HTTPClient http;
    http.begin("http://clients3.google.com/generate_204");
    int httpCode = http.GET();
    http.end();
    return (httpCode == 204);
}
std::vector<std::string> splitJson(String input, int chunkSize) {
  std::vector<std::string> chunks;
  for (int i = 0; i < input.length(); i += chunkSize) {
    String part = input.substring(i, i + chunkSize);
    chunks.push_back(part.c_str());
  }
  return chunks;
}


void handleReceivedData(String data) {
  latestJsonData = data;

  // Tìm ID tương ứng với connectedMac
  String id = "";
  for (auto& pair : macNameMap) {
    if (pair.second.equalsIgnoreCase(connectedMac)) {
      id = pair.first;
      break;
    }
  }
  // Thêm "id" vào đầu JSON nếu có ID
  if (id != "" && latestJsonData.startsWith("{") && latestJsonData.endsWith("}")) {
    latestJsonData.remove(0, 1);  // Xoá dấu {
    latestJsonData.remove(latestJsonData.length() - 1);  // Xoá dấu }
    latestJsonData = "{\"id\":\"" + id + "\"," + latestJsonData + "}";  // Ghép lại với id ở đầu
  }

  Serial.println("✅ Received full JSON from client (with id on top):");
  Serial.println(latestJsonData);
  if (client.send(latestJsonData)) {
    Serial.println("Sent JSON to server:");
    Serial.println(latestJsonData);
    delay(100);  // Thêm một khoảng thời gian chờ (100ms) sau mỗi lần gửi dữ liệu
  } else {
    Serial.println("Error sending data to WebSocket server.");
  }
}

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* server, esp_ble_gatts_cb_param_t *param) override {
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             param->connect.remote_bda[0], param->connect.remote_bda[1],
             param->connect.remote_bda[2], param->connect.remote_bda[3],
             param->connect.remote_bda[4], param->connect.remote_bda[5]);
    connectedMac = String(macStr);
    connectedMac.toUpperCase();
    lastConnId = param->connect.conn_id;

    Serial.print("Client attempting to connect: ");
    Serial.println(connectedMac);

    if (connectedMac != targetMac) {
      Serial.println("Not target MAC, disconnecting...");
      vTaskDelay(50 / portTICK_PERIOD_MS);
      server->disconnect(lastConnId);
    } else {
      deviceConnected = true;
      Serial.println("Client accepted.");
      lcd.clear();
      lcd.setCursor(0, 0); 
      lcd.print("Client accepted");
      delay(500);
    }
    dataReceivedFlag = false;
  }

  void onDisconnect(BLEServer* pServer) override {
    Serial.println("Client disconnected");
    deviceConnected = false;
    connectedMac = "";
    lastConnId = 0;
    // Tiếp tục quảng bá
    pAdvertising->start();
    dataReceivedFlag = false;
  }

  void onWrite(BLECharacteristic *pCharacteristic) {
    String receivedData = pCharacteristic->getValue().c_str();  // Nhận dữ liệu gửi từ client
    Serial.println("Data received from client:");
    Serial.println(receivedData);  // In ra serial dữ liệu nhận được
  }
};

class MyCharacteristicCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) override {
    String chunk = pCharacteristic->getValue().c_str();
    Serial.println("Received chunk:");
    Serial.println(chunk);

    jsonBuffer += chunk;

    if (chunk == "|END|" || jsonBuffer.endsWith("}")) {
      Serial.println("JSON complete, processing...");
      dataReceivedFlag = true;
      handleReceivedData(jsonBuffer);
      jsonBuffer = ""; // reset sau khi xử lý
    }
  }
};

void startAccessPoint() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    Serial.println("AP Started. Connect and go to 192.168.4.1");
    lcd.clear();
    lcd.setCursor(0, 0); 
    lcd.print("AP Started");
    lcd.setCursor(0, 1); 
    lcd.print("IP: 192.168.4.1");

    server.on("/", HTTP_GET, []() {
        int n = WiFi.scanNetworks();
        String savedSSID, savedPassword, savedWSIP;
        loadWiFiCredentials(savedSSID, savedPassword, savedWSIP);

        String page = R"rawliteral(
            <html>
            <head>
                <meta charset='UTF-8'>
                <style>
                    body {
                        background: linear-gradient(135deg, #f3f4f6, #e3e4e8);
                        text-align: center;
                        font-family: Arial, sans-serif;
                        color: #333;
                    }
                    h2 {
                        font-size: 28px;
                        color: #222;
                    }
                    form {
                        background: white;
                        padding: 20px;
                        border-radius: 10px;
                        display: inline-block;
                        box-shadow: 0px 4px 10px rgba(0, 0, 0, 0.1);
                    }
                    select, input {
                        font-size: 18px;
                        margin: 10px;
                        padding: 8px;
                        border-radius: 5px;
                        border: 1px solid #ccc;
                        width: 90%;
                        max-width: 300px;
                    }
                    input[type='submit'] {
                        background: #4CAF50;
                        color: white;
                        border: none;
                        padding: 10px 20px;
                        cursor: pointer;
                        border-radius: 5px;
                    }
                    input[type='submit']:hover {
                        background: #45a049;
                    }
                    input:disabled {
                        background: #aaa;
                        cursor: not-allowed;
                    }
                    p {
                        color: red;
                        font-size: 16px;
                        margin-top: 5px;
                        display: none;
                    }
                </style>
                <script>
                    function validateForm() {
                        var password = document.getElementById('password').value;
                        var wsip = document.getElementById('wsip').value;
                        var submitBtn = document.getElementById('submitBtn');
                        var warningText = document.getElementById('warningText');
                        if (password.length >= 8 && wsip.length > 0) {
                            submitBtn.disabled = false;
                            warningText.style.display = 'none';
                        } else {
                            submitBtn.disabled = true;
                            warningText.style.display = 'block';
                        }
                    }
                    function togglePassword() {
                        var passwordField = document.getElementById('password');
                        var checkbox = document.getElementById('showPassword');
                        passwordField.type = checkbox.checked ? 'text' : 'password';
                    }
                </script>
            </head>
            <body>
                <h2>Chọn mạng WiFi và nhập IP WebSocket</h2>
                <form action='/connect' method='POST'>
                    <select name='ssid'>
        )rawliteral";

        for (int i = 0; i < n; i++) {
            page += "<option value='" + WiFi.SSID(i) + "'>" + WiFi.SSID(i) + "</option>";
        }

        page += R"rawliteral(
                    </select><br>
                    <input id='password' name='password' type='password' placeholder='Mật khẩu' oninput='validateForm()' required><br>
                    <input type='checkbox' id='showPassword' onclick='togglePassword()'> <label for='showPassword'>Hiển thị mật khẩu</label><br>
                    <input id='wsip' name='wsip' type='text' placeholder='WebSocket IP (VD: 192.168.4.1)' oninput='validateForm()' required><br>
                    <p id='warningText'>Mật khẩu phải >=8 ký tự và IP không được để trống!</p>
                    <input id='submitBtn' type='submit' value='Kết nối' disabled>
                </form>
            </body>
            </html>
        )rawliteral";

        server.send(200, "text/html", page);
    });

    server.on("/connect", HTTP_POST, []() {
        String ssid = server.arg("ssid");
        String password = server.arg("password");
        String wsip = server.arg("wsip");

        saveWiFiCredentials(ssid, password, wsip);

        String response = R"rawliteral(
            <html>
            <head>
                <meta charset='UTF-8'>
                <style>
                    body {
                        text-align: center;
                        font-family: Arial, sans-serif;
                        color: #333;
                    }
                    h2 {
                        font-size: 24px;
                    }
                    button {
                        background: #007bff;
                        color: white;
                        border: none;
                        padding: 10px 20px;
                        font-size: 16px;
                        border-radius: 5px;
                        cursor: pointer;
                        margin-top: 20px;
                    }
                    button:hover {
                        background: #0056b3;
                    }
                </style>
            </head>
            <body>
                <h2>Đã lưu! Đang khởi động lại ESP32...</h2>
                <form action="/" method="GET">
                    <button type="submit">Quay lại cấu hình</button>
                </form>
            </body>
            </html>
        )rawliteral";

        server.send(200, "text/html", response);

        vTaskDelay(3000 / portTICK_PERIOD_MS);
        ESP.restart();
    });

    server.begin();
}


void WiFiTask(void *parameter) {
  String ssid, password, WSIP;

loadWiFiCredentials(ssid, password, WSIP);

    if (ssid == "" || digitalRead(BUTTON_AP) == LOW) {  
        Serial.println("No WiFi. Switch to AP...");
        lcd.clear();
        lcd.setCursor(0, 0); 
        lcd.print("No WiFi.");
        lcd.setCursor(0, 1); 
        lcd.print("Switch to AP...");

    if (bleTaskHandle != NULL) {
        Serial.println("Stop BLE...");
        vTaskDelete(bleTaskHandle);
        bleTaskHandle = NULL;
    }
        startAccessPoint();
        while (true) {
            server.handleClient();
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
    }

    WiFi.begin(ssid.c_str(), password.c_str());
    Serial.print("Connecting to WiFi");
    lcd.clear();
    lcd.setCursor(0, 0); 
    lcd.print("Connecting to WF");
    lcd.setCursor(0, 1); 
    lcd.print(".....");
    for (int i = 0; i < 20; i++) {
        if (WiFi.status() == WL_CONNECTED) break;
        Serial.print(".");
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi connected!");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
        lcd.clear();
        lcd.setCursor(0, 0); 
        lcd.print("IP Address:");
        lcd.setCursor(0, 1); 
        lcd.print(WiFi.localIP());
        digitalWrite(LED_XANH, HIGH);

        if (!checkInternet()) {
            Serial.println("No Internet! Keep AP active.");
            startAccessPoint();
            while (!checkInternet()) {
                server.handleClient();
                vTaskDelay(1000 / portTICK_PERIOD_MS);
            }
        }

        Serial.println("Internet connected! Turn off AP.");
        WiFi.softAPdisconnect(true);

        if (checkInternet()) {
            Serial.println("Start WebSocket Task...");
            xTaskCreate(WebSocketTask, "WebSocketTask", 8192, NULL, 2, &webSocketTaskHandle);
            xTaskCreate(SNTPTask, "SNTP Task", 4096, NULL, 1, NULL);
        } else {
            Serial.println("No Internet, WebSocket Task not started.");
        }

    } else {
        Serial.println("WiFi connection failed! Switching to AP.");
        startAccessPoint();
    }

    vTaskDelete(NULL);
}


void onMessageCallback(WebsocketsMessage message) {
  String newData = message.data();
  // Kiểm tra xem dữ liệu có thay đổi không
  if (newData == lastData) {
      return;
  }
  lastData = newData; // Cập nhật dữ liệu mới
  Serial.print("Nhận dữ liệu WebSocket: ");
  Serial.println(newData); 

  // Chuyển dữ liệu nhận được thành JSON
  StaticJsonDocument<256> doc; // Dung lượng phù hợp với JSON của bạn
  DeserializationError error = deserializeJson(doc, newData);

  if (error) {
      Serial.print("Lỗi phân tích JSON: ");
      Serial.println(error.c_str());
      return;
  }

  // Kiểm tra nếu dữ liệu có giá trị 'null'
  if (doc["id"].isNull() || doc["product"].isNull() || doc["address"].isNull() ||
  doc["price"].isNull() || doc["sale"].isNull() || doc["saleStartTime"].isNull() || 
  doc["saleEndTime"].isNull()) {
      return;
  }

  // Lấy thời gian hiện tại
  time_t now = time(NULL);
  struct tm *currentTime = localtime(&now);
  int currentSeconds = currentTime->tm_hour * 3600 + currentTime->tm_min * 60;
  
  // Chuyển saleStartTime & saleEndTime thành giây trong ngày
  String saleStartTime = doc["saleStartTime"].as<String>();
  String saleEndTime = doc["saleEndTime"].as<String>();

  struct tm saleStartStruct = {0}, saleEndStruct = {0};
  strptime(saleStartTime.c_str(), "%H:%M", &saleStartStruct);
  int startSeconds = saleStartStruct.tm_hour * 3600 + saleStartStruct.tm_min * 60;
  int endSeconds = 86400;  // Mặc định là hết ngày (23:59:59) nếu saleEndTime rỗng
  if (saleEndTime != "") {
      strptime(saleEndTime.c_str(), "%H:%M", &saleEndStruct);
      endSeconds = saleEndStruct.tm_hour * 3600 + saleEndStruct.tm_min * 60;
  }

  // So sánh thời gian
  if (currentSeconds < startSeconds || currentSeconds > endSeconds) {
      doc["sale"] = "0";
  }

  // Tạo JSON mới với dữ liệu cần thiết
  StaticJsonDocument<256> filteredDoc;
  filteredDoc["id"] = doc["id"];
  filteredDoc["product"] = doc["product"];
  filteredDoc["price"] = doc["price"];
  filteredDoc["sale"] = doc["sale"];

  // Thêm thời gian thực dạng HH:MM:SS DD/MM/YYYY
  // Gửi JSON cập nhật
  String newJson;
  serializeJson(filteredDoc, newJson);

  if (newJson.startsWith("{") && newJson.endsWith("}")) {
    Serial.println("JSON input received:");
    Serial.println(newJson);

    // Lấy ID từ JSON
    int idPos = newJson.indexOf("\"id\":\"");
    if (idPos != -1) {
      int idStart = idPos + 6;
      int idEnd = newJson.indexOf("\"", idStart);
      String id = newJson.substring(idStart, idEnd);

      if (macNameMap.count(id)) {
        String mac = macNameMap[id];
        mac.toUpperCase();
        auto chunks = splitJson(newJson, CHUNK_SIZE);
        jsonQueue.push({mac, chunks});
        Serial.println("Queued JSON for ID: " + id + ", MAC: " + mac);
        
        if (targetMac != mac) {
            targetMac = mac;  // Cập nhật targetMac với MAC mới
            Serial.println("Target MAC updated to: " + targetMac);
        }
      } else {
        Serial.println("Unknown ID: " + id);
      }
    } else {
      Serial.println("Invalid JSON, no 'id' found.");
    }
  } else {
    Serial.println("Invalid input.");
  }
}

void onEventsCallback(WebsocketsEvent event, String data) {
    if (event == WebsocketsEvent::ConnectionOpened) {
        Serial.println("Kết nối WebSocket thành công!");
        client.send("{\"device\":\"ESP32\"}"); // Gửi đăng ký khi kết nối thành công
    } else if (event == WebsocketsEvent::ConnectionClosed) {
        Serial.println("WebSocket bị đóng, thử kết nối lại...");
        reconnectWebSocket();
    }
}


void WebSocketTask(void *pvParameters) {
    client.onMessage(onMessageCallback);
    client.onEvent(onEventsCallback);
    reconnectWebSocket(); // Kết nối ngay khi task bắt đầu

    while (1) {
        if (WiFi.status() == WL_CONNECTED) {
            client.poll(); // Kiểm tra và nhận dữ liệu WebSocket
        } else {
            Serial.println("Mất kết nối WiFi, đợi 5s thử lại...");
            vTaskDelay(pdMS_TO_TICKS(5000));
        }
        vTaskDelay(pdMS_TO_TICKS(50));  // Giảm thời gian chờ để nhận dữ liệu nhanh hơn
    }
}
void processJsonQueue() {
    if (jsonQueue.empty()) {
      return;  // Không làm gì nếu hàng đợi trống
    }
  
    // Lấy dữ liệu đầu tiên từ hàng đợi
    auto& current = jsonQueue.front();
    String mac = current.first;
    auto chunks = current.second;
  
    // Gán targetMac với mac từ hàng đợi mà không cần gọi lại onConnect
    if (mac != targetMac) {
      targetMac = mac;  // Cập nhật targetMac
      Serial.println("Updated target MAC to: " + targetMac);
      lcd.clear();
      lcd.setCursor(0, 0); 
      lcd.print("Updated Data for");
      lcd.setCursor(0, 1); 
      lcd.print(targetMac);
    }
  
    // Kiểm tra nếu đã kết nối và bắt đầu gửi dữ liệu
    if (deviceConnected && !jsonQueue.empty()) {
      delay(500);
      Serial.println("Chunks sent.");
      for (auto& chunk : chunks) {
        delay(100);
        pCharacteristic->setValue(chunk.c_str());
        pCharacteristic->notify();
        Serial.print("📤 Sent: ");
        Serial.println(chunk.c_str());
        
      }
  
      // Gửi ký tự kết thúc
      pCharacteristic->setValue("|END|");
      pCharacteristic->notify();
      Serial.println("✅ All chunks sent.");
      lcd.clear();
      lcd.setCursor(0, 0); 
      lcd.print("Updated Success!");
      // Xóa khỏi hàng đợi
      jsonQueue.pop();
  
      delay(200);
      // Ngắt kết nối và reset targetMac sau khi hoàn tất gửi
      Serial.println("🔌 Disconnecting client...");
      pServer->disconnect(lastConnId);
      delay(1000);
      targetMac = "";  // Reset MAC sau khi ngắt kết nối
    }
}
void BLETask(void* parameter) {
  BLEDevice::init("ESP32_JSON_Server");

  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY
                    );
  pCharacteristic->addDescriptor(new BLE2902());
  pCharacteristic->setCallbacks(new MyCharacteristicCallbacks()); 
  pService->start();

  pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->start();

  Serial.println("📡 BLE Advertising started...");

  while (true) {
 
    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
}

void loop() {
    if (client.available()) {
        client.poll(); // Kiểm tra tin nhắn mới
        processJsonQueue();
    }
    server.handleClient();
}
