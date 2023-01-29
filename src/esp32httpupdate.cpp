#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "esp32httpupdate.h"


// Public methods
ESP32HttpUpdate::ESP32HttpUpdate(int httpClientTimeout) {
  _CA_cert = nullptr;
  _use_insecure = false;
  _client = nullptr;
  _httpUpdate = new HTTPUpdate(httpClientTimeout);
}
ESP32HttpUpdate::ESP32HttpUpdate(Client &client, int httpClientTimeout) {
  _CA_cert = nullptr;
  _use_insecure = false;
  _client = &client;
  _httpUpdate = new HTTPUpdate(httpClientTimeout);
}

void ESP32HttpUpdate::setCACert(const char *rootCA) {
  _CA_cert = rootCA;
}

void ESP32HttpUpdate::setInsecure() {
  _use_insecure = true;
}

void ESP32HttpUpdate::onStart(void (*cbOnStart)(void)) {
  _httpUpdate->onStart(cbOnStart);
}
void ESP32HttpUpdate::onEnd(void (*cbOnEnd)(void)) {
  _httpUpdate->onEnd(cbOnEnd);
}
void ESP32HttpUpdate::onError(void (*cbOnError)(int e)) {
  _httpUpdate->onError(cbOnError);
}
void ESP32HttpUpdate::onProgress(void (*cbOnProgress)(int c, int t)) {
  _httpUpdate->onProgress(cbOnProgress);
}

static void ota_watchdog(TimerHandle_t xTimer)  {
  Serial.printf("httpUpdate: OTA watchdog");
  ESP.restart();
}
void ESP32HttpUpdate::setWatchdog(unsigned long ms) {
  if(_WatchDogTimer)
    xTimerDelete(_WatchDogTimer, 0);

  _WatchDogTimer = xTimerCreate("OTAWD", pdMS_TO_TICKS(ms), pdFALSE, (void*)0, &ota_watchdog);
  if(_WatchDogTimer)
    if (_debug) Serial.println("Watchdog timer created.");
  else
    if (_debug) Serial.println("Failed to create Watchdog timer.");
}
void ESP32HttpUpdate::stopWatchdog() {
  if (_WatchDogTimer) {
    xTimerStop(_WatchDogTimer, 0);
    if (_debug) Serial.println("Watchdog timer stopped.");
  }
}

void ESP32HttpUpdate::httpUpdate(String &url, bool fsimg) {
  httpUpdate((char *)url.c_str(), fsimg);
}

void ESP32HttpUpdate::httpUpdate(char *url, bool fsimg) {
  url_t url_elts;
  t_httpUpdate_return ret;

  parseurl(url, &url_elts);

  const String host = String(url_elts.host);
  uint16_t port = url_elts.port;
  const String uri = url_elts.uri;

  if (_debug) Serial.printf("httpUpdate (%s): %s://%s:%d%s\n",(fsimg)?"FSIMG":"FLASH", (url_elts.ssl)?"https":"http", url_elts.host, url_elts.port, url_elts.uri);

  if (_WatchDogTimer)
    if (xTimerStart(_WatchDogTimer, 0))
      if (_debug) Serial.println("Watchdog timer started.");
    else
      if (_debug) Serial.println("Failed to start Watchdog timer.");

  if (_client) {
    WiFiClient * wclient =  reinterpret_cast<WiFiClient *>(_client);
    if (_debug) Serial.println("httpUpdate: use provided client");

    if (fsimg)
      ret = _httpUpdate->updateSpiffs(*wclient, String(url));
    else
      ret = _httpUpdate->update(*wclient, host, port, uri);

  } else {
    if (url_elts.ssl) {
      WiFiClientSecure sclient;
      // Reading data over SSL may be slow, use an adequate timeout (in seconds)
      sclient.setTimeout(12);

      if ((!_use_insecure)&&(_CA_cert)) {
        if (_debug) {Serial.println("httpUpdate: Setting CACert");Serial.println(_CA_cert);}
        sclient.setCACert(_CA_cert);
      } else if (_use_insecure) {
        if (_debug) Serial.println("httpUpdate: Setting Insecure");
        sclient.setInsecure();
      }

      if (_debug) Serial.println("httpUpdate: starting update with sclient (https)");

      if (fsimg)
        ret = _httpUpdate->updateSpiffs(sclient, String(url));
      else
        ret = _httpUpdate->update(sclient, host, port, uri);

    } else {
      WiFiClient client;

      if (_debug) Serial.println("httpUpdate: starting update with client (http)");

      if (fsimg)
        ret = _httpUpdate->updateSpiffs(client, String(url));
      else
        ret = _httpUpdate->update(client, host, port, uri);
    }
  }

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n", _httpUpdate->getLastError(), _httpUpdate->getLastErrorString().c_str());
      break;

    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("HTTP_UPDATE_NO_UPDATES");
      break;

    case HTTP_UPDATE_OK:
      Serial.println("HTTP_UPDATE_OK");
      if (fsimg) ESP.restart();
      break;
  }
}

// Private methods
void ESP32HttpUpdate::parseurl(char *url, url_t *url_elts) {
  char *host = nullptr;
  char *ptr = nullptr;
  url_elts->ssl = false;
  url_elts->port = 0;
  if (strstr(url, "http://")) {
    ptr = &url[7];
    url_elts->ssl = false;
  } else if (strstr(url, "https://")) {
    ptr = &url[8];
    url_elts->ssl = true;
  }
  char *col = strchr(ptr, ':');
  if (col)
    *col = 0;
  else {
    col = ptr;
    url_elts->port = (url_elts->ssl)?443:80;
  }
  url_elts->uri = strchr(&col[1], '/');
  *url_elts->uri = 0;
  if (!url_elts->port) url_elts->port = atoi(&col[1]);
  url_elts->host = (char *)malloc(strlen(ptr)+1);
  strcpy(url_elts->host, ptr);
  *url_elts->uri = '/';
}

