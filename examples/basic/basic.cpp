#include <Arduino.h>
#include "httpupdate.h"

#define WIFI_SSID "MyOwnSSID"
#define WIFI_KEY "MyOwnKey"

static int pinOTAvalue;
void display_ota_ip(const char *ip);

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
    return;
  }
  // Put your own application set-up code after this line

}


void loop() {
  if (pinOTAvalue == LOW) {
    httpUpdate("https://www.myserver.com:8443/device/firmware.bin", &display_ota_ip);
    return;
  }

  // Put your own application loop code after this line


}

void display_ota_ip(const char *ip) {

  // Put your own IP address display code after this line
  // It can be a Serial print to the console (which is already done by the MyOwnOTA library),
  // a print out on a LCD display or whatever else display.
  // It can be through a message sent to Telegram, to a Gotify server: possibilties are endless
  Serial.printf("Ready for OTA connection on: %s/n", ip);

}