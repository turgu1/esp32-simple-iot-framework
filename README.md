### Simple ESP32 IoT Framework

2022-12-01 - Version 0.3.0

[NOT READY YET - IN DEVELOPMENT]

This is a simple ESP32 Internet of Things framework.

This library implements a small, minimally secure, IOT Framework for embedded ESP32 devices to serve into a Home IOT environment. It diminishes the amount of code to be put in the targeted application source code. The application interacts with the framework through a Finite State Machine allowing for specific usage at every stage.

This framework requires the use of a **ESP32 Gateway** using the software v0.3.0 available [here](https://github.com/turgu1/esp32-gateway).

The framework is to be used with the PlarformIO ecosystem. Some examples can be found in the examples folder and shall be compiled through PlatformIO.

The following is the current state of items being developped:

- [x] ESP-NOW and UDP packets transmission to the ESP-32 Gateway
- [x] ESP-NOW encryption
- [x] ESP-NOW packet delivery verification
- [x] JSON Config file in a LittleFS partition
- [ ] UDP encryption
- [ ] UDP packet delivery verification
- [ ] MQTT delivery
- [ ] MQTT TLS encryption
- [ ] ESP-NOW and UDP packets reception from the ESP-32 Gateway
- [ ] Some configuration parameters update through specific packet reception
- [ ] Device reset / restart / status requests through specific packet reception
- [ ] OTA support for all protocols (UDP, ESP-NOW and MQTT)

[To be completed]

### Configuration

The ESP32 Simple IoT Framework configuration is done through the menuconfig capability associated with ESP-IDF. The following PlatformIO's menu option can be used to access the menuconfig application: `Platform → Run Menuconfig`.

The libray's **src/Kconfig** file must be merged in the user application **src/Kconfig.projbuild** as shown in provided examples: there is some issues with the platformio menuconfig capability, unable to retrieve the Kconfig content of lib_deps platformio.ini defined libraries. This maybe corrected in a future platformio version.

The parameters entered into menuconfig are considered as the default parameters. Through a JSON file named `config.json` in a LittleFS partition, it is possible to override the majority of the parameters, allowing for a single binary code usable for many different devices through the modification of the `config.json` content.

Here is the list of the configuration items. Labels are shown in bold face. When a parameter can be overriden through the `config.json` file, the field name is shown in parentheses with the maximum size in squared brackets (without counting the null terminating character) when appropriate:

- **Log Level** (*log_level*): Max log level used by the framework to report various log information on the USB port. The ESP-IDF maximum log level may require to be adjusted according to this item. It can be found in menuconfig at the following location: `Component config → Log output → Maximum log verbosity`. Value must be one of 0 (NONE), 1 (ERROR), 2 (WARN), 3 (INFO), 4 (DEBUG) and 5 (VERBOSE).
- **Device Name** (*device_name[32]*): The device name to be used inside transmitted JSON packets.
- **MQTT Topic Name** (*topic_name[32]*): The topic name that will be used by the gateway to generate the topic to be sent to the MQTT broker.
- **Enable battery voltage level retrieval**: If enabled, the battery voltage level will be retrieved using the `Battery` class. The code may require some adjustments depending on the electronics. Cannot be changed through config.json file.
- **Interval (in seconds) between Watchdog packet transmission** (*watchdog_interval*): The IoT framework is sending a Watchdog packet at the specified interval to signify that the device is still alive. 86400 seconds is one day. Value must be between 60 and 846000 seconds inclusive.
- **Transmission Protocol**: The protocol to be used to transmit packets to the ESP32 Gateway. One of **UDP** or **ESP-NOW**. Cannot be changed through config.json file.

For the UDP Protocol:
- **UDP Port** (*port*): The UDP Port to be used by the exerciser to transmit packets to the gateway. Value is between 1 and 65535.
- **UDP Max Packet Size** (*max_pkt_size*): The UDP maximum packet size allowed. Value can be between 10 and 1450 inclusive.
- **Gateway Address** (*gateway_address[128]*): The Gateway address. It can be entered as a standard IPv4 dotted decimal notation (xx.xx.xx.xx) or as a DNS name.
- **Wifi Router SSID** (*wifi_ssid[32]*): SSID as defined in your router. 
- **Wifi Router Password** (*wifi_psw[32]*): Password as defined in your router. Can be empty.  

For the ESP-NOW Protocol:
- **Primary Master Key** (*primary_master_key[16]*): The Primary Master Key (PMK) to use. The length of the PMK MUST BE 16 characters. Please ensure that the key is in synch with the PMK defined in the gateway.
- **Gateway AP SSID Prefix** (*gateway_ssid_prefix[16]*): The beginning of the SSID for the gateway Access Point (AP). This will be used to find the AP MAC address to transmit ESP-NOW packets to the gateway.
- **Encryption Enabled** (*encryption_enabled*): Set if this device is using packet encryption. If set, the gateway internal table of encrypted devices must be modified accordingly. Must be 0 (false), or 1 (true).
- **Local Master Key** (*local_master_key[16]*): If encryption is enabled, the Local Master Key (LMK) for the exerciser to use. The length of LMK MUST BE 16 characters. Please ensure that the LMK reflects the gateway configuration.
- **Channel** (*channel*): The Wifi channel to be used. Note that it must be the same as defined in the WiFi router. Usual values are 1, 6, or 11. These preferred values are to diminish potential r/f interference. Must be between 0 and 11 inclusive.
- **Max Packet Size** (*max_packet_size*): The ESP-NOW maximum packet size allowed without considering the CRC.  Cannot be larger than 248.
- **Enable Long Range** (*enable_long_range*): When enable long range, the PHY rate of ESP32 will be 512Kbps or 256Kbps. Must be 0 (false) or 1 (true).

For the Wifi sub-system:

- **Wifi Authorization Mode**: The authorization mode. Can be WEP, WPA, WPA2, or WPA3.  Cannot be changed through config.json file.