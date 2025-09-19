#include <DHT.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// --- Cấu hình DHT ---
#define DHTPIN 4        // GPIO4
#define DHTTYPE DHT11   // đổi thành DHT22 nếu bạn dùng DHT22
DHT dht(DHTPIN, DHTTYPE);

// --- WiFi ---
const char* WIFI_SSID = "HUAWEI P30 Pro";
const char* WIFI_PASS = "1234567890";

// --- ThingSpeak ---
const char* WRITE_URL = "http://api.thingspeak.com/update?api_key=448NOUSI1WW9H9VD";  
// ↑ Write API Key của bạn (để gửi dữ liệu)

// Thay channel_id và Read API Key của bạn vào đây
const char* READ_URL  = "https://api.thingspeak.com/channels/3071980/feeds/last.json?api_key=15ZX2TOL14CTYMJK";  
  
void setup() {
  Serial.begin(9600);
  dht.begin();

  // Kết nối WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Dang ket noi WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nDa ket noi!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void sendDataToThingSpeak(float t, float h) {
  if (WiFi.status() == WL_CONNECTED) {
    char params[64];
    snprintf(params, sizeof(params), "&field1=%.2f&field2=%.2f", t, h);
    String url = String(WRITE_URL) + String(params);

    HTTPClient http;
    http.begin(url);

    int code = http.GET();
    if (code > 0) {
      Serial.printf("Da gui du lieu len ThingSpeak. HTTP %d, Tra ve: %s\n", code, http.getString().c_str());
    } else {
      Serial.printf("Gui du lieu that bai, code: %d\n", code);
    }

    http.end();
  } else {
    Serial.println("Mat ket noi WiFi!");
  }
}

void getDataFromThingSpeak() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(READ_URL);

    int httpCode = http.GET();
    if (httpCode > 0) {
      String payload = http.getString();
      Serial.println("JSON nhan duoc:");
      Serial.println(payload);

      // Parse JSON
      StaticJsonDocument<512> doc;
      DeserializationError error = deserializeJson(doc, payload);

      if (!error) {
        String temp = doc["field1"]; // nhiệt độ
        String humi = doc["field2"]; // độ ẩm

        Serial.print("Du lieu doc tu ThingSpeak -> Nhiet do: ");
        Serial.print(temp);
        Serial.print(" °C, Do am: ");
        Serial.print(humi);
        Serial.println(" %");
      } else {
        Serial.println("Loi parse JSON!");
      }
    } else {
      Serial.printf("Loi HTTP khi doc du lieu, code: %d\n", httpCode);
    }

    http.end();
  } else {
    Serial.println("Mat ket noi WiFi!");
  }
}

void loop() {
  // --- 1. Đọc cảm biến ---
  float h = dht.readHumidity();
  float t = dht.readTemperature(); // °C

  if (isnan(h) || isnan(t)) {
    Serial.println("Loi doc cam bien!");
  } else {
    Serial.printf("Doc cam bien -> Nhiet do: %.2f °C, Do am: %.2f %%\n", t, h);

    // --- 2. Gửi dữ liệu lên ThingSpeak ---
    sendDataToThingSpeak(t, h);
  }

  // --- 3. Lấy dữ liệu từ ThingSpeak ---
  getDataFromThingSpeak();

  delay(20000); // chờ 20s rồi lặp lại (ThingSpeak yêu cầu >=15s giữa 2 lần request)
}
