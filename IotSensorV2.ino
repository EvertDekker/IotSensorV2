/*
    Gerneric Esp8266 Module: Flash Mode: QIO, Flash Frequency: 40MHz, CPU Frequency: 80MHz
    Flash Size: 4M (1M SPIFFS), Debug Port: Disabled, Debug Level: None,  Reset Method: ck
*/
/*
2.1 20180409  Added ESP.Restart to wifi connection failed, deepsleep didn't work here in Setup()
2.2 20180422  Changed wifi connection flow from MODE, BEGIN, CONFIG to MODE, CONFIG, BEGIN
2.3 20180425  Core role dback from 2.4.1 to 2.3.0  (Arduino Core 2.4.1 gave a lot of problems with wifi connection) 
*/

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <MQTTClient.h>  //https://github.com/256dpi/arduino-mqtt
#include <BME280I2C.h>   //https://github.com/finitespace/BME280
#include <Tsl2561Util.h> //https://github.com/joba-1/Joba_Tsl2561
#include <Wire.h>

//System
#define HOSTNAME "TemperatureV2"
#define JOSHUANODE 112

//Wifi
#define WIFI_SSID "YourSSID"
#define WIFI_PASSW "SecretPsw"

//network   NOTE: must be comma delimited
#define MY_IP 192,168,1,89
#define GATEWAY 192,168,1,254
#define NETMASK 255,255,255,0

//Mqtt
#define MQTT_IP "192.168.1.127"
#define MQTT_LOGIN "MqttLogin"
#define MQTT_PASSW "Password"
#define MQTT_ROOT_TOPIC "/livingroom/tempnodev2/"

//Ota
#define OTA_PASSW "Welkom01"

//===========================================
const char* vrs = "2.3"; // Evert Dekker 2017
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSW;
const char* hostn = HOSTNAME;

IPAddress ip(MY_IP);
IPAddress gateway(GATEWAY);
IPAddress subnet(NETMASK);

//Set the debug output, only one at the time
#define NONE true
//#define SERIAL true

#ifdef  NONE
#define DEBUG_PRINTF(x,...)
#define DEBUG_PRINTLN(x)
#define DEBUG_PRINT(x)
#elif SERIAL
#define DEBUG_PRINTF(x,...) Serial.printf(x, ##__VA_ARGS__)
#define DEBUG_PRINTLN(x)    Serial.println(x)
#define DEBUG_PRINT(x)      Serial.print(x)
#endif


// Config
#define LEDPIN 13 // Led pin
#define VOLTDIVIDER 4.39 //

// declaration
void connect(); // <- predefine connect() for setup Mqtt()

// Lib instance
WiFiClient net;
MQTTClient client;
BME280I2C::Settings settings(
  BME280::OSR_X1,
  BME280::OSR_X1,
  BME280::OSR_X1,
  BME280::Mode_Normal,
  BME280::StandbyTime_500us,
  BME280::Filter_16,
  BME280::SpiEnable_False,
  0x76 // I2C address. I2C specific.
);
BME280I2C bme(settings);
Tsl2561 Tsl(Wire);
uint8_t id;


// globals
unsigned long startMills;
float humidity = 0;
float temperature = 0;
float pressure = 0;
int batvolt = 0;
int nodestatus;
unsigned long lastMillis = 0;
float lux = 0;
boolean good;
bool gain;
byte expos;
const String Gain[2] = {"1x", "16x"};
const String Integration[4] = {"13.7ms", "101ms", "402ms", "manual"};

