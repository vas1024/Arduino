#include <ESP8266WiFi.h> 
#include <PubSubClient.h>
//#define FASTLED_ESP8266_NODEMCU_PIN_ORDER
//#include <FastLED.h>
#include <Adafruit_NeoPixel.h>

//  i choose board = Generic ESP8266 Module

//#define NUM_LEDS 180  // my tape has 180 leds
#define NUM_LEDS 180

#define MESSAGE_BUFFER_SIZE 1024  // mqtt message max size
#define LED_BRIGTNESS 40 // brightness 0..255
//#define STRIP_PIN 2  // D2 = GPIO 4
#define STRIP_PIN 2


int arr[NUM_LEDS];

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, STRIP_PIN, NEO_GRB + NEO_KHZ800);
bool newMessageInQ = false; 
bool ledsOn = false;

const char* ssid = "Paris";
const char* password = "ps97889600P";
const char* mqtt_server = "192.168.1.101";

WiFiClient espClient;
PubSubClient client(espClient);


void setup() {

  Serial.begin(115200);

  // Инициализация ленты
  strip.begin();
  strip.setBrightness(50);  // Яркость от 0 до 255
  strip.show();  // Очищаем ленту

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  client.setBufferSize(MESSAGE_BUFFER_SIZE);

  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      client.subscribe("test/topic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }

}



void loop() {

  yield();

  if (!client.connected()) {
    reconnect();
  }

  client.loop();

  if( newMessageInQ ){
    newMessageInQ = false;


  for (int i = 0; i < NUM_LEDS; i++)  {
    strip.setPixelColor(i, strip.ColorHSV(arr[i] * 255, 254, LED_BRIGTNESS));
    if (i % 20 == 0) {
      yield();
    }
  }

  strip.show();


  }
}



void callback(char* topic, byte* payload, unsigned int length) {
  
  newMessageInQ = true;
//  parseCSV( (char*)payload, length );
  int size = min( NUM_LEDS, (int)length );
  for( int i = 0; i < size; i++) {
    arr[i] = payload[i];
  }

} 



void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      client.subscribe("test/topic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}



// Функция для конвертации CSV строки в массив целых чисел global int arr[NUM_LEDS]
// проходит payload один раз и использует два указателя:  i - текущая позиция в payload, j - текущая позиция в arr
void parseCSV(const char* payload, unsigned int length ) {

  int j = 0;
  int i = 0;
  String num_str = "";

  Serial.printf("got new message, length %d \n", length);

  while( i < length and j < NUM_LEDS ){
    if( payload[i] >= '0' and payload[i] <= '9' ){
      num_str = num_str + payload[i];
    }
    else{
      if( num_str.length() > 0 ){
        arr[j] = num_str.toInt();

        Serial.print( arr[j] );
        Serial.print( " ");

        j++;
        num_str = "";
      }
    }
    i++;
    if(i % 50 == 0) {  
      yield();         
    }
  }
  if( num_str.length() > 0 and j < NUM_LEDS ){
    arr[j] = num_str.toInt();
  }
  
  Serial.println(" all ");

}

