#include <WiFi.h>
#include <MQTT.h> // MQTT by joel Gaehwiler 2.5.2
#include <NTPClient.h> // NTPClient by Fabrice Weinberg 3.2.1
#include <WiFiUdp.h>

#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include "sensor_id.h"

// ADC2関連のピン。Wifi使用中にはADCとして使用不可。 0,2,4,12-15,25-27
#define DHT1PIN 13     // Digital pin connected to the DHT sensor 
// Wifi使用中に問題なく使用可能なデジタルI/Oピン 1,3,5,18-23,26-27
#define DHT2PIN  5     // Digital pin connected to the DHT sensor 
#define DHT3PIN 18     // Digital pin connected to the DHT sensor 
#define DHT4PIN 19     // Digital pin connected to the DHT sensor 
#define DHT5PIN 21     // Digital pin connected to the DHT sensor 
#define DHT6PIN 22     // Digital pin connected to the DHT sensor 
#define DHT7PIN 23     // Digital pin connected to the DHT sensor 
// Feather HUZZAH ESP8266 note: use pins 3, 4, 5, 12, 13 or 14 --
// Pin 15 can work but DHT must be disconnected during program upload.

// Uncomment the type of sensor in use:
#define DHTTYPE    DHT11     // DHT 11
//#define DHTTYPE    DHT22     // DHT 22 (AM2302)
//#define DHTTYPE    DHT21     // DHT 21 (AM2301)

// See guide for details on sensor wiring and usage:
//   https://learn.adafruit.com/dht/overview

DHT_Unified dht1(DHT1PIN, DHTTYPE);
DHT_Unified dht2(DHT2PIN, DHTTYPE);
DHT_Unified dht3(DHT3PIN, DHTTYPE);
DHT_Unified dht4(DHT4PIN, DHTTYPE);
DHT_Unified dht5(DHT5PIN, DHTTYPE);
DHT_Unified dht6(DHT6PIN, DHTTYPE);
DHT_Unified dht7(DHT7PIN, DHTTYPE);

bool dht1_en=true;
bool dht2_en=true;
bool dht3_en=true;
bool dht4_en=true;
bool dht5_en=true;
bool dht6_en=true;
bool dht7_en=true;



// Interval設定
const int interval_sec = 600; //600 sec = 10 min

// --- WiFi設定 ---
const char* ssid     = "TOCOS-2F-08-G"; //"ELITEBOOK_BEE";//SSID:	TOCOS-2F-07-G
const char* password = "Tocos2F082110"; //"tocos2023"; Tocos2F072110

// --- MQTT設定 ---
//const char* mqtt_server = IPAddress(192, 168, 137, 1);
//const char* mqtt_server = "192.168.137.1";
//const char* mqtt_server = IPAddress(106, 73, 70, 128)
//const char* mqtt_server = "106.73.70.128";
const char* mqtt_server = "tocosiot.mydns.jp";
//const char* mqtt_server = "240b:11:4680:a300:dea6:32ff:fe71:250b";
//const int   mqtt_port   = 1883;
const int   mqtt_port   = 6713;
//const char* mqtt_topic  = "async-mqtt/esp32epm";
const char* mqtt_topic  = "async-mqtt/esp32th";
const char* mqtt_user  = "tocos";
const char* mqtt_pass  = "tocos2023";
//const int channelID = 1;

// --- NTP設定 ---
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 9*3600, 60000); // JST(UTC+9), 60秒更新

// --- MQTTオブジェクト ---
WiFiClient net; // 修正: WiFiClientオブジェクトを用意
MQTTClient mqttClient(256);

void connectWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(ssid, password);
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
      delay(500);
      Serial.print(".");
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("WiFi connected.");
    } else {
      Serial.println("WiFi connection failed.");
    }
  }
}

void disconnectWiFi() {
  WiFi.disconnect(true); // trueで省電力も有効化
  Serial.println("WiFi disconnected.");
}

