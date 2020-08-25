# esp32-terrarium-control-board

This is a simple microcontroller terrariums climate control project. It consists of three parts:
- ESP32 program written in the Arduino IDF
- Server-side code
- Android app for remote control

# ESP32 program
Basically, the ESP32 controls the climate of our terrariums using a PID controller based on the input data from an DHT sensor and user input. Feature list:
- two core utilization
- simple PID implementation
- controls a heater and a humidifier
- displays data on a local LCD
- can be locally managed with 4 buttons
- connects with an internet server
-- to send readings
-- to send the state of the heater and humidifier
-- to send new locally defined target states of temperature and humidity
-- receive new target states of temperature and humidity from an online user
- can execute order 66 - approved by Darth Sidious himself [Alpha, not present in the public build]

# The project on a breadboard

![alt text](https://github.com/Asteliks/esp32-terrarium-control-board/blob/master/Pictures/IMG_3757.JPG)
![alt text](https://github.com/Asteliks/esp32-terrarium-control-board/blob/master/Pictures/IMG_3758.JPG)
![alt text](https://github.com/Asteliks/esp32-terrarium-control-board/blob/master/Pictures/IMG_3759.JPG)
![alt text](https://github.com/Asteliks/esp32-terrarium-control-board/blob/master/Pictures/IMG_3760.JPG)
note: I've used LEDs to depict the state of the heater and humidifier relays
