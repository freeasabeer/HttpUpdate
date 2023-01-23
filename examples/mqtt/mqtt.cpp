#include <Arduino.h>
#include "httpupdate.h"
#include "MQTT.h" // https://github.com/256dpi/arduino-mqtt
#define WIFI_SSID "MyOwnSSID"
#define WIFI_KEY "MyOwnKey"
#define MQTT_SERVER "192.168.1.20"
#define MQTT_PORT 1883

MQTTClient mqtt;
WiFiClient _mqttclient;
String otaurl = String("");
const char *OTA_UPDATE_TOPIC = "my-device/ota";

void display_ota_ip(const char *ip);
void OnMessageCB(String &topic, String &payload);

void setup() {
  Serial.begin(115200);
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Wi-Fi connected");

  mqtt.begin(MQTT_SERVER, MQTT_PORT, _mqttclient);
  mqtt.onMessage(OnMessageCB);

  Serial.println("Connecting to MQTT...");
  while (!mqtt.connect("my-device")) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("MQTT connected");

  mqtt.subscribe(OTA_UPDATE_TOPIC, 1);
  Serial.printf("Topic %s subscribed\n", OTA_UPDATE_TOPIC);
}

void loop() {
  mqtt.loop();
  if (otaurl != "") {
    Serial.println("OTA update: "+otaurl);
    mqtt.unsubscribe(OTA_UPDATE_TOPIC);
    mqtt.publish(OTA_UPDATE_TOPIC, "", true, 1);
    mqtt.disconnect();
    httpUpdate((char *)otaurl.c_str(), &ota_callback);
  }
  // Put your own application loop code after this line

}

void OnMessageCB(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);

  // Note: Do not use the client in the callback to publish, subscribe or
  // unsubscribe as it may cause deadlocks when other things arrive while
  // sending and receiving acknowledgments. Instead, change a global variable,
  // or push to a queue and handle it in the loop after calling `client.loop()`.

  StaticJsonDocument<128> doc;

  if (topic == OTA_UPDATE_TOPIC) {

    // Expected payload: {"url": "https://www.myserver.com:8443/device/firmware.bin"}

    DeserializationError error = deserializeJson(doc, payload);
    // Test if parsing succeeds.
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
    } else {
      if (doc["url"].as<String>() == "null") // e.g. if the received key isn't what we except
        Serial.println("Couldn't find \"url\" key");
      else
        otaurl = doc["url"].as<String>();
    }
  }
}

void display_ota_ip(const char *ip) {

  // Put your own IP address display code after this line
  // It can be a Serial print to the console (which is already done by the MyOwnOTA library),
  // a print out on a LCD display or whatever else display.
  // It can be through a message sent to Telegram, to a Gotify server: possibilties are endless
  Serial.printf("Ready for OTA connection on: %s/n", ip);
}