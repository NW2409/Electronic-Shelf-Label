/*
    Dung hai UUID: 1 cai cho Service, 1 cai cho Characteristic
      1. UUID cho Service
        +, UUID cho Service: Dai dien cho Service - "goi chuc nang" ma Server cung cap
        +, Client BLE khi quet thiet bi se nhin thay UUID nay va biet Server co dich vu phu hop voi minh
      2. UUID cho Characteristic
        +, UUID dai dien cho Characteristic - la tung "du lieu con" trong 1 Service
      std::vector<std::string> allowedMacs = {
        "30:AE:A4:12:34:56",  
        "24:6F:28:AB:CD:EF",
        "24:6F:28:AE:7C:7A",
        "D0:EF:76:31:AA:B6",
        "D0:EF:76:34:75:48"
      };  
  Ex: {"id":"01","product":"Milk","price":"20000","time":"08:38:00 17/4/2025"}
*/
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEClient.h>
#include <BLERemoteService.h>
#include <BLERemoteCharacteristic.h>
#include "esp_bt_device.h" 
#include <GxEPD2_3C.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include "esp_sleep.h"
#include "HX711.h"

#define ENABLE_GxEPD2_GFX 0
#define SERVICE_UUID        "12345678-1234-1234-1234-1234567890ab"
#define CHARACTERISTIC_UUID "abcdefab-1234-5678-1234-abcdefabcdef"
#define SCREEN_WIDTH 250
#define SCREEN_HEIGHT 122
#define LOGO_WIDTH 65
#define LOGO_HEIGHT 65

#define BATTERY_PIN 2
#define BATTERY_THRESHOLD 84.84

const unsigned char logo  [] =   {
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x80, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x80, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x80, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0x80, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x80, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0x80, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x80, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x80, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0x80, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x80, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0x80, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe0, 0xff, 0xff, 0x80, 0xff, 0xff, 0xff, 0xff, 
	0xf8, 0x00, 0x0f, 0xff, 0x80, 0xff, 0xff, 0xff, 0xff, 0x80, 0xff, 0x83, 0xff, 0x80, 0xff, 0xff, 
	0xff, 0xfc, 0x0f, 0xf7, 0xf8, 0xff, 0x80, 0xff, 0xff, 0xff, 0xe0, 0xf8, 0x00, 0x1c, 0x7f, 0x80, 
	0xff, 0xff, 0xff, 0x83, 0xc0, 0x7f, 0x07, 0x3f, 0x80, 0xff, 0xff, 0xfe, 0x1e, 0x0f, 0xff, 0xf1, 
	0x9f, 0x80, 0xff, 0xff, 0xf8, 0x70, 0x7f, 0xff, 0xf8, 0xcf, 0x80, 0xff, 0xff, 0xe1, 0xc3, 0xff, 
	0xf1, 0xfe, 0x67, 0x80, 0xff, 0xff, 0x87, 0x0f, 0xff, 0xe0, 0x7e, 0x27, 0x80, 0xff, 0xfe, 0x1c, 
	0x3f, 0xff, 0xce, 0x3f, 0x33, 0x80, 0xff, 0xfc, 0x70, 0xff, 0xff, 0xce, 0x3f, 0x93, 0x80, 0xff, 
	0xf1, 0xc3, 0xff, 0xff, 0xe0, 0x7f, 0x93, 0x80, 0xff, 0xe3, 0x0f, 0xff, 0xff, 0xf9, 0xff, 0xdb, 
	0x80, 0xff, 0xce, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xdb, 0x80, 0xff, 0x18, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xd9, 0x80, 0xfe, 0x31, 0xff, 0xff, 0xff, 0xff, 0xff, 0xd9, 0x80, 0xfc, 0x63, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0x99, 0x80, 0xfc, 0xcf, 0xff, 0xff, 0xff, 0xff, 0xff, 0x91, 0x80, 0xf9, 0x9f, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0x91, 0x80, 0xf3, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0x31, 0x80, 
	0xf2, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x61, 0x80, 0xe6, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xfc, 
	0x41, 0x80, 0xe4, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf8, 0x89, 0x80, 0xec, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xe3, 0x19, 0x80, 0xcd, 0xff, 0xff, 0x03, 0xff, 0xff, 0x06, 0x39, 0x80, 0xcd, 0xff, 0xf0, 
	0x00, 0x07, 0xf8, 0x1c, 0x7b, 0x80, 0xcc, 0xff, 0xc1, 0xff, 0x00, 0x00, 0xf1, 0xf3, 0x80, 0xc4, 
	0xff, 0x0f, 0x87, 0xff, 0x1f, 0x83, 0xf3, 0x80, 0xc6, 0x00, 0x38, 0x00, 0x0f, 0xfc, 0x1f, 0xe7, 
	0x80, 0xc3, 0x01, 0xe0, 0xff, 0x00, 0x00, 0xff, 0xe7, 0x80, 0xc1, 0xff, 0x87, 0xff, 0xfc, 0x0f, 
	0xff, 0xcf, 0x80, 0xcc, 0x00, 0x3f, 0xff, 0xff, 0xff, 0xff, 0x9f, 0x80, 0xce, 0x00, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0x3f, 0x80, 0xcf, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x3c, 0x7f, 0x80, 0xcf, 0xff, 
	0xff, 0xff, 0xff, 0xf0, 0x70, 0xff, 0x80, 0xe7, 0xff, 0xff, 0xff, 0xff, 0xf3, 0xc3, 0xff, 0x80, 
	0xe7, 0xff, 0xfc, 0x00, 0x3f, 0xfe, 0x0f, 0xff, 0x80, 0xe3, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x3f, 
	0xff, 0x80, 0xf1, 0xff, 0x83, 0xff, 0xe0, 0x03, 0xff, 0xff, 0x80, 0xf8, 0x7e, 0x1f, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0x80, 0xfe, 0x00, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0x80, 0xff, 0xc7, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0x80, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x80, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x80, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0x80, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x80, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0x80, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x80, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0x80, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x80, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x80, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x80, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x80
};
// CS (Chip Select) → GPIO 5
// DC (Data/Command) → GPIO 17
// RST (Reset) → GPIO 16
// BUSY (Busy Status Pin) → GPIO 4
// SCK (Clock) → GPIO 18
// MOSI (Data Out) → GPIO 23
GxEPD2_3C<GxEPD2_213_Z98c, GxEPD2_213_Z98c::HEIGHT> display(GxEPD2_213_Z98c(5, 17, 16, 4));
volatile bool newDataAvailable = false;

