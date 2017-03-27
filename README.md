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
