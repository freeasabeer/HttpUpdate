#pragma once

class ESP32HttpUpdate {
  public:
    ESP32HttpUpdate();
    ESP32HttpUpdate(Client &client);
    void setCACert(const char *rootCA);
    void setInsecure();
    void setWatchdog(unsigned long ms);
    void stopWatchdog();
    void httpUpdate(char *url, void (*cb)(const char* param) = nullptr);   // Deprecated, kept for legacy compatibility
    void httpUpdate(String &url, void (*cb)(const char* param) = nullptr); // Deprecated, kept for legacy compatibility
    void httpUpdate(char *url, bool spiffs = false);
    void httpUpdate(String &url, bool spiffs = false);
    void onStart(void (*cbOnStart)(void));
    void onEnd(void (*cbOnEnd)(void));
    void onError(void (*cbOnError)(int e));
    void onProgress(void (*cbOnProgress)(int c, int t));
    void (*_cbLegacy)(const char *s) = nullptr;

  private:
    typedef struct {
      char *host;
      uint16_t port;
      char *uri;
      bool ssl;
    } url_t;

    void update(Client &client, char *host, uint16_t port, char *uri);
    void parseurl(char *url, url_t *url_elts);
    String getHeaderValue(String header, String headerName);
    const char *_CA_cert;
    bool _use_insecure;
    Client *_client;
    bool _debug = false;
    void (*_cbStart)(void) = nullptr;
    void (*_cbEnd)(void) = nullptr;
    void (*_cbError)(int e) = nullptr;
    void (*_cbProgress)(int c, int t) = nullptr;
    bool _fsimg = false;
    TimerHandle_t _WatchDogTimer = NULL;
};
