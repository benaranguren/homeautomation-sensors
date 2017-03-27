#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <PubSubClient.h>
#include <WiFiUdp.h>

#ifdef HAS_BMP
#include <Adafruit_BMP085_U.h>
#include <Adafruit_Sensor.h>
#endif

#ifdef HAS_DHT
#include <DHT.h>
#endif

/* User specific configuration such as WiFi credentials */
#include <user_cfg.h>

#ifdef HAS_RF433_TX
#include <rf433.h>
#endif

#define XSTR(x) #x
#define STR(x) XSTR(x)

#ifndef CLIENT_ID
#error "-DCLIENT_ID=xxxx is required"
#endif

#define MQTT_TOPIC "/home/" STR(CLIENT_ID)
#define STATE_TOPIC MQTT_TOPIC "status"

/* PIN ASSIGNEMENT */
#define PIN_SCL D1 // BMP
#define PIN_SDA D2 // BMP
#define PIN_PIR D3 // PIR
#define PIN_DHT D4 // DHT22
/* RF 433 MHz Transmitter
 * Data Pin = PIN D2
 * VCC = 5V Vin
 * GND = Ground Pin
 */
#define PIN_RFTX D2 // RF 433MHz Transmitter

/* PIR */
#ifdef HAS_PIR
const int PIR_CAL_TIME = 60 * 1000;
int pir_needs_calibration = 1;
int pir_cal_start;
int prev_pir_val = -99;

int pub_pir_val = -1;
char pub_pir_status[20];
#endif

/* BMP 180 */
#ifdef HAS_BMP
Adafruit_BMP085_Unified bmp = Adafruit_BMP085_Unified(10000);
#endif

#ifdef HAS_DHT
DHT dht(PIN_DHT, DHT22);
#endif

#ifdef HAS_DHT || HAS_BMP
float pub_temperature = -1;
float pub_humidity = -1;
float prev_temperature = -99;
float prev_humidity = -99;
#endif

#ifdef HAS_LIGHTSWITCH
// data published by this sensor
int pub_s2_val = -1;
int pub_s1_val = -1;
int toggle_val = 0;
#endif

#ifdef HAS_RF433_TX
unsigned long rf_code;
char buffer[20] = {0};
#endif

/* This needs to be set to 1 to send data to MQTT broker */
int has_changes = 0;

/* MQTT */
WiFiClient wifi_client;
PubSubClient mqtt_client(wifi_client);

void callback(const char* topic, byte* payload, unsigned int length) {
  Serial.print("Message received: [");
  Serial.print(topic);
  Serial.printf("] payload: %s length: %d\n", payload, length);

#ifdef HAS_RF433_TX
  int i;
  for (i = 0; i < length; i++) {
    buffer[i] = payload[i];
  }
  buffer[i] = 0;
  rf_code = atol(buffer);
  Serial.printf("Converted payload %d\n", rf_code);
  has_changes = 1;
#endif
}

void connect_mqtt() {
  while (!mqtt_client.connected()) {
    Serial.print("Connecting to MQTT broker ... ");
    if (mqtt_client.connect(STR(CLIENT_ID))) {
      Serial.println("OK");
      mqtt_client.subscribe(MQTT_TOPIC);
    } else {
      Serial.println("Failed .. retrying in 5 sec");
      delay(5000);
    }
  }
}

/* Connnect to WiFi AP */
void connect_wifi() {

#ifdef NO_WIFI
  return;
#endif

  Serial.printf("Connecting to %s ...", MY_SSID);
  WiFi.begin(MY_SSID, MY_WIFI_PASSWD);
  delay(100);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Failed ... Rebooting in 5 sec");
    delay(5000);
    ESP.restart();
  }

  Serial.print("Connected ... ");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);
  Serial.println("Booting ...");

  delay(100);

  WiFi.mode(WIFI_STA);
  connect_wifi();

  ArduinoOTA.onStart([]() {
    Serial.println("OTA upgrade start");
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("OTA upgrade done");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total){
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if      (error == OTA_AUTH_ERROR   ) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR  ) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR    ) Serial.println("End Failed");
  });

  ArduinoOTA.begin();

  mqtt_client.setServer(mqtt_broker, 1883);
  mqtt_client.setCallback(callback);

#ifdef HAS_PIR
  /* PIR */
  pinMode(PIN_PIR, INPUT_PULLUP);
#endif

#ifdef HAS_BMP
  /* BMP 180 */
  if (!bmp.begin()) {
    Serial.println("BMP 180 not found");
    while (1) {}
  }
  sensor_t sensor;
  bmp.getSensor(&sensor);
  Serial.printf("Sensor: %s\n", sensor.name);
