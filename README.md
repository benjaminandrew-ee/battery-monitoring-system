# battery-monitoring-system
An ESP32-based battery monitoring system that measures voltage, current, and temperature to estimate battery state of health (SOH).

## Overview

I built this project to get more hands-on experience with electronics and embedded systems. I wanted to learn more about sensors, communication between components, and using data to control a system. I also got some practice designing schematics and troubleshooting a circuit.

## Hardware
- ESP32-S3
- INA226 voltage and current sensor
- 10kΩ NTC thermistor
- IRLZ44N MOSFET
- Molicel P28A 18650 lithium-ion battery
- 6Ω, 5W resistor
- x2 10k resistors
- TP4056 charging module

## How It Works

The ESP32 collects voltage, current, and temperature measurements from the battery. The INA226 communicates with the ESP32 over I2C, while the thermistor is read through an analog input. A MOSFET controls the discharge load, allowing the system to collect data as the battery discharges. The collected data is then used to calculate capacity, energy, and estimate battery state of health (SOH).

## Photos

### Completed Prototype

[View Prototype Photo](https://github.com/user-attachments/assets/af0d56b4-0042-4482-aae2-b19aa42c081f)
