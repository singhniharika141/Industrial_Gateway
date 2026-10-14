# Industrial IoT Gateway — ESP32

An embedded systems learning project focused on building an industrial gateway using ESP32, ESP-IDF, FreeRTOS, UART and RS485 communication.

## Overview

An industrial gateway connects field devices to another system by receiving data, processing it and making it available for monitoring or further communication.

This project explores the gateway concept through two independent ESP-IDF applications: gateway firmware and a device simulator.

## Repository Structure

```text
Industrial_Gateway/
├── README.md
├── .gitignore
├── gateway/
│   ├── CMakeLists.txt
│   └── main/
│       ├── CMakeLists.txt
│       └── main.c
└── industrial_device_simulator/
    ├── CMakeLists.txt
    └── main/
        ├── CMakeLists.txt
        └── main.c
```

## Hardware

- ESP32 development boards
- MAX485 RS485 transceiver modules
- UART connections between the ESP32 boards and their RS485 transceivers

## Technologies

- C
- ESP-IDF
- FreeRTOS
- UART
- RS485

## Current Implementation

- Configured the ESP-IDF development environment in VS Code.
- Built the initial ESP32 firmware.
- Implemented a periodic system heartbeat using FreeRTOS.
- Configured UART communication and installed the UART driver.
- Connected two ESP32 boards using MAX485 transceiver modules.
- Successfully tested message transmission and reception over RS485, receiving `Hello from gateway`.

## Build Instructions

Install ESP-IDF and its required tools, then open a PowerShell terminal and activate the ESP-IDF environment.

Build the gateway:

```powershell
cd gateway
idf.py build
```

Build the device simulator:

```powershell
cd industrial_device_simulator
idf.py build
```

Run each build command from the corresponding project directory.

## Planned Improvements

- Implement Modbus RTU communication.
- Develop a structured device-data request and response flow.
- Add data validation and error handling.
- Expand the gateway toward industrial monitoring and data collection use cases.

This project is under active development. Implemented features and planned improvements are documented separately to make progress clear.
