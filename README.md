# ESP32-SDK
This is an UNIQUID SDK for ESP32 based on ESP-IDF SDK using PlatformIO IDE.

## Requirements
- esp32-sdk repository (this repo)
- ESP32 Dev board (with ESP-WROOM-32)
- Visual Studio Code ([latest version](https://code.visualstudio.com/))
    - plugin: PlatformIO IDE (tested version 0.16.1)
- Termite: a simple RS232 terminal ([3.4 - latest version](https://www.compuphase.com/software_termite.htm)) to debug

## Getting started
### setup the development environment
- install Visual Studio and then PlatformIO IDE plugin from the marketplace
### setup the hardware
- connect the board to the PC to install driver (PlatformIO discovers the right COM port by itself)
### download the repository 
- ``git clone --recurse-submodules https://github.com/uniquid/esp32-sdk.git``

### HOW-TO
- open firmware directory (esp32-sdk) with Visual Studio Code (right click is the best way)
- click on the white arrow in the footbar of visual code window (PlatformIO: Upload button) to compile and upload the firmware
- if it's not working (sometimes Visual Studio Code updates too quickly), click on the embedded terminal and add new one (it's a powershell) and execute ``platformio.exe run --target upload``
- the board is ready!

#### define
there are some define in ``platformio.ini`` thant can/must be change.

- ``-DLOG_LOCAL_LEVEL=ESP_LOG_ERROR``: to choose the level of the ESP-IDF logger
- ``-DNAME_PREFIX=\"ESP32_\"``: prefix of the name of the identity
- ``-DMQTT_HOST=\"IP_MQTT\"``: mqtt broker host (MUST)
- ``-DMQTT_PORT=1883``: mqtt port
- ``-DREGISTRY_URL=\"http://xxx.xxx.xxx.xxx:xxxx/registry\"``: registry url with ip:port (MUST)
- ``-DMY_SSID=\"SSID\"``: wifi ssid (MUST)
- ``-DMY_PASSWORD=\"PASSWORD\"``: wifi password (MUST)
- ``-DTIMEOUT_READER=5``: interaction timeout with the serial terminal from the system start, in seconds x2, if you enter 5, you have 10 seconds after starting to send commands.

## APPLICATION
### persistence
The Uniquid identity is saved in the NV-SRAM of the ESP32, using the Non-volatile storage library (NVS).
### serial terminal interaction
At the system start, you have TIMEOUT_READER x2 seconds to send this commands:
- ``e``: to erase the Uniquid identity
- ``r``: to restart the board

if you send ``e``, remember to send ``r`` after or the board will not have an identity
### rpc method
All rpc methods are setted like ECHO method.

if the user sends
- ``{"sender":"user-address", "body":{"method":35, "params":"Hello World!", "id":65656} }``

the provider (board) responds with
- ``{"sender":"provider-address","body":{"result":"Hello World!","error":0,"id":65656} }``

## info about porting
- PlatformIO don't have a clean way to manage the include search path order. System paths are put always before the user ones. We used prebuild.py script to insert extern/hw/includeoverride before the system include search paths.
- prebuild.py is used to install the yajl api include files in extern/hw/yajl