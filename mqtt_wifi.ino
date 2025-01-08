#include <ESP8266WiFi.h> 
#include <PubSubClient.h>
//#define FASTLED_ESP8266_NODEMCU_PIN_ORDER
#include <FastLED.h>

//  i choose board = Generic ESP8266 Module

#define NUM_LEDS 10
//#define STRIP_PIN 2  // D2 = GPIO 4
#define STRIP_PIN 4

CRGB leds[NUM_LEDS];
int arr[NUM_LEDS];

const char* ssid = "LENIVEC";
const char* password = "z9c3-39yw-84r1";
const char* mqtt_server = "192.168.1.100";

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {

  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

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

  FastLED.addLeds<WS2811, STRIP_PIN, GRB>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(50);
 // pinMode(13, OUTPUT);

}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  parseCSV( (char*)payload, length );

  for (int i = 0; i < NUM_LEDS; i++) {
    Serial.println(arr[i]);
  }
  for (int i = 0; i < NUM_LEDS; i++) leds[i] = CHSV( arr[i], 255, 255 ) ;
  FastLED.show();
  for (int i = 0; i < NUM_LEDS; i++) arr[i] = 0;

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
void parseCSV(const char* payload, unsigned int length ) {
  // Преобразуем payload в строку
  String data = String(payload);
  Serial.println( "inside function parseCSV");
  Serial.println( data );
  Serial.print("length = ");
  Serial.println(length);


  int j = 0;
  int i = 0;
  String num_str = "";
  while( i < length and j < NUM_LEDS ){
    if( payload[i] >= '0' and payload[i] <= '9' ){
      num_str = num_str + payload[i];
    }
    else{
      if( num_str.length() > 0 ){
        arr[j] = num_str.toInt();
        j++;
        num_str = "";
      }
    }
    i++;
  }
  if( num_str.length() > 0 and j < NUM_LEDS ){
    arr[j] = num_str.toInt();
  }
  

}

