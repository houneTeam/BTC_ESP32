#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h> // Библиотека для дисплея

// Настройки Wi-Fi
const char* ssid = "";
const char* password = "";

// Binance API для получения данных о свечах
const char* binanceAPI = "https://api.binance.com/api/v3/klines?symbol=BTCUSDT&interval=1m&limit=20";

TFT_eSPI tft = TFT_eSPI(); // Инициализация дисплея

// Параметры графика
const int graphX = 0;
const int graphY = 20;
const int graphWidth = 320;
const int graphHeight = 220;
const int candleWidth = 14;

struct Candle {
  float open;
  float high;
  float low;
  float close;
};
Candle candles[20]; // Храним последние 20 свечей (интервал 1 минута)

// Время обновления
unsigned long lastUpdate = 0;
const unsigned long updateInterval = 5000; // Обновление каждые 5 секунд

void setup() {
  Serial.begin(115200);

  // Инициализация дисплея
  tft.init();
  tft.setRotation(3);  // Повернуть экран на 180 градусов
  tft.fillScreen(TFT_BLACK);

  // Подключение к Wi-Fi
  connectToWiFi();

  // Первая загрузка данных и отрисовка
  if (fetchCandleData()) {
    drawCandlestickChart();
  }
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - lastUpdate >= updateInterval) {
    lastUpdate = currentMillis;

    if (fetchCandleData()) {
      drawCandlestickChart();
    }
  }
}

// Подключение к Wi-Fi
void connectToWiFi() {
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(10, 0);
  tft.println("Connecting to Wi-Fi...");

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    tft.print(".");
  }

  tft.fillRect(0, 0, 320, 20, TFT_BLACK); // Очищаем область
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setCursor(10, 0);
  tft.println("Connected!");
}

// Получение данных о свечах
bool fetchCandleData() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(binanceAPI);
    int httpResponseCode = http.GET();

    if (httpResponseCode == 200) {
      String payload = http.getString();
      http.end();

      // Разбор JSON
      DynamicJsonDocument doc(8192); // Увеличен буфер для 20 свечей
      DeserializationError error = deserializeJson(doc, payload);
      if (!error) {
        for (int i = 0; i < 20; i++) {
          JsonArray candle = doc[i];
          candles[i].open = candle[1].as<float>();
          candles[i].high = candle[2].as<float>();
          candles[i].low = candle[3].as<float>();
          candles[i].close = candle[4].as<float>();
        }
        return true;
      } else {
        Serial.println("JSON parse error");
      }
    } else {
      Serial.printf("HTTP request failed. Error code: %d\n", httpResponseCode);
    }
    http.end();
  }
  return false;
}

// Отрисовка свечного графика
void drawCandlestickChart() {
  tft.fillScreen(TFT_BLACK); // Полностью очищаем экран

  // Находим минимальную и максимальную цену
  float minPrice = candles[0].low, maxPrice = candles[0].high;
  for (int i = 1; i < 20; i++) {
    if (candles[i].low < minPrice) minPrice = candles[i].low;
    if (candles[i].high > maxPrice) maxPrice = candles[i].high;
  }

  float scale = (float)graphHeight / (maxPrice - minPrice);

  // Рисуем свечи
  for (int i = 0; i < 20; i++) {
    int x = graphX + i * (candleWidth + 2);
    int yOpen = graphY + graphHeight - (candles[i].open - minPrice) * scale;
    int yClose = graphY + graphHeight - (candles[i].close - minPrice) * scale;
    int yHigh = graphY + graphHeight - (candles[i].high - minPrice) * scale;
    int yLow = graphY + graphHeight - (candles[i].low - minPrice) * scale;

    // Вертикальная линия между максимумом и минимумом
    tft.drawLine(x + candleWidth / 2, yHigh, x + candleWidth / 2, yLow, TFT_WHITE);

    // Прямоугольник для тела свечи
    if (candles[i].close > candles[i].open) {
      // Зеленая свеча (рост)
      tft.fillRect(x, yClose, candleWidth, yOpen - yClose, TFT_GREEN);
    } else {
      // Красная свеча (падение)
      tft.fillRect(x, yOpen, candleWidth, yClose - yOpen, TFT_RED);
    }
  }

  // Отображение информации о цене
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(10, 0);
  tft.printf("BTC: %.2f", candles[19].close); // Текущая цена
}
