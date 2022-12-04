### Simple ESP32 IoT Framework

2022-12-01 - Version 0.3.0

[NOT READY YET - IN DEVELOPMENT]

This is a simple ESP32 Internet of Things framework.

Note: This is still work in progress. The documentation is also a work in progress. The code is working and must be considered of a Beta level.

This library implements a small, minimally secure, IOT Framework for embedded ESP32 devices to serve into a Home IOT environment. It diminishes the amount of code to be put in the targeted application source code. The application interacts with the framework through a Finite State Machine algorithm allowing for specific usage at every stage.

Here are the main characteristics:

    Framework configuration saved in file through SPIFFS.
    WiFi MQTT based communications. Nothing else.
    TLS encryption: All MQTT communication with encryption.
    Server authentication through fingerprinting.
    User/Password identification.
    JSON based data transmission with server.
    Configuration updates through MQTT.
    Option: OTA over MQTT.
    Option: Automated Watchdog transmission every 24 hours.
    Option: Battery voltage transmission.
    Option: DeepSleep or continuous power.
    Option: Application specific MQTT topic subscription.
    Option: Application specific automatic state saving in RTC memory.
    Option: Verbose/Silent debugging output through compilation.

The MQTT based transmission architecture is specific to this implementation and is describe below.

The framework is to be used with the PlarformIO ecosystem. Some examples can be found in the examples folder and shall be compiled through PlatformIO.

Note that the library maybe usable through the Arduino IDE, but this is not supported. It is known to be a challenge to set compiling options and access Maison defined types from .ino source code.

The Maison framework, to be functional, requires the following:

    Proper application setup parameters in file platformio.ini. Look at the Building an Application section;
    Code in the user application to setup and use the framework. Look at the Code Usage section;
    Configuration parameters located in SPIFFS (file data/config.json in example folders). Look at the Configuration Parameters section.

The sections below describe the specific of these requirements.

[To be completed]

### Configuration

The ESP32 IoT Framework configuration is done through the menuconfig capability associated with ESP-IDF. The following PlatformIO's menu option can be used to access the menuconfig application: `Platform → Run Menuconfig`.

All options for the framework will be found under the menuconfig entry named `Component config → ESP32 Simple IOT Framework`.

Here is the list of the configuration items:

- **Log Level**: Max log level used by the framework to report various log information on the USB port. The ESP-IDF maximum log level may require to be adjusted according to this item. It can be found in menuconfig at the following location: `Component config → Log output → Maximum log verbosity`.
- **MQTT Topic Name**: The topic name that will be used by the gateway to generate the topic to be sent to the MQTT broker.
- **Transmission Protocol**: The protocol to be used to transmit packets to the ESP32 Gateway. One of **UDP** or **ESP-NOW**.

For the UDP Protocol:
- **UDP Port**: The UDP Port to be used by the exerciser to transmit packets to the gateway.
- **UDP Max Packet Size**: The UDP maximum packet size allowed.
- **Gateway Address**: The Gateway address. It can be entered as a standard IPv4 dotted decimal notation (xx.xx.xx.xx) or as a DNS name.

For the ESP-NOW Protocol:
- **Primary Master Key**: The Primary Master Key (PMK) to use. The length of the PMK must be 16 characters. Please ensure that the key is in synch with the PMK defined in the gateway.
- **Gateway AP SSID Prefix**: The beginning of the SSID for the gateway Access Point (AP). This will be used to find the AP MAC address to transmit ESP-NOW packets to the gateway.
- **Encryption Enabled**: Set if this device is using packet encryption. If set, the gateway internal table of encrypted devices must be modified accordingly.
- **Local Master Key**: If encryption is enabled, the Local Master Key (LMK) for the exerciser to use. The length of LMK must be 16 characters. Please ensure that the LMK reflects the gateway configuration.
- **Channel**: The Wifi channel to be used. Note that it must be the same as defined in the WiFi router. Usual values are 1, 6, or 11. These preferred values are to diminish potential r/f interference.
- **Max Packet Size**: The ESP-NOW maximum packet size allowed without considering the CRC.  Cannot be larger than 248.
- **Enable Long Range**: When enable long range, the PHY rate of ESP32 will be 512Kbps or 256Kbps.

For the Wifi sub-system:
- **Wifi Gateway SSID**: SSID associated with Wifi as defined in your gateway. Will be used to find the gateway MAC address when unknown. Only needed with the ESP-NOW protocol.
- **Wifi Gateway Password**: Password associated to Wifi as defined in your gateway. Can be empty for a password-less configuration.  Only needed with the ESP-NOW protocol.
- **Wifi Router SSID**: SSID as defined in your router. Only needed with the UDP Protocol.
- **Wifi Router Password**: Password as defined in your router. Can be empty.  Only needed with the UDP Protocol.
- **Wifi Authorization Mode**: The authorization mode. Can be WEP, WPA, WPA2, or WPA3.