#if 0
void ESP32HttpUpdate::update(Client &client, char *host, uint16_t port, char *uri) {

  long contentLength = 0;
  bool isValidContentType = false;

  if(_WatchDogTimer)
    if (xTimerStart(_WatchDogTimer, 0))
      if (_debug) Serial.println("Watchdog timer started.");
    else
      if (_debug) Serial.println("Failed to start Watchdog timer.");


  Serial.printf("Connecting to: %s:%d\n", host, port);
  // Connect to server
  if (client.connect(host, port)) {
    // Connection Succeed.
    // Fecthing the bin
    Serial.println("Fetching Bin: " + String(uri));

    // Get the contents of the bin file
    client.print(String("GET ") + uri + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "Cache-Control: no-cache\r\n" +
                 "Connection: close\r\n\r\n");

    // Check what is being sent
    //    Serial.print(String("GET ") + bin + " HTTP/1.1\r\n" +
    //                 "Host: " + host + "\r\n" +
    //                 "Cache-Control: no-cache\r\n" +
    //                 "Connection: close\r\n\r\n");

    unsigned long timeout = millis();
    while (client.available() == 0) {
      if (millis() - timeout > 5000) {
        Serial.println("Client Timeout !");
        client.stop();
        if (host) free(host);
        return;
      }
    }
    // Once the response is available,
    // check stuff

    /*
       Response Structure
        HTTP/1.1 200 OK
        x-amz-id-2: NVKxnU1aIQMmpGKhSwpCBh8y2JPbak18QLIfE+OiUDOos+7UftZKjtCFqrwsGOZRN5Zee0jpTd0=
        x-amz-request-id: 2D56B47560B764EC
        Date: Wed, 14 Jun 2017 03:33:59 GMT
        Last-Modified: Fri, 02 Jun 2017 14:50:11 GMT
        ETag: "d2afebbaaebc38cd669ce36727152af9"
        Accept-Ranges: bytes
        Content-Type: application/octet-stream
        Content-Length: 357280
        Server: AmazonS3
                                   
        {{BIN FILE CONTENTS}}
    */
    while (client.available()) {
      // read line till /n
      String line = client.readStringUntil('\n');
      // remove space, to check if the line is end of headers
      line.trim();

      // if the the line is empty,
      // this is end of headers
      // break the while and feed the
      // remaining `client` to the
      // Update.writeStream();
      if (!line.length()) {
        //headers ended
        break; // and get the OTA started
      }

      // Check if the HTTP Response is 200
      // else break and Exit Update
      if (line.startsWith("HTTP/1.1")) {
        if (line.indexOf("200") < 0) {
          Serial.println("Got a non 200 status code from server. Exiting OTA Update.");
          break;
        }
      }

      // extract headers here
      // Start with content length
      if (line.startsWith("Content-Length: ")) {
        contentLength = atol((getHeaderValue(line, "Content-Length: ")).c_str());
        Serial.println("Got " + String(contentLength) + " bytes from server");
      }

      // Next, the content type
      if (line.startsWith("Content-Type: ")) {
        String contentType = getHeaderValue(line, "Content-Type: ");
        Serial.println("Got " + contentType + " payload.");
        if (contentType == "application/octet-stream") {
          isValidContentType = true;
        }
      }
    }
  } else {
    // Connect to S3 failed
    // May be try?
    // Probably a choppy network?
    Serial.println("Connection to " + String(host) + " failed. Please check your setup");
    // retry??
    // execOTA();
  }

  // Check what is the contentLength and if content type is `application/octet-stream`
  Serial.println("contentLength : " + String(contentLength) + ", isValidContentType : " + String(isValidContentType));

  // check contentLength and content type
  if (contentLength && isValidContentType) {
    // Check if there is enough to OTA Update
    if(_cbProgress) Update.onProgress(_cbProgress);
    bool canBegin = Update.begin(contentLength, (_fsimg)?U_SPIFFS:U_FLASH);

    // If yes, begin
    if (canBegin) {
      Serial.println("Begin OTA. This may take 2 - 5 mins to complete. Things might be quite for a while.. Patience!");
      if(_cbStart) _cbStart();
      // No activity would appear on the Serial monitor
      // So be patient. This may take 2 - 5mins to complete
      size_t written = Update.writeStream(client);

      if (written == contentLength) {
        Serial.println("Written : " + String(written) + " successfully");
      } else {
        Serial.println("Written only : " + String(written) + "/" + String(contentLength) + ". Retry?" );
        // retry??
        // execOTA();
      }

      if (Update.end()) {
        Serial.println("OTA done!");
        if (Update.isFinished()) {
          if (_cbEnd) _cbEnd();
          Serial.println("Update successfully completed. Rebooting.");
          ESP.restart();
        } else {
          Serial.println("Update not finished? Something went wrong!");
        }
      } else {
        uint8_t err = Update.getError();
        Serial.println("Error Occurred. Error #: " + String(err));
        if (_cbError) _cbError(err);
      }
    } else {
      // not enough space to begin OTA
      // Understand the partitions and
      // space availability
      Serial.println("Not enough space to begin OTA");
      client.flush();
    }
  } else {
    Serial.println("There was no content in the response");
    client.flush();
  }
  if (host) free(host);
}
#endif