#endif

#ifdef HAS_DHT
  dht.begin();
#endif

#ifdef HAS_LIGHTSWITCH
  pinMode(D7, INPUT);
  pinMode(D6, INPUT);
#endif
}

/* Main MQTT loop
 *
 * Connect to broker if necessary.
 */
void loop_mqtt() {
  // connect to mqtt broker
  if (!mqtt_client.connected()) {
    connect_mqtt();
  } else {
    mqtt_client.loop();
  }
}

/* PIR device loop
 *
 * Read PIR status.  When the device first boots up, this routine will perform
 * calibration first before reading the sensor.
 */
void loop_pir() {
#ifdef HAS_PIR
  if (pir_needs_calibration) {
    if (pir_cal_start == -1) {
      Serial.print("PIR calibrating");
      pir_cal_start = millis();
    }
    sprintf(pub_pir_status, "%s: %d", "Calibrating",
            millis() - pir_cal_start);
    has_changes = 1;
    Serial.print(".");
    if (millis() - pir_cal_start > PIR_CAL_TIME) {
      pir_needs_calibration = 0;
      Serial.println(" Done");
      sprintf(pub_pir_status, "%s", "Active");
    }
  } else {
    pub_pir_val = digitalRead(PIN_PIR);
    if (prev_pir_val != pub_pir_val) {
      has_changes = 1;
    }
    prev_pir_val = pub_pir_val;
  }
#endif
}

void loop_bmp() {
#ifdef HAS_BMP
  sensors_event_t event;
  bmp.getEvent(&event);

  if (event.pressure) {
    bmp.getTemperature(&pub_temperature);
  } else {
    Serial.println("Sensor error");
  }

  if (abs(prev_temperature - pub_temperature) > 1.5) {
    has_changes = 1;
  }
#endif
}

void loop_dht() {
#ifdef HAS_DHT
  pub_humidity = dht.readHumidity();
  pub_temperature = dht.readTemperature();

  if (isnan(pub_humidity) || isnan(pub_temperature)) {
    Serial.println("ERROR reading from DHT");
  }

  if (abs(prev_temperature - pub_temperature) > 1.5) {
    has_changes = 1;
  } else if (abs(prev_humidity - pub_humidity) > 1.5) {
    has_changes = 1;
  }

  prev_temperature = pub_temperature;
  prev_humidity = pub_humidity;
#endif
}

void loop_lightswitch() {
#ifdef HAS_LIGHTSWITCH
  int tmp;

  tmp = digitalRead(D6);
  if (pub_s1_val ^ tmp) {
    has_changes = 1;
  }
  pub_s1_val = tmp;

  tmp = digitalRead(D7);
  //if (pub_s2_val ^ tmp) {
  //  has_changes = 1;
  //}
  pub_s2_val = tmp;

  if (has_changes) {
    toggle_val ^= 1;
  }
#endif
}

/* Send JSON data to MQTT broker
 *
 * The contents of the JSON data depends on the funionality enabled.
 */
void publish_data() {
  StaticJsonBuffer<200> json_buffer;
  char json_data[200];
  JsonObject& root = json_buffer.createObject();

#ifdef HAS_LIGHTSWITCH
  root["toggle"] = toggle_val;
  root["s1"] = pub_s1_val;
  root["s2"] = pub_s2_val;
#endif

#ifdef HAS_PIR
  root["pir_val"] = pub_pir_val;
  root["status"] = pub_pir_status;
#endif

#ifdef HAS_DHT || HAS_BMP
  root["temperature"] = pub_temperature * 1.8 + 32;
  root["humidity"] = pub_humidity;
#endif

  if (has_changes) {
    if (mqtt_client.connected()) {
      root.printTo(json_data, root.measureLength() + 1);
      mqtt_client.publish(STATE_TOPIC, json_data, true);
    }
    root.prettyPrintTo(Serial);
    Serial.println();
  }
}

/* Transmit data to RF receivers */
void loop_433mhz() {
#ifdef HAS_RF433_TX
  #define PULSE_WIDTH 183
  #define DATA_LEN 24
  int retries = 10;
  for (int i = 0; i < retries; i++) {
    rf_write_code(PIN_RFTX, PULSE_WIDTH, rf_code, DATA_LEN);
  }
#endif
}

/* Main loop */
void loop() {

  ArduinoOTA.handle();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Wifi is disconnected!\n");
    connect_wifi();
  }

  has_changes = 0;
  loop_mqtt();
  loop_pir();
  loop_bmp();
  loop_dht();
  loop_lightswitch();
  loop_433mhz();

  publish_data();

  delay(300);
}
