#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>        
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <ESP8266HTTPClient.h>


// Настройки WiFi
const char* ssid = "Paris";
const char* password = "ps97889600P";
// Настройки MQTT
const char* mqtt_server = "192.168.1.3";  
const int mqtt_port = 1883;
// Интервал отправки (миллисекунды)
const unsigned long sendInterval = 30000;  // 30 секунд
unsigned long lastSendTime = 0;
// MQTT локальные топики
const char* tempTopic1 = "arduino_thermometer/air_inside";
const char* tempTopic2 = "arduino_thermometer/heater";
const char* tempTopic3 = "arduino_thermometer/air_outside";

//#define ONE_WIRE_BUS D1
//#define ONE_WIRE_BUS 5  // GPIO5 (это и есть D1)
#define PIN_SENSOR1 5   // D1
#define PIN_SENSOR2 4   // D2
//#define PIN_SENSOR3 0   // D3 (осторожно - используется при загрузке)
#define PIN_SENSOR3 14  // D5 - безопасно

//PIN_SENSOR1 мапится в tempTopic1
//PIN_SENSOR2 мапится в tempTopic2
//PIN_SENSOR3 мапится в tempTopic3

OneWire oneWire1(PIN_SENSOR1);
OneWire oneWire2(PIN_SENSOR2);
OneWire oneWire3(PIN_SENSOR3);

DallasTemperature sensor1(&oneWire1);
DallasTemperature sensor2(&oneWire2);
DallasTemperature sensor3(&oneWire3);

struct Sensors {
  const char* topic;
  DallasTemperature* dallas;
  String temp;
};

int const numOfSensors = 3;
Sensors sensors[numOfSensors] = {
  {tempTopic1, &sensor1,  ""},
  {tempTopic2, &sensor2,  ""},
  {tempTopic3, &sensor3,  ""},
};




WiFiClient espClient;
PubSubClient client(espClient);

WiFiClient espRightech;
PubSubClient rightechClient(espRightech);


// --- Настройки Rightech ---
const char* rightech_server = "dev.rightech.io";
const char* rightech_client_id = "vas-haus-pz"; 
const char* rightech_topic = "haus/arduino/temperature";
const unsigned long sendIntervalRightech = 300000;  // 5 мин
unsigned long lastSendTimeRightech = 0;
//https://dev.rightech.io/#?v=dashboard&m=dashboards&id=69f6eb5d27411287b241875a&t=journal
//ip14922@gmail.com
//System-1system-1


// --- Настройки InfluxDb Grafana ---
const String influxUrl = "https://us-central1-1.gcp.cloud2.influxdata.com/api/v2/write?org=36d3009f718d515a&bucket=bucket1&precision=s";
const String influxToken = "Token aaIYdhng3-hZSxEd_mN7CgvqsZCiHGUHrDeUCQkGiVuYsnBfLRsQ6g3lTRsDPq8AV2NAmDQba4rAFIiubGKoCQ=="; 
const unsigned long sendIntervalInflux = 300000; // 5 мин
unsigned long lastSendTimeInflux = 60000;
// https://grafana.com/
// авторизовался через google


// Переменные для усреднения
float tempSum[3] = {0, 0, 0};
int countSamples[3] = {0, 0, 0};



void setup() {
  delay(2000);  // Даем время для стабилизации
  Serial.begin(115200);

  // Подключение к WiFi
  setupWiFi();


  Serial.println("start initializing temperature sensors");
  for (int i = 0; i < numOfSensors; i++) sensors[i].dallas->begin();
  Serial.println("DS18B20 Temperature Sensors");


  
  // Настройка MQTT
  client.setServer(mqtt_server, mqtt_port);
  rightechClient.setServer(rightech_server, 1883);
  
  Serial.println("Система запущена");

}

void loop() {

  // Проверяем WiFi
  if (WiFi.status() != WL_CONNECTED) {
    setupWiFi();
  }
  
  // Проверяем MQTT
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();
  
  unsigned long currentTime = millis();
  
  // Отправляем данные по таймеру
  if (currentTime - lastSendTime >= sendInterval) {
    sendTemperature(1);
    sendTemperature(2);
    sendTemperature(3);    
    lastSendTime = currentTime;
  }


  currentTime = millis();
  if (currentTime - lastSendTimeRightech >= sendIntervalRightech) {
    sendToRightech();
    lastSendTimeRightech = currentTime;
  }

  currentTime = millis();
  if (currentTime - lastSendTimeInflux >= sendIntervalInflux) {
    lastSendTimeInflux = currentTime;
    updateSensors();
    sendToInflux();
  }


  
  // Короткая пауза
  delay(100);

}




void setupWiFi() {
  delay(10);
  Serial.println();
  Serial.print("Подключение к ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(". ");
  }
  
  Serial.println("");
  Serial.println("WiFi подключен");
  Serial.print("IP адрес: ");
  Serial.println(WiFi.localIP());
}