static BLEAddress *pServerAddress;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;

String receivedData = "";

int battery;
bool warningSent = false;

String ID = "";
String Product = "";
int new_price = 0;

// HX711 wiring
const int LOADCELL_DOUT_PIN = 35;
const int LOADCELL_SCK_PIN = 12;
HX711 scale;
float weight;

int Price = 0;
int Sale = 0;
String Time = "";

int readBatteryPercent() {
  int value = analogRead(BATTERY_PIN);
  float voltage = value * 3.3 / 4095.0 * 1.27; // Hệ số chia áp bạn dùng (R1=27k, R2=100k)
  int percent = map(voltage * 100, 0, 330, 0, 100); // từ 0V đến 3.3V tương ứng 0–100%
  return constrain(percent, 0, 100);
}

float readWeightGrams() {
  if (scale.is_ready()) {
    return (scale.get_units(10) + 1858.35); // trung bình 10 lần
  } else {
    Serial.println("HX711 not found.");
    return -1;
  }
}

void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {

  String chunk = "";
  for (size_t i = 0; i < length; i++) {
    chunk += (char)pData[i];
  }

  Serial.print("Chunk received: ");
  Serial.println(chunk);

  if (chunk == "|END|") {
    Serial.println("Full JSON received:");
    Serial.println(receivedData);
    newDataAvailable = true;

    int idx;
    idx = receivedData.indexOf("\"id\":\"");
    if (idx != -1) {
      ID = receivedData.substring(idx + 6, receivedData.indexOf("\"", idx + 6));
    }
    idx = receivedData.indexOf("\"product\":\"");
    if (idx != -1) {
      Product = receivedData.substring(idx + 11, receivedData.indexOf("\"", idx + 11));
    }
    idx = receivedData.indexOf("\"price\":\"");
    if (idx != -1) {
      String priceStr = receivedData.substring(idx + 9, receivedData.indexOf("\"", idx + 9));
      Price = priceStr.toInt();
    }
    idx = receivedData.indexOf("\"sale\":\"");
    if (idx != -1) {
      String saleStr = receivedData.substring(idx + 8, receivedData.indexOf("\"", idx + 8));
      Sale = saleStr.toInt();
    }
    // Tách Time
    idx = receivedData.indexOf("\"time\":\"");
    if (idx != -1) {
      Time = receivedData.substring(idx + 8, receivedData.indexOf("\"", idx + 8));
    }
    Serial.println("Đã phân tích xong JSON:");
    Serial.println("ID: " + ID);
    Serial.println("Product: " + Product);
    Serial.print("Price: "); Serial.println(Price);
    Serial.print("Sale: "); Serial.println(Sale);
    Serial.println("Time: " + Time);
    if (newDataAvailable) {  // Nếu có dữ liệu mới, cập nhật màn hình
          Serial.println("Cập nhật màn hình ePaper...");
          newDataAvailable = false;  // Reset flag sau khi cập nhật xong
          display.firstPage();
          do {
              drawSaleScreen();  // Hàm vẽ nội dung lên màn hình
          } while (display.nextPage());

          display.hibernate();  // Đưa màn hình vào chế độ ngủ để tiết kiệm pin
    }
    receivedData = "";  // clear buffer sau khi xử lý
    Serial.println("Sleep 60s...");
    esp_sleep_enable_timer_wakeup(60 * 1000000ULL);
    esp_deep_sleep_start();
  } else {
    receivedData += chunk;
  }
}

