# ARK - Automated Wastepaper Recycling Machine

## Required Libraries

1. LiquidCrystal I2C by Frank de Brabander
2. HX711 Arduino Library by Bogdan Necula

## Hardware

- Arduino Mega 2560
- HX711 + Load Cell
- I2C LCD 16x2
- L298N Motor Driver(s)
- DC Motors
- PIR Sensor
- Relay Module
- Push Buttons
- 12V Power Supply

## Workflow

WEIGHING -> READY -> SHREDDING -> PULPING -> PRESSING -> DRYING -> COMPLETE

## Wiring Summary

HX711

- DT -> Pin 3
- SCK -> Pin 2

LCD I2C

- SDA -> Pin 20
- SCL -> Pin 21

Buttons

- START -> Pin 4 to GND
- RESET -> Pin 5 to GND

PIR

- OUT -> Pin 6

Relay

- IN -> Pin 7

Shredder Motor

- IN1 -> 8
- IN2 -> 9
- ENA -> 10

Mixer Motor

- IN1 -> 11
- IN2 -> 12
- ENA -> 13

Press Motor

- IN1 -> 22
- IN2 -> 23
- ENA -> 44

## Notes

- Calibrate the load cell before testing.
- Change LCD address from 0x27 to 0x3F if display stays blank.
- Motor timings are placeholders and should be adjusted after testing.
- All motor driver grounds and Arduino grounds must be connected together.
