#include <Arduino.h>
#define ARDUINOJSON_DECODE_UNICODE 1
#include <ArduinoJson.h>
#include <httpupdate.h>
#include <MQTT.h> // https://github.com/256dpi/arduino-mqtt
#define WIFI_SSID "MyOwnSSID"
#define WIFI_KEY "MyOwnKey"
#define MQTT_SERVER "my.mqtt.server"
#define MQTT_PORT 1883

const char CACert[] PROGMEM = R"CERT(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)CERT";

ESP32HttpUpdate esp32Update;
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

  esp32Update.setCACert(CACert);
  //esp32Update.setInsecure();
}

void loop() {
  mqtt.loop();
  if (otaurl != "") {
    Serial.println("OTA update: "+otaurl);
    mqtt.unsubscribe(OTA_UPDATE_TOPIC);
    mqtt.publish(OTA_UPDATE_TOPIC, "", true, 1);
    mqtt.disconnect();
    esp32Update.httpUpdate((char *)otaurl.c_str(), &ota_callback);
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