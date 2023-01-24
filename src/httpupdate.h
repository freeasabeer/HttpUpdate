#pragma once

class ESP32HttpUpdate {
  public:
    ESP32HttpUpdate();
    ESP32HttpUpdate(Client &client);
    void setCACert(const char *rootCA);
    void setInsecure();
    void httpUpdate(char *url, void (*cb)(const char* param) = nullptr);
    void httpUpdate(String &url, void (*cb)(const char* param) = nullptr);

  private:
    typedef struct {
      char *host;
      uint16_t port;
      char *uri;
      bool ssl;
    } url_t;

    void update(Client &client, char *host, uint16_t port, char *uri, void (*cb)(const char* param));
    void parseurl(char *url, url_t *url_elts);
    String getHeaderValue(String header, String headerName);
    const char *_CA_cert;
    bool _use_insecure;
    Client *_client;
    bool _debug = false;
};
