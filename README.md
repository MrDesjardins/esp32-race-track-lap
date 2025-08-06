# Summary

The project contains the code for the ESP-32 that power the lap counter.

# Hardware

1. ESP32-WROOM-32D
1. 1.3 Inch I2C IIC OLED Display Module 128X64 Pixel SH110

# Pre-Requisite

1. Adafruit SH110X@2.1.13
1. Adafruit GFX Library@1.12.1
1. Wire
1. Bounce2@2.71.0

# Wiring

| Device    | ESP32 Pin | Device       Pin |
| --------- | --------- | ---------------- |
| Button 1  | GPIO 18   | GND              |
| Button 2  | GPIO 19   | GND              |
| Reset Btn | GPIO 23   | GND              |
| OLED SDA  | GPIO 21   | SDA              |
| OLED SCL  | GPIO 22   | SCL              |
| OLED GND  | GND       | GND              |
| OLED VCC  | VCC       | VCC              |