void reconnectMQTT() {
  if (client.connected()) return;
  
  Serial.print("Подключение к MQTT...");
  
  String clientId = "ArduinoThermometer-";
  clientId += String(WiFi.macAddress());
  
  if (client.connect(clientId.c_str())) {
    Serial.println("подключено");
  } else {
    Serial.print("ошибка, rc=");
    Serial.print(client.state());
    Serial.println(" — попробуем в следующем цикле");
  }
}



void sendTemperature(int sensorNum) {
  DallasTemperature* sensor;
  const char* tempTopic;
  if( sensorNum == 1) {
    sensor = &sensor1;
    tempTopic = tempTopic1;
  }
  else if( sensorNum == 2) {
    sensor = &sensor2;
    tempTopic = tempTopic2;
  }
  else if( sensorNum == 3) {
    sensor = &sensor3;
    tempTopic = tempTopic3;
  }
  else {
    Serial.println("Неправильный номер датчика!");
    return;
  }
  
  sensor->requestTemperatures();
  float tempC = sensor->getTempCByIndex(0);
  
  if (tempC != DEVICE_DISCONNECTED_C) {
    // Форматируем температуру (2 знак после запятой)
    char tempStr[10];
    dtostrf(tempC, 1, 2, tempStr);  // min 1 символа всего, 2 после запятой
    
    // Отправляем в local MQTT
    if (client.connected()) {
      if (client.publish(tempTopic, tempStr, true)) {
        Serial.print("Отправлено: ");
        Serial.print(tempStr);
        Serial.print(" °C в топик ");
        Serial.println(tempTopic);
      } else {
        Serial.print("Ошибка отправки в топик ");
        Serial.println(tempTopic);
      }
    } else {
      Serial.println("Локальный MQTT недоступен, пропускаем");
    }



    tempSum[sensorNum - 1] += tempC;
    countSamples[sensorNum - 1]++;

    
  } else {
    Serial.print("Ошибка датчика ");
    Serial.println(sensorNum);
    client.publish(tempTopic, "", true);
    sensor->begin(); // пробуем переинициализировать для работы в следующем цикле
  }
}



void sendToRightech() {

  
  String avg[3];
  // Считаем среднее
  for( int i = 0; i < 3; i++ ){
    if( countSamples[i] == 0 ){ 
      avg[i]="null";
    } else {
      avg[i] = String( tempSum[i] / countSamples[i] );
    }

  }

  // Формируем JSON пакет (строго под вашу модель)
  // ВНИМАНИЕ: ключи "temperature" и т.д. должны совпадать с кодами в модели Rightech
  String payload = "{";
  payload += "\"air_outside\":" + avg[0] + ",";
  payload += "\"air_inside\":" + avg[1] + ",";
  payload += "\"heater\":" + avg[2];
  payload += "}";

  Serial.println("Отправка в Rightech...");
  
  if (!rightechClient.connected()) {
    if (rightechClient.connect(rightech_client_id)) {
       rightechClient.publish(rightech_topic, payload.c_str());
       Serial.println("Успешно: " + payload);
    } else {
       Serial.println("Ошибка связи с Rightech");
    }
  } else {
     rightechClient.publish(rightech_topic, payload.c_str());
  }

  // Сброс накопителей
  for( int i = 0; i < 3; i++){
    tempSum[i] = 0;
    countSamples[i] = 0;
  }
  rightechClient.disconnect(); // Отключаемся, чтобы не висеть в лимитах
}




void updateSensors() {
  for (int i = 0; i < numOfSensors; i++) {
    sensors[i].dallas->requestTemperatures();
    float t = sensors[i].dallas->getTempCByIndex(0);
    if (t != DEVICE_DISCONNECTED_C) {
      char buf[10];
      dtostrf(t, 1, 2, buf);
      sensors[i].temp = String(buf);
    } else {
      sensors[i].temp = "";
      Serial.println("error with sensor number: " + String(i) + "  topic: " + sensors[i].topic);
      sensors[i].dallas->begin();
    }
  }
}


void sendToInflux(){
  
    if (WiFi.status() == WL_CONNECTED) {
      // Создаем объект защищенного клиента
      WiFiClientSecure client;
      client.setInsecure(); // Игнорируем проверку отпечатков сертификатов ради стабильности
      HTTPClient http;

      // ====================================================
      //  ОТПРАВКА В INFLUXDB CLOUD (Формат Line Protocol)
      // ====================================================
      Serial.print("Отправка в InfluxDB Cloud... ");
      if (http.begin(client, influxUrl)) {
        http.addHeader("Authorization", influxToken);
        http.addHeader("Content-Type", "text/plain; charset=utf-8");
        
        // Собираем строку Line Protocol. Важно: ровно один пробел между тегами и полями
        String linePayload = "home_sensors,location=haus ";  // measurement,теги ПРОБЕЛ
        for (int i = 0; i < numOfSensors; i++) {
          if (i > 0) linePayload += ",";
          linePayload += String(sensors[i].topic) + "=" + sensors[i].temp;
        }
        int httpCode = http.POST(linePayload);

        Serial.println("sent payload:  " + linePayload );
        Serial.println("http code " + String( httpCode ) );
        
        http.end();
      }
      
    } else {
      Serial.println("Ошибка: Нет подключения к Wi-Fi!");
    }


}

