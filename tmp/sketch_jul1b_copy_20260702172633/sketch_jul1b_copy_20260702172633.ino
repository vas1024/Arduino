#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>

// ==================== НАСТРОЙКИ ====================
const char* ssid = "ИМЯ_ВАШЕГО_WI-FI_В_ДЕРЕВНЕ";
const char* password = "ПАРОЛЬ_ОТ_WI-FI";

// 1. Настройки для Yandex Cloud
const String yandexUrl = "https://yandexcloud.net";

// 2. Настройки для InfluxDB Cloud
// Внимательно замените ИМЯ_БАКЕТА и ВАШ_ТОКЕН на реальные данные
const String influxUrl = "https://influxdata.com";
const String influxToken = "Token ВАШ_ДЛИННЫЙ_API_TOKEN_ИЗ_ИНФЛЮКСА"; 

// Интервал отправки данных (в миллисекундах). 300000 мс = 5 минут
const unsigned long interval = 300000; 
// ====================================================

unsigned long lastMsg = 0;

void setup() {
  Serial.begin(1152 gets0);
  delay(10);

  // Подключаемся к Wi-Fi
  Serial.println();
  Serial.print("Подключение к сети: ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("Wi-Fi подключен успешно!");
  Serial.print("IP адрес платы: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  unsigned long now = millis();
  
  // Проверяем, прошло ли 5 минут с момента последней отправки
  if (now - lastMsg > interval || lastMsg == 0) {
    lastMsg = now;

    if (WiFi.status() == WL_CONNECTED) {
      
      // --- ЭМУЛЯЦИЯ ДАТЧИКОВ ---
      // Генерируем случайную температуру от 20.0 до 25.0 градусов
      float mock_temp = 20.0 + (random(0, 50) / 10.0);
      // Генерируем случайную влажность от 40.0% до 60.0%
      float mock_hum = 40.0 + (random(0, 200) / 10.0);
      
      Serial.println("\n--- Сформированы новые показания ---");
      Serial.print("Температура: "); Serial.println(mock_temp);
      Serial.print("Влажность: "); Serial.println(mock_hum);

      // Создаем объект защищенного клиента
      WiFiClientSecure client;
      client.setInsecure(); // Игнорируем проверку отпечатков сертификатов ради стабильности
      
      HTTPClient http;

      // ====================================================
      // ШАГ 1. ОТПРАВКА В YANDEX CLOUD (Формат JSON)
      // ====================================================
      Serial.print("Отправка в Yandex Cloud... ");
      if (http.begin(client, yandexUrl)) {
        http.addHeader("Content-Type", "application/json");
        
        // Собираем JSON строку для Яндекса
        String jsonPayload = "{\"sensor_id\":\"test_arduino_8266\",\"value\":" + String(mock_temp) + "}";
        
        int httpCode = http.POST(jsonPayload);
        
        if (httpCode > 0) {
          Serial.printf("Успешно! Ответ сервера: %d\n", httpCode);
          // Полнотекстовый ответ Яндекса (можно раскомментировать для отладки):
          // String response = http.getString(); Serial.println(response);
        } else {
          Serial.printf("Ошибка! Причина: %s\n", http.errorToString(httpCode).c_str());
        }
        http.end();
      }

      // ====================================================
      // ШАГ 2. ОТПРАВКА В INFLUXDB CLOUD (Формат Line Protocol)
      // ====================================================
      Serial.print("Отправка в InfluxDB Cloud... ");
      if (http.begin(client, influxUrl)) {
        http.addHeader("Authorization", influxToken);
        http.addHeader("Content-Type", "text/plain; charset=utf-8");
        
        // Собираем строку Line Protocol. Важно: ровно один пробел между тегами и полями
        String linePayload = "home_sensors,room=kitchen,location=village temperature=" + String(mock_temp) + ",humidity=" + String(mock_hum);
        
        int httpCode = http.POST(linePayload);
        
        if (httpCode > 0) {
          Serial.printf("Успешно! Код: %d (204 - отлично)\n", httpCode);
        } else {
          Serial.printf("Ошибка! Причина: %s\n", http.errorToString(httpCode).c_str());
        }
        http.end();
      }
      
    } else {
      Serial.println("Ошибка: Нет подключения к Wi-Fi!");
    }
  }
}
