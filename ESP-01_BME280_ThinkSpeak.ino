// ESP-01 with BME-280 via MQTT with deep sleep
// Based on https://github.com/akkerman/espthp

#include <ESP8266WiFi.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include "config.h"
#include "ThingSpeak.h"

#define STATUS_DISCONNECTED "disconnected"
#define STATUS_ONLINE "online"
#define DEEP_SLEEP_S 30  // 900 s = 15 min.
#define BME280ON  // Comment to disable BME280 sensor readout.
#define DEBUG  // Comment to disable debug serial output.
#ifdef DEBUG
  #define DPRINT(...)    Serial.print(__VA_ARGS__)
  #define DPRINTLN(...)  Serial.println(__VA_ARGS__)
#else
  #define DPRINT(...)
  #define DPRINTLN(...)
#endif

ADC_MODE(ADC_VCC);

unsigned long previousMillis = 0;
const long interval = 10 * 60 * 1000;
const String chipId = String(ESP.getChipId());
const String hostName = "ESP-01_" + chipId;
const String baseTopic = "raw/" + chipId + "/";
const String tempTopic = baseTopic + "temperature";
const String humiTopic = baseTopic + "humidity";
const String presTopic = baseTopic + "pressure";
const String willTopic = baseTopic + "status";
const String vccTopic  = baseTopic + "vcc";
const String ipTopic   = baseTopic + "ip";
IPAddress ip;
WiFiClient WiFiClient;

#ifdef BME280ON
  Adafruit_BME280 bme; // I2C
#endif

char temperature[6];
char humidity[6];
char pressure[7];
char vcc[10];
void setup() {
#ifdef DEBUG
  Serial.begin(9600);
#else
  pinMode(3, OUTPUT);  // Power up BME280 when powered via RX (GPIO3).
  digitalWrite(3, HIGH);
#endif
  delay(10);
#ifdef BME280ON
  Wire.begin(0, 2);
  Wire.setClock(100000);
  if (!bme.begin(0x76)) {
    DPRINTLN("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }
#endif
  DPRINTLN();
  DPRINT("Connecting to ");
  DPRINTLN(WIFI_SSID);
  WiFi.mode(WIFI_STA);
  ThingSpeak.begin(WiFiClient);
  WiFi.hostname(hostName);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    DPRINT(".");
  }
  DPRINTLN();
  DPRINTLN("WiFi connected");
  DPRINT("IP address: ");
  ip = WiFi.localIP();
  DPRINTLN(ip);
  
  yield();
  bmeRead();
  vccRead();
  printMeasurements();
  thingSpeakSend();
#ifndef DEBUG
  digitalWrite(3, LOW);
#endif
  delay(500); // Needed to properly close connection.
  WiFi.disconnect();
  DPRINTLN("Going to deep sleep...");
  ESP.deepSleep(DEEP_SLEEP_S * 1000 * 1000);
}

void loop() {
}

void bmeRead() {
#ifdef BME280ON
  float t = bme.readTemperature();
  float h = bme.readHumidity();
  float p = bme.readPressure() / 100.0F;
#else
  float t = 20 + random(5);
  float h = 50 + random(10);
  float p = 1013 + random(10);
#endif
  sprintf(temperature, "%.1f", t);
  sprintf(humidity, "%.1f", h);
  sprintf(pressure, "%.1f", p);
}

void printMeasurements() {
  DPRINT("t,h,p: ");
  DPRINT(temperature); DPRINT(", ");
  DPRINT(humidity);    DPRINT(", ");
  DPRINT(pressure);    DPRINTLN();
  DPRINT("vcc:   ");   DPRINTLN(vcc);
}

void vccRead() {
  float v  = ESP.getVcc() / 1000.0;
  sprintf(vcc, "%.2f", v);
}

void thingSpeakSend() {
  ThingSpeak.setField(1, temperature);
  ThingSpeak.setField(2, humidity);
  ThingSpeak.setField(3, pressure);
  ThingSpeak.setField(4, vcc);
  ThingSpeak.setStatus("Done");
  int res = ThingSpeak.writeFields(TS_CHANNEL_ID, TS_API_KEY);
  if (res == 200) {
    DPRINTLN("ThingSpeak: data odoslane");
  } else {
    DPRINTLN("ThingSpeak: ERROR");
  }
  // TODO: check res == 4*200
  
}
