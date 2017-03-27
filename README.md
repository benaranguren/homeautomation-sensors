# Home Automation Sensors

# Compiling
PlatformIO is used to compile this code.

To get started,
1. First clone this repository
2. Install dependencies
```
$ pio lib install 16 # [ 16  ] Adafruit BMP085 Unified
$ pio lib install 64 # [ 64  ] ArduinoJson
$ pio lib install 89 # [ 89  ] PubSubClient
$ pio lib install 18 # [ 18  ] Adafruit DHT Unified
```
3. Compile
```
$ pio run
```

# Modes
## Lightswitch 

`void loop_lightswitch()` tracks the state changes of the pins defined in sensor.ino.  When the state changes, HIGH to LOW and vice versa, a JSON message is sent containing the following:
```{"toggle_val": 0, "s1_val": 0, "s2_val": 1}```

Home Assistant can capture this message and perform the desired automation.

## Temperature and Humidity Sensor

## PIR Sensor

## RF433MHz Transmitter
PIN_RFTX is used in this mode to send data across the RF 433MHz channel.

![alt text](https://github.com/benaranguren/homeautomation-sensors/raw/master/doc/rf433.jpg "RF 433MHz Transmitter")

The pins on the transmitter board from left to right in picture are: GND, VCC, DATA.  In this wiring, VCC is connected to VIN of the NodeMCU and Data is connected to pin D2 via the orange wire.
