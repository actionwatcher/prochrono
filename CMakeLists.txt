# this cmake file was designed for CLion IDE. It is not necessary to use it. Just make sure that you have all the libraries and the toolchain installed.
cmake_minimum_required(VERSION 2.8.4)
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/cmake/ArduinoToolchain.cmake)
set(CMAKE_CXX_STANDARD 11)
set(PROJECT_NAME ProChrono)
set(ARDUINO_TYPE pro) # uno or pro
set(${PROJECT_NAME}_BOARD ${ARDUINO_TYPE})
if (ARDUINO_TYPE MATCHES pro)
    set(ARDUINO_CPU 16MHzatmega328)
endif()

project(${PROJECT_NAME})

### Additional static libraries to include in the target.
set(${PROJECT_NAME}_LIBS ${ARDUINO_TYPE}_SoftwareSerial)

### Main sketch file
set(${PROJECT_NAME}_SKETCH pro_chrono.ino)


#### Additional settings for programmer. From programmers.txt
if (ARDUINO_TYPE MATCHES pro)
    set(${PROJECT_NAME}_PROGRAMMER usbasp)
    set(${PROJECT_NAME}_PORT /dev/cu.usbserial-A9WR3D5H) # FTDI programmer
else()
    set(${PROJECT_NAME}_PROGRAMMER avrispmkii)
    set(${PROJECT_NAME}_PORT /dev/cu.usbmodem14301) #uno
endif()

generate_arduino_firmware(${PROJECT_NAME})


print_board_list()
print_programmer_list()
