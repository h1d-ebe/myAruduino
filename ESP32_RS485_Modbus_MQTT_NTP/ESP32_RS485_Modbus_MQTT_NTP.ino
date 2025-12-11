#include <WiFi.h>
#include <MQTT.h> // MQTT by joel Gaehwiler 2.5.2
#include <ModbusMaster.h> // ModbusMaster by Doc Walker 2.0.1
#include <HardwareSerial.h> // MAX_RS485 by Victor Arzoz 1.1.0
#include <NTPClient.h> // NTPClient by Fabrice Weinberg 3.2.1
#include <WiFiUdp.h>

// KM-N1 Holding Register Address Map
// 0x0000 電圧1[V] 電圧の10倍値
// 0x0002 電圧2[V] 電圧の10倍値
// 0x0004 電圧3[V] 電圧の10倍値
// 0x0006 電流1[A] 電流の1000倍値
// 0x0008 電流2[A] 電流の1000倍値
// 0x000A 電流3[A] 電流の1000倍値
// 0x000C 力率 力率の100倍値
// 0x000E 周波数[Hz] 周波数の10倍値
// 0x0010 有効電力[W] 有効電力の10倍値 effective_electric_power
// 0x0012 無効電力[Var] 無効電力の10倍値 reactive_electric_power
// 0x0200 積算有効電力量[Wh]  integrated_effective_electric_power
// 0x0202 積算回生電力量[Wh]  integrated_regenerated_electric_power
// 0x0204 積算進み無効電力量[Wh]  integrated_leading_reactive_electric_power
// 0x0206 積算遅れ無効電力量[Wh]  integrated_lagging_reactive_electric_power
// 0x0208 積算総合無効電力量[Wh] 
// 0x0220 積算有効電力量[kWh] 
// 0x0222 積算回生電力量[kWh] 
// 0x0224 積算進み無効電力量[kVarh] 
// 0x0226 積算遅れ無効電力量[kVarh] 
// 0x0228 積算総合無効電力量[kVarh] 
// 0x0300 換算値[JPY] 
// 0x0302 換算値[K.JPY]

// Interval設定
const int interval_sec = 60; //60 sec

//日付をまたいだらKM-N1積算値クリア用
// int last_hours=23; // 初回起動時は積算値がリセットされる

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
const char* mqtt_topic  = "async-mqtt/esp32epm";
const char* mqtt_user  = "tocos";
const char* mqtt_pass  = "tocos2023";

// --- NTP設定 ---
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 9*3600, 60000); // JST(UTC+9), 60秒更新

// --- Modbus設定 ---
#define RXD2 16   // UART2 RX
#define TXD2 17   // UART2 TX
#define RS485_DE_RE  18  // DE/REピン
HardwareSerial RS485Serial(2);
ModbusMaster node;

// --- MQTTオブジェクト ---
WiFiClient net; // 修正: WiFiClientオブジェクトを用意
MQTTClient mqttClient(256);

// --- センサー設定 ---
//const int sensorID = 1;
const int ModbusID = 1;// Modbus ID
const uint8_t SENSOR_NUM = 2;
const uint8_t SENSOR_ADR [SENSOR_NUM]={1,2};
const uint8_t SENSOR_ID = 1;  // ModbusデバイスID(アドレス)
const uint16_t SENSOR_REG = 0x0010; // 読み取るレジスタアドレス

void preTransmission()  { digitalWrite(RS485_DE_RE, HIGH); }
void postTransmission() { digitalWrite(RS485_DE_RE, LOW);  }

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
                       // (積算電力は23時に接続失敗が始まった時以外は日付をまたいでもリセットされない)
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    // MQTTサーバーへ接続
    //if (mqttClient.connect("clientID")) {
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

  // RS485初期化
  pinMode(RS485_DE_RE, OUTPUT);
  digitalWrite(RS485_DE_RE, LOW);
  //RS485Serial.begin(9600, SERIAL_8N1, RXD2, TXD2);
  //RS485Serial.begin(9600, SERIAL_8N2, RXD2, TXD2);
  RS485Serial.begin(1200, SERIAL_8N2, RXD2, TXD2);
  // node.begin(SENSOR_ADR[0], RS485Serial);
  // node.preTransmission(preTransmission);
  // node.postTransmission(postTransmission);

  // WiFi接続
  connectWiFi();

  // NTP開始
  timeClient.begin();

  // MQTT接続
  mqttClient.begin(mqtt_server, mqtt_port, net);
  /*  while (!mqttClient.connect("ESP32Client")) {
    delay(500); Serial.print("#");
  }
  Serial.println("\nMQTT connected");*/
  reconnectMQTT();
}