void setup() {

  pinMode(LEDPIN, OUTPUT);
  digitalWrite(LEDPIN, HIGH);
  Serial.begin(115200);
  Serial.println("Booting");

  WiFi.mode(WIFI_STA);
  WiFi.hostname(hostn);
  WiFi.config(ip, gateway, subnet);
  WiFi.begin(ssid, password);
  
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    DEBUG_PRINTLN("Connection Failed! Rebooting...");
    blinkledlong(1); //1 long blink: failed wifi connected
   delay(5000);
   ESP.restart();
  }
  blinkledshort(1); //1 fast blink: wifi connected
  ArduinoOTA.setHostname(hostn);   // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setPassword((const char *)OTA_PASSW);

  ArduinoOTA.onStart([]() {
    DEBUG_PRINTLN("OTA Start");
  });

  ArduinoOTA.onEnd([]() {
    DEBUG_PRINTLN("\nOTA End");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    DEBUG_PRINTF("Progress: %u%%\r", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    DEBUG_PRINTF("OTA Error[%u]: ", error);
    blinkledlong(3); //3 long blink: Ota Failed
    if (error == OTA_AUTH_ERROR) DEBUG_PRINTLN("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) DEBUG_PRINTLN("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) DEBUG_PRINTLN("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) DEBUG_PRINTLN("Receive Failed");
    else if (error == OTA_END_ERROR) DEBUG_PRINTLN("End Failed");
  });

  ArduinoOTA.begin();
  DEBUG_PRINTLN("Ready");
  DEBUG_PRINT("IP address: ");
  DEBUG_PRINTLN(WiFi.localIP());
  DEBUG_PRINT("Hostname: ");
  DEBUG_PRINTLN(WiFi.hostname());
  client.begin(MQTT_IP, net);
  client.onMessage(messageReceived);
  connect();

  Wire.begin();
  bme280init();
  while ( !Tsl.begin() ); // wait until chip detected or wdt reset
  DEBUG_PRINTLN("\nStarting Tsl2561Util autogain loop");
 Tsl.on();
 Tsl.id(id);
}

void loop() {
  ArduinoOTA.handle();
  delay(10); // <- fixes some issues with WiFi stability

  if (!client.connected()) {
    DEBUG_PRINTLN("\Retry connecting Mqtt");
    connect();
  }
  blinkledshort(2); //2 fast blink: Mqtt connected
  client.loop();

  if (nodestatus == 15) {  //You can change status with mqtt broker to not go in deep sleep
    DEBUG_PRINT("Not going to sleep");
    client.publish(MQTT_ROOT_TOPIC "status", String(nodestatus), true, 0);
    for (int i = 1; i < 30; i++) { //waiting 30 seconds for OTA
      blinkledlong(1);
      DEBUG_PRINTLN("Waiting for OTA");
      ArduinoOTA.handle();
    }
    DEBUG_PRINTLN("No OTA received, back to normal");
    nodestatus = 0;
  }
  else
  {
    acquiresensor();
    client.publish(MQTT_ROOT_TOPIC "nodeid", String(JOSHUANODE), true, 0);
    client.publish(MQTT_ROOT_TOPIC "versie", String(vrs), true, 0);
    client.publish(MQTT_ROOT_TOPIC "temperatuur", String(temperature, 2), true, 0);
    client.publish(MQTT_ROOT_TOPIC "luchtVochtigheid", String(humidity, 1), true, 0);
    client.publish(MQTT_ROOT_TOPIC "luchtdruk", String(pressure, 2), true, 0);
    client.publish(MQTT_ROOT_TOPIC "gain", String(gain));
    client.publish(MQTT_ROOT_TOPIC "exposure", String(expos));
    client.publish(MQTT_ROOT_TOPIC "lux", String(lux));
    client.publish(MQTT_ROOT_TOPIC "good", String(good));
    client.publish(MQTT_ROOT_TOPIC "batterij", String(batvolt), true, 0);
    client.publish(MQTT_ROOT_TOPIC "status", String(nodestatus), true, 0);
    client.loop();
    delay(200);
    settings.mode = BME280::Mode_Sleep; //Put the BM280 sensor in sleep mode
    bme.setSettings(settings);
    Tsl.off();                     //Shutdown the light sensor
    delay(100);                     //Wait until the sensor are sleepie 
    ESP.deepSleep(120 * 1000000);   // deepSleep time is defined in microseconds. Multiply seconds by 1e6
    delay(5000);
  }
}




void connect() {
  DEBUG_PRINT("Mqtt checking wifi...");
  while (WiFi.status() != WL_CONNECTED) {
    DEBUG_PRINT(".");
    blinkledlong(1); //1 long blink: failed wifi connected
    delay(60000);
    ESP.restart();
  }

  DEBUG_PRINT("\nMqtt connecting...");
  while (!client.connect(HOSTNAME, MQTT_LOGIN , MQTT_PASSW)) {
    DEBUG_PRINT(".");
    blinkledlong(2); //2 Long blink: Mqtt failed to connect
    delay(1000);
    // ArduinoOTA.handle();
  }
  DEBUG_PRINTLN("\nMqtt connected!");
  client.subscribe(MQTT_ROOT_TOPIC "status");
}


void messageReceived(String &topic, String &payload) {
  nodestatus = payload.toInt();
  DEBUG_PRINT("incoming: ");
  DEBUG_PRINT(topic);
  DEBUG_PRINT(" ");
  DEBUG_PRINT(payload);
  DEBUG_PRINTLN();
}

void acquiresensor() {
  aquiretsl2561data();
  acquireBME280Data();
  batvolt = analogRead(A0) * VOLTDIVIDER;
  DEBUG_PRINT("Battery (mV): ");
  DEBUG_PRINTLN(batvolt);
}


void blinkledshort(byte blinks) {
  for (int i = 1; i <= blinks; i++) {
    digitalWrite(LEDPIN, LOW);
    delay(50);
    digitalWrite(LEDPIN, HIGH);
    delay(100);
  }
}

void blinkledlong(byte blinks) {
  for (int i = 1; i <= blinks; i++) {
    digitalWrite(LEDPIN, LOW);
    delay(500);
    digitalWrite(LEDPIN, HIGH);
    delay(500);
  }
}



