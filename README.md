# battery-monitoring-system
An ESP32-based battery monitoring system that measures voltage, current, and temperature to estimate battery state of health (SOH).

## Overview

I built this project to get more hands-on experience with electronics and embedded systems. I wanted to learn more about sensors, communication between components, and using data to control a system. I also got some practice designing schematics and troubleshooting a circuit.

## What I Learned

- Working with sensors and I2C communication.
- Reading analog signals and collecting measurement data.
- Using a MOSFET to control a circuit.
- Designing circuit schematics.
- Troubleshooting hardware and investigating measurement errors.
- Processing data and implementing calculations in C++.

## Firmware

The firmware is written in C++ using the Arduino IDE. It handles sensor communication, data collection, discharge control, and battery measurements.

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

### Completed Build

[View Prototype Photo](https://github.com/user-attachments/assets/af0d56b4-0042-4482-aae2-b19aa42c081f)

[View Circuit Schematic](https://github.com/user-attachments/assets/05567cf4-07ac-48ec-be65-62c255312d40)

## Testing and Results

The system was tested using a Molicel P28A 18650 lithium-ion battery and a 6Ω discharge resistor. The ESP32 collected voltage, current, and temperature measurements and performed calculations for capacity, energy, internal resistance, and estimated state of health (SOH).

During testing, approximately 0.8V of voltage drop was observed across the circuit. Further investigation identified significant voltage drops at the alligator clip connections, with approximately 0.3V lost at each clip. This contributed to inaccurate internal resistance calculations and caused the low-voltage cutoff to activate earlier than expected, shortening the discharge test.

The resulting SOH estimate was approximately 60%, which was not representative of the expected condition of the new battery. Temperature was also measured throughout testing but was not incorporated into the final SOH calculations.

Given the experience and functionality gained from the project, I decided to conclude testing rather than replace the connections and continue troubleshooting. With additional development time, I would improve the circuit connections, calibrate the measurement system, and incorporate temperature into the SOH estimation process.