void loop() {
  float effectpow;
  float reactpow;
  uint32_t ieep;
  uint32_t irep;
  uint32_t ileadep;
  uint32_t ilagep;
  // NTP時刻取得
  timeClient.update();
  //String iso8601 = "ISODate(\""+getISOTimestamp(timeClient)+"\")";
  String iso8601 = "\""+getISOTimestamp(timeClient)+"\"";
  int hours = timeClient.getHours();

  for(int i=0;i<SENSOR_NUM;i++){
    node.begin(SENSOR_ADR[i], RS485Serial);
    node.preTransmission(preTransmission);
    node.postTransmission(postTransmission);
    // Modbusでセンサーデータ取得
    // 0x0010 有効電力[W] 有効電力の10倍値 effective_electric_power
    // 0x0012 無効電力[Var] 無効電力の10倍値 reactive_electric_power
    uint8_t result = node.readHoldingRegisters(0x0010, 4); // 4レジスタ例
    if (result == node.ku8MBSuccess) {
      effectpow=(node.getResponseBuffer(0x0)*0x10000+node.getResponseBuffer(0x1))/10.0f;
      reactpow=(node.getResponseBuffer(0x2)*0x10000+node.getResponseBuffer(0x3))/10.0f;
  // 0x0200 積算有効電力量[Wh]  integrated_effective_electric_power
  // 0x0202 積算回生電力量[Wh]  integrated_regenerated_electric_power
  // 0x0204 積算進み無効電力量[Wh]  integrated_leading_reactive_electric_power
  // 0x0206 積算遅れ無効電力量[Wh]  integrated_lagging_reactive_electric_power
      delay(100);
      result = node.readHoldingRegisters(0x0200, 8); // 8レジスタ
      if (result == node.ku8MBSuccess) {
        ieep=node.getResponseBuffer(0x0)*0x10000+node.getResponseBuffer(0x1);
        irep=node.getResponseBuffer(0x2)*0x10000+node.getResponseBuffer(0x3);
        ileadep=node.getResponseBuffer(0x4)*0x10000+node.getResponseBuffer(0x5);
        ilagep=node.getResponseBuffer(0x6)*0x10000+node.getResponseBuffer(0x7);
    
        // JSON形式でMQTT送信
        String payload = "{";
        payload += "\"recorded\" : " + iso8601;
        payload += ", \"sensor_id\" : " + String(SENSOR_ADR[i]); // modbus address
        payload += ", \"channel_id\" : " + String(ModbusID); // modbus id
        payload += ", \"effec_ep\" : " + String(effectpow, 3);
        payload += ", \"reac_ep\":" + String(reactpow, 3);
        payload += ", \"intg_effec_ep\":" + String(ieep);
        payload += ", \"intg_regenerated_ep\":" + String(irep);
        payload += ", \"intg_leading_reac_ep\":" + String(ileadep);
        payload += ", \"intg_lagging_reac_ep\":" + String(ilagep);
        payload += ", \"migrated\" : false }";

        // // 日付をまたいでいたら積算値をクリア
        // if(last_hours==23 && hours!=23){
        //   // 積算値クリア
        //   node.writeSingleRegister(0xffff,0x0300); // UNIT_IDに対応する積算値クリア
        //   Serial.println("KM-N1 integral values cleard.");
        // }
        // last_hours=hours;

        // Wifi再接続処理
        connectWiFi();
        // MQTT再接続処理
        reconnectMQTT();
        mqttClient.publish(mqtt_topic, payload);
        Serial.println(payload);
      } else {
        Serial.print("Modbus error2: "); Serial.println(result);
      }
    } else {
      Serial.print("Modbus error1: "); Serial.println(result);
    }
  }

  mqttClient.disconnect();
  Serial.println("MQTT disconnected intentionally.");
  //disconnectWiFi();
  Serial.println("Wifi disconnected intentionally.");
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