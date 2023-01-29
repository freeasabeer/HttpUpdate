#pragma once
#include <map>

#define ESP32HTTPUPDATE_CLIENT_TIMEOUT        (-100)
#define ESP32HTTPUPDATE_NON_200_STATUS_CODE   (-101)
#define ESP32HTTPUPDATE_CONNECTION_FAILED     (-102)
#define ESP32HTTPUPDATE_WRITTEN_LESS_CONTENT_LENGTH (-103)
#define ESP32HTTPUPDATE_UPDATE_NOT_FINISHED   (-104)
#define ESP32HTTPUPDATE_NOT_ENOUGH_SPACE      (-105)
#define ESP32HTTPUPDATE_EMPTY_SERVER_RESPONSE (-106)


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
    int getLastError();
    String getLastErrorString();

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
    void setLastError(int err);
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
    int _lastError = 0;
    std::map<int, String> _ErrorString {
      {ESP32HTTPUPDATE_CLIENT_TIMEOUT, "Connection to server timeout."},
      {ESP32HTTPUPDATE_NON_200_STATUS_CODE, "Got a non 200 status code from server. Exiting OTA Update."},
      {ESP32HTTPUPDATE_CONNECTION_FAILED, "Connection to server failed. Please check your setup."},
      {ESP32HTTPUPDATE_WRITTEN_LESS_CONTENT_LENGTH, "Written less data than expected. Retry ?"},
      {ESP32HTTPUPDATE_UPDATE_NOT_FINISHED, "Update not finished? Something went wrong!"},
      {ESP32HTTPUPDATE_NOT_ENOUGH_SPACE, "Not enough space to begin OTA."},
      {ESP32HTTPUPDATE_EMPTY_SERVER_RESPONSE, "There was no content in the response."}};
};
