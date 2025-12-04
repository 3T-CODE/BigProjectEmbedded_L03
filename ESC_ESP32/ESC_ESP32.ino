#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#define RXD1 20
#define TXD1 21
#define ADC_PIN 4 

HardwareSerial MySerial(1); // UART2

const char* ssid = "PIF_CLUB";
const char* password = "chinsochin";

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

unsigned long lastUpdate = 0;
const long interval = 200;
String uartBuffer = "";

void notifyClients(float adcValue, String uartData) {
  String json = "{\"adc\":" + String(adcValue, 2) + ",\"uart\":\"" + uartData + "\"}";
  ws.textAll(json);
}

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
               AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.println("WebSocket client connected");
  }
}

void setup() {
  Serial.begin(115200);
  MySerial.begin(115200, SERIAL_8N1, RXD1, TXD1); // UART1

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected. IP: " + WiFi.localIP().toString());
  delay(2000);
  delay(2000);
  delay(2000);
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    String html = R"rawliteral(
      <!DOCTYPE html>
      <html lang="vi">
      <head>
        <meta charset="UTF-8">
        <title>ESP32-C3 UART & ADC Monitor</title>
        <style>
          body { font-family: Arial; background: #f2f2f2; text-align: center; padding: 30px; }
          .data-box { background: #fff; border-radius: 8px; box-shadow: 0 0 10px rgba(0,0,0,0.1); padding: 20px; margin: 20px auto; width: 300px; }
          .label { font-weight: bold; color: #555; }
          .value { font-size: 24px; color: #007acc; margin-top: 10px; }
        </style>
      </head>
      <body>
        <h1>Giám sát ESP32-C3</h1>
        <div class="data-box">
          <div class="label">Giá trị ADC:</div>
          <div id="adcValue" class="value">--</div>
        </div>
        <div class="data-box">
          <div class="label">Dữ liệu UART:</div>
          <div id="uartData" class="value">--</div>
        </div>
        <script>
          const ws = new WebSocket('ws://' + location.host + '/ws');
          ws.onmessage = function(event) {
            try {
              const data = JSON.parse(event.data);
              if (data.adc !== undefined) {
                document.getElementById('adcValue').innerText = data.adc;
              }
              if (data.uart !== undefined) {
                document.getElementById('uartData').innerText = data.uart;
              }
            } catch (e) {
              console.error("Lỗi dữ liệu WebSocket:", e);
            }
          };
        </script>
      </body>
      </html>
    )rawliteral";
    request->send(200, "text/html", html);
  });

  server.begin();
}

void loop() {
  ws.cleanupClients();
  unsigned long now = millis();
  if (now - lastUpdate >= interval) {
    lastUpdate = now;

    // Đọc ADC
    float adcValue = analogRead(ADC_PIN) * (3.3 / 4095.0);

    // Đọc UART nếu có dữ liệu
    if (MySerial.available()) {
      uartBuffer = MySerial.readStringUntil('\n');
    }

    notifyClients(adcValue, uartBuffer);
  }

  if (MySerial.available()) {
    String data = MySerial.readStringUntil('\n');
    Serial.print("Nhận được: ");
    Serial.println(data);
  }

}
