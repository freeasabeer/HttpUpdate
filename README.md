# HttpUpdate

This library provides a working http/https OTA update based on the ESP32 UpdateClass.

# Installation instruction

## Arduino IDE
First make sure that all instances of the Arduino IDE are closed. The IDE only scans for libraries at startup. It will not see your new library as long as any instance of the IDE is open!

Download https://github.com/freeasabeer/HttpUpdate/archive/refs/heads/main.zip and unzip `MyOwnOTA-main.zip`

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
