#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Update.h>
#include "httpupdate.h"
#include <StreamString.h>

static void (*cb)(const char *s) = nullptr;
static void workaround(ESP32HttpUpdate *ptr) {
  cb = ptr->_cbLegacy;
}
static void cbHelper() {
  if (cb) cb("OTA start");
}

// Public methods
ESP32HttpUpdate::ESP32HttpUpdate() {
  _CA_cert = nullptr;
  _use_insecure = false;
  _client = nullptr;
}
ESP32HttpUpdate::ESP32HttpUpdate(Client &client) {
  _CA_cert = nullptr;
  _use_insecure = false;
  _client = &client;
}

void ESP32HttpUpdate::setCACert(const char *rootCA) {
  _CA_cert = rootCA;
}

void ESP32HttpUpdate::setInsecure() {
  _use_insecure = true;
}

void ESP32HttpUpdate::onStart(void (*cbOnStart)(void)) {
  _cbStart = cbOnStart;
}
void ESP32HttpUpdate::onEnd(void (*cbOnEnd)(void)) {
  _cbEnd = cbOnEnd;
}
void ESP32HttpUpdate::onError(void (*cbOnError)(int e)) {
  _cbError = cbOnError;
}
void ESP32HttpUpdate::onProgress(void (*cbOnProgress)(int c, int t)) {
  _cbProgress = cbOnProgress;
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

void ESP32HttpUpdate::httpUpdate(char *url, void (*cb)(const char* param)) {   // Deprecated, kept for legacy compatibility
  _cbLegacy = cb;
  _cbStart = &cbHelper;
  workaround(this);
  httpUpdate(url, false);
}

void ESP32HttpUpdate::httpUpdate(String &url, void (*cb)(const char* param)) { // Deprecated, kept for legacy compatibility
  httpUpdate((char *)url.c_str(), cb);
}


void ESP32HttpUpdate::httpUpdate(String &url, bool fsimg) {
  httpUpdate((char *)url.c_str(), fsimg);
}

void ESP32HttpUpdate::httpUpdate(char *url, bool fsimg) {
  url_t url_elts;
  _fsimg = fsimg;
  parseurl(url, &url_elts);
  if (_debug) Serial.printf("httpUpdate (%s): %s://%s:%d%s\n", (fsimg)?"FSIMG":"FLASH", (url_elts.ssl)?"https":"http", url_elts.host, url_elts.port, url_elts.uri);
  if (_client) {
      if (_debug) Serial.println("httpUpdate: use provided client");
      update(*_client, url_elts.host, url_elts.port, url_elts.uri);
  } else {
    if (url_elts.ssl) {
      WiFiClientSecure sclient;
      if ((!_use_insecure)&&(_CA_cert)) {
        if (_debug) {Serial.println("httpUpdate: Setting CACert");Serial.println(_CA_cert);}
        sclient.setCACert(_CA_cert);
      } else if (_use_insecure) {
        if (_debug) Serial.println("httpUpdate: Setting Insecure");
        sclient.setInsecure();
      }
      if (_debug) Serial.println("httpUpdate: starting update with sclient (https)");
      update(sclient, url_elts.host, url_elts.port, url_elts.uri);
    } else {
      WiFiClient client;
      if (_debug) Serial.println("httpUpdate: starting update with client (http)");
      update(client, url_elts.host, url_elts.port, url_elts.uri);
    }
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

// Utility to extract header value from headers
String ESP32HttpUpdate::getHeaderValue(String header, String headerName) {
  return header.substring(strlen(headerName.c_str()));
}

void ESP32HttpUpdate::update(Client &client, char *host, uint16_t port, char *uri) {

  long contentLength = 0;
  bool isValidContentType = false;

  if(_WatchDogTimer)
    if (xTimerStart(_WatchDogTimer, 0))
      if (_debug) Serial.println("Watchdog timer started.");
    else
      if (_debug) Serial.println("Failed to start Watchdog timer.");


  if (_debug) Serial.printf("Connecting to: %s:%d\n", host, port);
  // Connect to server
  if (client.connect(host, port)) {
    // Connection Succeed.
    // Fecthing the bin
    if (_debug) Serial.println("Fetching Bin: " + String(uri));

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
        setLastError(ESP32HTTPUPDATE_CLIENT_TIMEOUT);
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
          setLastError(ESP32HTTPUPDATE_NON_200_STATUS_CODE);
          client.stop();
          break;
        }
      }

      // extract headers here
      // Start with content length
      if (line.startsWith("Content-Length: ")) {
        contentLength = atol((getHeaderValue(line, "Content-Length: ")).c_str());
        if (_debug) Serial.println("Got " + String(contentLength) + " bytes from server");
      }

      // Next, the content type
      if (line.startsWith("Content-Type: ")) {
        String contentType = getHeaderValue(line, "Content-Type: ");
        if (_debug) Serial.println("Got " + contentType + " payload.");
        if (contentType == "application/octet-stream") {
          isValidContentType = true;
        }
      }
    }
  } else {
    // Connect to S3 failed
    // May be try?
    // Probably a choppy network?
    setLastError(ESP32HTTPUPDATE_CONNECTION_FAILED);
    // retry??
    // execOTA();
  }

  // Check what is the contentLength and if content type is `application/octet-stream`
  if (_debug) Serial.println("contentLength : " + String(contentLength) + ", isValidContentType : " + String(isValidContentType));

  // check contentLength and content type
  if (contentLength && isValidContentType) {
    // Check if there is enough to OTA Update
    if(_cbProgress) Update.onProgress(_cbProgress);
    bool canBegin = Update.begin(contentLength, (_fsimg)?U_SPIFFS:U_FLASH);

    // If yes, begin
    if (canBegin) {
      if (_debug) Serial.println("Begin OTA. This may take 2 - 5 mins to complete. Things might be quite for a while.. Patience!");
      if (_cbStart) _cbStart();
      // No activity would appear on the Serial monitor
      // So be patient. This may take 2 - 5mins to complete
      size_t written = Update.writeStream(client);

      if (written == contentLength) {
        if (_debug) Serial.println("Written : " + String(written) + " successfully");
      } else {
        _lastError = ESP32HTTPUPDATE_WRITTEN_LESS_CONTENT_LENGTH;
        if (_debug) Serial.println("Written only : " + String(written) + "/" + String(contentLength) + ". Retry?" );
        if (_cbError) _cbError(_lastError);
        // retry??
        // execOTA();
      }

      if (Update.end()) {
        if (_debug) Serial.println("OTA done!");
        if (Update.isFinished()) {
          if (_cbEnd) _cbEnd();
          if (_debug) Serial.println("Update successfully completed. Rebooting.");
          ESP.restart();
        } else {
          setLastError(ESP32HTTPUPDATE_UPDATE_NOT_FINISHED);
        }
      } else {
        uint8_t err = Update.getError();
        _lastError = err;
        if (_debug) Serial.println("Update Error #: " + String(err));
        if (_cbError) _cbError(err);
      }
    } else {
      // not enough space to begin OTA
      // Understand the partitions and
      // space availability
      setLastError(ESP32HTTPUPDATE_NOT_ENOUGH_SPACE);
      client.flush();
    }
  } else {
    setLastError(ESP32HTTPUPDATE_EMPTY_SERVER_RESPONSE);
    client.flush();
  }
  if (host) free(host);
}

int ESP32HttpUpdate::getLastError() {
    return _lastError;
}

String ESP32HttpUpdate::getLastErrorString() {
  if(_lastError == 0) {
    return String(); // no error
  }

  // error from Update class
  if(_lastError > 0) {
    StreamString error;
    Update.printError(error);
    error.trim(); // remove line ending
    return String("Update error: ") + error;
  }
  return _ErrorString[_lastError];
}

void ESP32HttpUpdate::setLastError(int err) {
  _lastError = err;
  if (_debug) Serial.println(_ErrorString[_lastError]);
  if (_cbError) _cbError(err);
}