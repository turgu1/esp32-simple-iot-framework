cmake_minimum_required(VERSION 3.16.0)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(test-esp32-simple-iot-framework)

littlefs_create_partition_image(littlefs data)