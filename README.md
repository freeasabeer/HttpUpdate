# HttpUpdate

This library provides a simple class around the [AWS_S3_OTA_Update
](https://github.com/espressif/arduino-esp32/tree/master/libraries/Update/examples/AWS_S3_OTA_Update) example of the ESP32 [Update](https://github.com/espressif/arduino-esp32/tree/master/libraries/Update) library.

Unlike the **HttpsOTAUpdate** class provided by the ESP32 [Update](https://github.com/espressif/arduino-esp32/tree/master/libraries/Update), this library provides either http or https update which actualy works with https.

It also offers the possibility to provide its own **Client** object which might be usefull with [TinyGSM](https://github.com/vshymanskyy/TinyGSM) clients or [SSLClient](https://github.com/govorox/SSLClient) wrappers.


# Installation instruction

## Arduino IDE
First make sure that all instances of the Arduino IDE are closed. The IDE only scans for libraries at startup. It will not see your new library as long as any instance of the IDE is open!

Download https://github.com/freeasabeer/HttpUpdate/archive/refs/heads/main.zip and unzip `HttpUpdate-main.zip`

Rename the unzipped folder `HttpUpdate-main`to `HttpUpdate` and move it to the Arduino IDE library folder:
- Windows: `C:\Users\<your user name>\Documents\Arduino\Libraries`
- Linux `$HOME/Arduino/sketchbook/libraries`
- MacOS: `$HOME/Documents/Arduino/libraries`

Restart the Arduino IDE and verify that the library appears in the File->Examples menu

## Platformio
Update the `lib_deps` section of your `platformio.ini` file as following:
```
lib_deps =
  HttpUpdate=https://github.com/freeasabeer/HttpUpdate
```
