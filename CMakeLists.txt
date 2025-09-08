cmake_minimum_required(VERSION 3.5)

# sdkconfig dosyasının konumunu ayarla
set(SDKCONFIG ${CMAKE_SOURCE_DIR}/configs/sdkconfig)
set(SDKCONFIG_DEFAULTS ${CMAKE_SOURCE_DIR}/configs/sdkconfig.defaults)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(firmware)