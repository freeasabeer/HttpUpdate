#pragma once
#define NO_GLOBAL_httpUpdate
#include <HTTPUpdate.h>

class ESP32HttpUpdate {
  public:
    ESP32HttpUpdate(Client &client, int httpClientTimeout=8000);
    ESP32HttpUpdate(int httpClientTimeout=8000);
    void setCACert(const char *rootCA);
    void setInsecure();
    void setWatchdog(unsigned long ms);
    void stopWatchdog();
    void httpUpdate(char *url, bool spiffs = false);
    void httpUpdate(String &url, bool spiffs = false);
    void onStart(void (*cbOnStart)(void));
    void onEnd(void (*cbOnEnd)(void));
    void onError(void (*cbOnError)(int e));
    void onProgress(void (*cbOnProgress)(int c, int t));

  private:
    typedef struct {
      char *host;
      uint16_t port;
      char *uri;
      bool ssl;
    } url_t;

    void parseurl(char *url, url_t *url_elts);
    const char *_CA_cert;
    bool _use_insecure;
    Client *_client;
    bool _debug = false;
    TimerHandle_t _WatchDogTimer = NULL;
    HTTPUpdate *_httpUpdate;
    int _httpClientTimeout;
};
