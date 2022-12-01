cmake_minimum_required(VERSION 3.16)
idf_component_register(
    SRCS
        "src/iot.cpp" 
        "src/global.cpp"
        "src/battery.cpp"
        "src/esp_now.cpp"
        "src/nvs_mgr.cpp"
        "src/udpp.cpp"
        "src/utils.cpp"
        "src/wifi.cpp"                    
    INCLUDE_DIRS 
        "include")