bool connectToServer(BLEAddress pAddress) {
  Serial.print("Connecting to server: ");
  Serial.println(pAddress.toString().c_str());

  BLEClient* pClient = BLEDevice::createClient();
  Serial.println("Client created");

  if (!pClient->connect(pAddress)) {
    Serial.println("Failed to connect to server.");
    return false;
  }
  Serial.println("Connected to server.");
  
  BLERemoteService* pRemoteService = pClient->getService(SERVICE_UUID);
  if (pRemoteService == nullptr) {
    Serial.println("Failed to find service UUID.");
    return false;
  }

  pRemoteCharacteristic = pRemoteService->getCharacteristic(CHARACTERISTIC_UUID);
  if (pRemoteCharacteristic == nullptr) {
    Serial.println("Failed to find characteristic UUID.");
    return false;
  }

  if (pRemoteCharacteristic->canNotify()) {
    pRemoteCharacteristic->registerForNotify(notifyCallback);
    Serial.println("Notification registered.");
  }

  return true;
}

// Scan callback to find target device
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.getServiceUUID().equals(BLEUUID(SERVICE_UUID))) {
      Serial.println("Found target BLE server.");
      advertisedDevice.getScan()->stop();
      pServerAddress = new BLEAddress(advertisedDevice.getAddress());
      doConnect = true;
    }
  }
};

void drawSaleScreen() {
  int16_t x1, y1;
  uint16_t w, h;
  uint16_t old_price_width, old_price_height, new_price_width, new_price_height;
  display.fillScreen(GxEPD_WHITE);

  // Vẽ đường kẻ giữa màn hình
  display.drawLine(0, SCREEN_HEIGHT / 2, 2 * SCREEN_WIDTH / 3, SCREEN_HEIGHT / 2, GxEPD_BLACK);

  // ======== TÊN SẢN PHẨM ========
  display.setFont(&FreeSansBold12pt7b);
  display.setTextColor(GxEPD_BLACK);

  // Tính toán độ dài tên sản phẩm
  display.getTextBounds(Product, 0, 0, &x1, &y1, &w, &h);

  // Xác định vị trí căn giữa của sản phẩm
  int logo_left = 175;  // Vị trí logo (bắt đầu từ đây)
  int maxWidth = logo_left - 10; // Khoảng cách từ logo đến lề trái
  int product_x = (maxWidth - w) /2 + 5; // Căn giữa tên sản phẩm
  int product_y = SCREEN_HEIGHT / 2 -20; // Vị trí bắt đầu theo chiều dọc

  // Cắt tên sản phẩm xuống dòng nếu quá dài
  String productString = Product;
  if (w > maxWidth) {
    int i = productString.length();
    while (i > 0) {
      display.getTextBounds(productString.substring(0, i), 0, 0, &x1, &y1, &w, &h);
      if (w <= maxWidth) break;
      i--;
    }
    productString = productString.substring(0, i) + "..."; // Thêm dấu "..." nếu sản phẩm quá dài
  }

  // Vẽ tên sản phẩm
  display.setCursor(product_x, product_y);
  display.print(productString);

  // ======== ID góc trái dưới ========
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(GxEPD_BLACK);
  display.setCursor(10, SCREEN_HEIGHT - 10);
  display.print("ID:");
  display.print(ID);

  // ======== GIÁ CŨ ========
  char old_price_str[50];  
  sprintf(old_price_str, "%d", Price);  
  display.getTextBounds(old_price_str, 0, 0, &x1, &y1, &old_price_width, &old_price_height);
  int old_price_x = SCREEN_WIDTH - old_price_width - 10;
  display.setCursor(old_price_x, SCREEN_HEIGHT - 30);
  display.setTextColor(GxEPD_BLACK);
  display.print(old_price_str);
  // sprintf(old_price_str, "%s", formatNumber(Price).c_str());  
  // display.getTextBounds(old_price_str, 0, 0, &x1, &y1, &old_price_width, &old_price_height);
  // int old_price_x = SCREEN_WIDTH - old_price_width - 10;

  display.setCursor(old_price_x, SCREEN_HEIGHT - 30);
  display.setTextColor(GxEPD_BLACK);
  display.print(old_price_str);

  // ======== SALE VÀ GIÁ MỚI ========
  if (Sale > 0) {
    // Hộp SALE
    int sale_box_x = 5;
    int sale_box_y = SCREEN_HEIGHT - 40 - 8;
    int sale_box_width = 80;
    int sale_box_height = 70;
    display.fillRect(sale_box_x, sale_box_y, sale_box_width, sale_box_height, GxEPD_RED);

    display.setFont(&FreeMonoBold9pt7b);
    display.setTextColor(GxEPD_WHITE);
    display.setCursor(sale_box_x + 8, sale_box_y + 22); 
    display.print("SALE");
    display.setCursor(sale_box_x + 8, sale_box_y + 42);
    display.printf("%d%%", Sale);

    // Gạch ngang giá cũ
    display.drawLine(old_price_x, SCREEN_HEIGHT - 36, old_price_x + old_price_width, SCREEN_HEIGHT - 36, GxEPD_RED);

    // Hiển thị giá mới
    char new_price_str[50];
    new_price = Price * (100 - Sale) / 100;
    sprintf(new_price_str, "%d", new_price);
    display.getTextBounds(new_price_str, 0, 0, &x1, &y1, &new_price_width, &new_price_height);
    int new_price_x = SCREEN_WIDTH - new_price_width - 10;
    display.setCursor(new_price_x, SCREEN_HEIGHT - 10);
    display.setTextColor(GxEPD_RED);
    display.print(new_price_str);

    // Hiển thị thời gian nếu Sale > 0
    display.setFont(&FreeMonoBold9pt7b);
    display.setTextColor(GxEPD_BLACK);
    int time_x = (SCREEN_WIDTH - 150) / 2;  // Căn giữa thời gian
    display.setCursor(time_x, SCREEN_HEIGHT / 2 + 15);  // Vị trí trên
    display.print(Time);        // Hiển thị biến Time
  }

  // ======== LOGO ========
  display.drawBitmap(logo_left, 5, logo, LOGO_WIDTH, LOGO_HEIGHT, GxEPD_RED);
}
void setup() {
  Serial.begin(115200);
  // Init HX711
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(183.91);
  Serial.println("Scale initialized.");

  Serial.println("Starting BLE Client scan...");
  BLEDevice::init("");
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1500);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(60, false);

  const uint8_t* mac = esp_bt_dev_get_address();
  char macStr[18];
  sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial.print("BLE MAC: ");
  Serial.println(macStr);

  display.init(115200, true, 50, false);
  display.setRotation(1);
  display.setFullWindow();
  battery = readBatteryPercent();
  warningSent = false;
  Serial.print("Battery Level: "); Serial.print(battery); Serial.println("%");
  weight = readWeightGrams();
  Serial.print("Weight: "); Serial.println(weight, 2);
}

