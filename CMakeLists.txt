cmake_minimum_required(VERSION 3.5)

# sdkconfig dosyasının konumunu ayarla
set(EXTRA_COMPONENT_DIRS ${CMAKE_SOURCE_DIR}/src/embedded/main ${CMAKE_SOURCE_DIR}/src/embedded/mongoose)
set(SDKCONFIG ${CMAKE_SOURCE_DIR}/configs/sdkconfig)
set(SDKCONFIG_DEFAULTS ${CMAKE_SOURCE_DIR}/configs/sdkconfig.defaults)



include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(firmware)