void reconnectMQTT() { // 接続できないときは接続できるまでループし続ける
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    // MQTTサーバーへ接続
    if (mqttClient.connect("clientID",mqtt_user,mqtt_pass)) {
      Serial.println("connected");
      // 必要に応じてsubscribeなど
    } else { 
      Serial.print("failed, rc=");
      Serial.print(mqttClient.lastError());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  // WiFi接続
  connectWiFi();

  // NTP開始
  timeClient.begin();

    // Initialize device.
  if(dht1_en) dht1.begin();
  if(dht2_en) dht2.begin();
  if(dht3_en) dht3.begin();
  if(dht4_en) dht4.begin();
  if(dht5_en) dht5.begin();
  if(dht4_en) dht6.begin();
  if(dht5_en) dht7.begin();

  Serial.println(F("DHTxx Unified Sensor Example"));
  // Print temperature sensor details.
  sensor_t sensor;
  if(dht1_en){
  dht1.temperature().getSensor(&sensor);
  Serial.println(F("------------------------------------"));
  Serial.println(F("Temperature Sensor 1"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("°C"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("°C"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("°C"));
  Serial.println(F("------------------------------------"));
  // Print humidity sensor details.
  dht1.humidity().getSensor(&sensor);
  Serial.println(F("Humidity Sensor 1"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("%"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("%"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("%"));
  Serial.println(F("------------------------------------"));
  }
  if(dht2_en){
  dht2.temperature().getSensor(&sensor);
  Serial.println(F("------------------------------------"));
  Serial.println(F("Temperature Sensor 2"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("°C"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("°C"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("°C"));
  Serial.println(F("------------------------------------"));
  // Print humidity sensor details.
  dht2.humidity().getSensor(&sensor);
  Serial.println(F("Humidity Sensor 2"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("%"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("%"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("%"));
  Serial.println(F("------------------------------------"));
  }
  if(dht3_en){
  dht3.temperature().getSensor(&sensor);
  Serial.println(F("------------------------------------"));
  Serial.println(F("Temperature Sensor 3"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("°C"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("°C"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("°C"));
  Serial.println(F("------------------------------------"));
  // Print humidity sensor details.
  dht3.humidity().getSensor(&sensor);
  Serial.println(F("Humidity Sensor 3"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("%"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("%"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("%"));
  Serial.println(F("------------------------------------"));
  }
  if(dht4_en){
  dht4.temperature().getSensor(&sensor);
  Serial.println(F("------------------------------------"));
  Serial.println(F("Temperature Sensor 4"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("°C"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("°C"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("°C"));
  Serial.println(F("------------------------------------"));
  // Print humidity sensor details.
  dht4.humidity().getSensor(&sensor);
  Serial.println(F("Humidity Sensor 4"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("%"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("%"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("%"));
  Serial.println(F("------------------------------------"));
  }
  if(dht5_en){
  dht5.temperature().getSensor(&sensor);
  Serial.println(F("------------------------------------"));
  Serial.println(F("Temperature Sensor 5"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("°C"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("°C"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("°C"));
  Serial.println(F("------------------------------------"));
  // Print humidity sensor details.
  dht5.humidity().getSensor(&sensor);
  Serial.println(F("Humidity Sensor 5"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("%"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("%"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("%"));
  Serial.println(F("------------------------------------"));
  }
    if(dht6_en){
  dht6.temperature().getSensor(&sensor);
  Serial.println(F("------------------------------------"));
  Serial.println(F("Temperature Sensor 6"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("°C"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("°C"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("°C"));
  Serial.println(F("------------------------------------"));
  // Print humidity sensor details.
  dht6.humidity().getSensor(&sensor);
  Serial.println(F("Humidity Sensor 6"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("%"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("%"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("%"));
  Serial.println(F("------------------------------------"));
  }
  if(dht7_en){
  dht7.temperature().getSensor(&sensor);
  Serial.println(F("------------------------------------"));
  Serial.println(F("Temperature Sensor 7"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("°C"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("°C"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("°C"));
  Serial.println(F("------------------------------------"));
  // Print humidity sensor details.
  dht7.humidity().getSensor(&sensor);
  Serial.println(F("Humidity Sensor 7"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("%"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("%"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("%"));
  Serial.println(F("------------------------------------"));
  }
  // MQTT接続
  mqttClient.begin(mqtt_server, mqtt_port, net);
  //   while (!mqttClient.connect("ESP32Client")) {
  //   delay(500); Serial.print("#");
  // }
  // Serial.println("\nMQTT connected");
  reconnectMQTT();
}

void loop() {
  bool bRead=true;
  float temperature;
  float temperature2;
  float temperature3;
  float temperature4;
  float temperature5;
  float temperature6;
  float temperature7;
  float humidity;
  float humidity2;
  float humidity3;
  float humidity4;
  float humidity5;
  float humidity6;
  float humidity7;

  // NTP時刻取得
  timeClient.update();
  //String iso8601 = "ISODate(\""+getISOTimestamp(timeClient)+"\")";
  String iso8601 = "\""+getISOTimestamp(timeClient)+"\"";
  int hours = timeClient.getHours();

  // Get temperature event and print its value.
  sensors_event_t event;
  if(dht1_en){
    dht1.temperature().getEvent(&event);
    if (isnan(event.temperature)) {
      Serial.println(F("Error reading temperature!"));
      bRead=false;
    }
    else {
      Serial.print(F("Temperature: "));
      Serial.print(event.temperature);
      Serial.println(F("°C"));
      temperature=event.temperature;
    }
    // Get humidity event and print its value.
    dht1.humidity().getEvent(&event);
    if (isnan(event.relative_humidity)) {
      Serial.println(F("Error reading humidity!"));
      bRead=false;
    }
    else {
      Serial.print(F("Humidity: "));
      Serial.print(event.relative_humidity);
      Serial.println(F("%"));
      humidity=event.relative_humidity;
    }
  }

  if(dht2_en){
    dht2.temperature().getEvent(&event);
    if (isnan(event.temperature)) {
      Serial.println(F("Error reading temperature2!"));
      bRead=false;
    }
    else {
      Serial.print(F("Temperature2: "));
      Serial.print(event.temperature);
      Serial.println(F("°C"));
      temperature2=event.temperature;
    }
    // Get humidity event and print its value.
    dht2.humidity().getEvent(&event);
    if (isnan(event.relative_humidity)) {
      Serial.println(F("Error reading humidity2!"));
      bRead=false;
    }
    else {
      Serial.print(F("Humidity2: "));
      Serial.print(event.relative_humidity);
      Serial.println(F("%"));
      humidity2=event.relative_humidity;
    }
  }
  if(dht3_en){
    dht3.temperature().getEvent(&event);
    if (isnan(event.temperature)) {
      Serial.println(F("Error reading temperature3!"));
      bRead=false;
    }
    else {
      Serial.print(F("Temperature3: "));
      Serial.print(event.temperature);
      Serial.println(F("°C"));
      temperature3=event.temperature;
    }
    // Get humidity event and print its value.
    dht3.humidity().getEvent(&event);
    if (isnan(event.relative_humidity)) {
      Serial.println(F("Error reading humidity3!"));
      bRead=false;
    }
    else {
      Serial.print(F("Humidity3: "));
      Serial.print(event.relative_humidity);
      Serial.println(F("%"));
      humidity3=event.relative_humidity;
    }
  }
  if(dht4_en){
    dht4.temperature().getEvent(&event);
    if (isnan(event.temperature)) {
      Serial.println(F("Error reading temperature4!"));
      bRead=false;
    }
    else {
      Serial.print(F("Temperature4: "));
      Serial.print(event.temperature);
      Serial.println(F("°C"));
      temperature4=event.temperature;
    }
    // Get humidity event and print its value.
    dht4.humidity().getEvent(&event);
    if (isnan(event.relative_humidity)) {
      Serial.println(F("Error reading humidity4!"));
      bRead=false;
    }
    else {
      Serial.print(F("Humidity4: "));
      Serial.print(event.relative_humidity);
      Serial.println(F("%"));
      humidity4=event.relative_humidity;
    }
  }
  if(dht5_en){
    dht5.temperature().getEvent(&event);
    if (isnan(event.temperature)) {
      Serial.println(F("Error reading temperature5!"));
      bRead=false;
    }
    else {
      Serial.print(F("Temperature: "));
      Serial.print(event.temperature);
      Serial.println(F("°C"));
      temperature5=event.temperature;
    }
    // Get humidity event and print its value.
    dht5.humidity().getEvent(&event);
    if (isnan(event.relative_humidity)) {
      Serial.println(F("Error reading humidity5!"));
      bRead=false;
    }
    else {
      Serial.print(F("Humidity5: "));
      Serial.print(event.relative_humidity);
      Serial.println(F("%"));
      humidity5=event.relative_humidity;
    }
  }
  if(dht6_en){
    dht6.temperature().getEvent(&event);
    if (isnan(event.temperature)) {
      Serial.println(F("Error reading temperature5!"));
      bRead=false;
    }
    else {
      Serial.print(F("Temperature: "));
      Serial.print(event.temperature);
      Serial.println(F("°C"));
      temperature6=event.temperature;
    }
    // Get humidity event and print its value.
    dht6.humidity().getEvent(&event);
    if (isnan(event.relative_humidity)) {
      Serial.println(F("Error reading humidity6!"));
      bRead=false;
    }
    else {
      Serial.print(F("Humidity6: "));
      Serial.print(event.relative_humidity);
      Serial.println(F("%"));
      humidity6=event.relative_humidity;
    }
  }
    if(dht7_en){
    dht7.temperature().getEvent(&event);
    if (isnan(event.temperature)) {
      Serial.println(F("Error reading temperature5!"));
      bRead=false;
    }
    else {
      Serial.print(F("Temperature: "));
      Serial.print(event.temperature);
      Serial.println(F("°C"));
      temperature7=event.temperature;
    }
    // Get humidity event and print its value.
    dht7.humidity().getEvent(&event);
    if (isnan(event.relative_humidity)) {
      Serial.println(F("Error reading humidity5!"));
      bRead=false;
    }
    else {
      Serial.print(F("Humidity5: "));
      Serial.print(event.relative_humidity);
      Serial.println(F("%"));
      humidity7=event.relative_humidity;
    }
  }
  if (bRead) { // 読出し成功
      // JSON形式でMQTT送信
      String payload;
      if(dht1_en){
        payload = "{";
        payload += "\"recorded\" : " + iso8601;
        payload += ", \"sensor_id\" : " + String(sensorID1);
        payload += ", \"temperature\" : " + String(temperature, 3);
        payload += ", \"humidity\":" + String(humidity, 3);
        payload += ", \"migrated\" : false }";
        // Wifi再接続処理
        connectWiFi();
        // MQTT再接続処理
        reconnectMQTT();
        mqttClient.publish(mqtt_topic, payload);
        Serial.println(payload);
      }
      if(dht2_en){
        payload = "{";
        payload += "\"recorded\" : " + iso8601;
        payload += ", \"sensor_id\" : " + String(sensorID2);
        payload += ", \"temperature\" : " + String(temperature2, 3);
        payload += ", \"humidity\":" + String(humidity2, 3);
        payload += ", \"migrated\" : false }";
        // Wifi再接続処理
        connectWiFi();
        // MQTT再接続処理
        reconnectMQTT();
        mqttClient.publish(mqtt_topic, payload);
        Serial.println(payload);
      }
      if(dht3_en){
        payload = "{";
        payload += "\"recorded\" : " + iso8601;
        payload += ", \"sensor_id\" : " + String(sensorID3);
        payload += ", \"temperature\" : " + String(temperature3, 3);
        payload += ", \"humidity\":" + String(humidity3, 3);
        payload += ", \"migrated\" : false }";
        // Wifi再接続処理
        connectWiFi();
        // MQTT再接続処理
        reconnectMQTT();
        mqttClient.publish(mqtt_topic, payload);
        Serial.println(payload);
      }
      if(dht4_en){
        payload = "{";
        payload += "\"recorded\" : " + iso8601;
        payload += ", \"sensor_id\" : " + String(sensorID4);
        payload += ", \"temperature\" : " + String(temperature4, 3);
        payload += ", \"humidity\":" + String(humidity4, 3);
        payload += ", \"migrated\" : false }";
        // Wifi再接続処理
        connectWiFi();
        // MQTT再接続処理
        reconnectMQTT();
        mqttClient.publish(mqtt_topic, payload);
        Serial.println(payload);
      }
      if(dht5_en){
        payload = "{";
        payload += "\"recorded\" : " + iso8601;
        payload += ", \"sensor_id\" : " + String(sensorID5);
        payload += ", \"temperature\" : " + String(temperature5, 3);
        payload += ", \"humidity\":" + String(humidity5, 3);
        payload += ", \"migrated\" : false }";
        // Wifi再接続処理
        connectWiFi();
        // MQTT再接続処理
        reconnectMQTT();
        mqttClient.publish(mqtt_topic, payload);
        Serial.println(payload);
      }
      if(dht6_en){
        payload = "{";
        payload += "\"recorded\" : " + iso8601;
        payload += ", \"sensor_id\" : " + String(sensorID6);
        payload += ", \"temperature\" : " + String(temperature6, 3);
        payload += ", \"humidity\":" + String(humidity6, 3);
        payload += ", \"migrated\" : false }";
        // Wifi再接続処理
        connectWiFi();
        // MQTT再接続処理
        reconnectMQTT();
        mqttClient.publish(mqtt_topic, payload);
        Serial.println(payload);
      }
      if(dht7_en){
        payload = "{";
        payload += "\"recorded\" : " + iso8601;
        payload += ", \"sensor_id\" : " + String(sensorID7);
        payload += ", \"temperature\" : " + String(temperature7, 3);
        payload += ", \"humidity\":" + String(humidity7, 3);
        payload += ", \"migrated\" : false }";
        // Wifi再接続処理
        connectWiFi();
        // MQTT再接続処理
        reconnectMQTT();
        mqttClient.publish(mqtt_topic, payload);
        Serial.println(payload);
      }
    mqttClient.disconnect();
    Serial.println("MQTT disconnected intentionally.");
    //disconnectWiFi();
    //Serial.println("Wifi disconnected intentionally.");
  }
  for (int i=0;i<interval_sec;i++) delay(1000); // interval_sec秒間隔
}

String getISOTimestamp(NTPClient &client) {
  time_t rawTime = client.getEpochTime();
  struct tm* ti = gmtime(&rawTime);
  char buf[25];
  sprintf(buf, "%04d-%02d-%02dT%02d:%02d:%02dZ",
          ti->tm_year + 1900, ti->tm_mon + 1, ti->tm_mday,
          ti->tm_hour, ti->tm_min, ti->tm_sec);
  return String(buf);
}