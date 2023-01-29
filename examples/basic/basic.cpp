#include <Arduino.h>
#include "httpupdate.h"

#define WIFI_SSID "MyOwnSSID"
#define WIFI_KEY "MyOwnKey"

static int pinOTAvalue;
ESP32HttpUpdate esp32Update;

void setup() {
  Serial.begin(115200);

  pinMode(27, INPUT_PULLUP);
#ifdef LED_BUILTIN
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
#endif
  pinOTAvalue = digitalRead(27);
  if (pinOTAvalue == LOW) {
    // Switch to OTA update mode
#ifdef LED_BUILTIN
    digitalWrite(LED_BUILTIN, HIGH);
#endif
    Serial.println("Connecting to Wi-Fi...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("Wi-Fi connected");
    return;
  }
  // Put your own application set-up code after this line

}

void httpupdate_started_cb() {
  Serial.println("Firmware OTA update started");
}
void httpupdate_finished_cb() {
  Serial.println("Firmware OTA update OK\nReboot");
  delay(1000);
}
void httpupdate_progress_cb(int cur, int total) {
  int p;
  p = (cur / (total / 100));
  Serial.printf("Progress: %u%%\r", p);
}
void httpupdate_error_cb(int err) {
  Serial.printf("Firmware OTA update fatal error\nCode: %d\n", err);
}

void loop() {
  if (pinOTAvalue == LOW) {
    esp32Update.setWatchdog(3*60*1000);
    esp32update.onStart(httpupdate_started_cb);
    esp32update.onEnd(httpupdate_finished_cb);
    esp32update.onProgress(httpupdate_progress_cb);
    esp32update.onError(httpupdate_error_cb);
    esp32Update.httpUpdate("http://www.myserver.com:8080/device/firmware.bin", false);
    return;
  }

  // Put your own application loop code after this line


}