void loop() {
  static unsigned long scanStart = millis();

  if (doConnect) {
    if (connectToServer(*pServerAddress)) {
      Serial.println("Ready to receive JSON data.");
      connected = true;
      
      if ((battery < BATTERY_THRESHOLD || weight < 100) && !warningSent) {
        Serial.println("Gửi cảnh báo (pin yếu hoặc cân nhẹ)...");
        if (connected && pRemoteCharacteristic && pRemoteCharacteristic->canWrite()) {
          String jsonPayload = "{\"power\":" + String(battery) + ",\"weight\":" + String(weight, 1) + "}";
          int chunkSize = 20;
          for (int i = 0; i < jsonPayload.length(); i += chunkSize) {
            String chunk = jsonPayload.substring(i, i + chunkSize);
            Serial.print("Chunk sent: ");
            Serial.println(chunk);
            pRemoteCharacteristic->writeValue((uint8_t*)chunk.c_str(), chunk.length());
            delay(100); 
          }
          Serial.println("Đã gửi JSON cảnh báo pin yếu:");
          Serial.println(jsonPayload);
          warningSent = true;
        } else {
          Serial.println("Chưa kết nối hoặc không thể ghi vào characteristic.");
        }
      }
      
    } else {
      Serial.println(" Failed to connect to server.");
    }
    doConnect = false;
  }

  if (connected && !pRemoteCharacteristic->getRemoteService()->getClient()->isConnected()) {
    Serial.println("Lost connection. Rescanning...");
    connected = false;
    BLEDevice::getScan()->start(60, false);
    scanStart = millis();
  }

  // Nếu sau 120s không kết nối được → ngủ
  if (!connected && (millis() - scanStart > 1000)) {
    Serial.println("Không tìm thấy server trong 60s. Đi ngủ...");
    esp_sleep_enable_timer_wakeup(60 * 1000000ULL);
    esp_deep_sleep_start();
  }

  delay(100);
}
