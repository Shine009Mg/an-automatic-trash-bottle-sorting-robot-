Robotics and Automation System Engineering Project-1
# ♻️ Smart Bottle Sorting Robot

An automated bottle sorting system developed as an engineering project for the **Robotics and Automation System** program.

The system uses an **ESP32 and multiple sensors** to detect bottle characteristics, process the collected data, and automatically sort bottles into different compartments.

## 🎯 Objectives

* Automate the bottle sorting process
* Detect and classify bottles using sensors
* Measure bottle weight
* Detect metallic objects
* Reduce the need for manual waste separation
* Monitor the system using an IoT dashboard

## ⚙️ System Overview

The bottle is placed on a rotating sorting plate. A **load cell** measures its weight while an **inductive proximity sensor** detects metallic material. Ultrasonic and magnetic sensors are also used for object and position detection.

The ESP32 processes the sensor data and controls the servo motors to rotate the sorting plate toward the correct compartment.

```text
Bottle
  ↓
Sensors
  ↓
ESP32
  ↓
Classification
  ↓
Servo Motor
  ↓
Correct Compartment
```

## 🔧 Hardware

* ESP32
* Load Cell + HX711
* Inductive Proximity Sensor
* Ultrasonic Sensors
* Hall Effect / Magnetic Sensors
* 180° & 360° Servo Motors
* LCD 16×2
* Buzzer
* Red/Green LEDs
* LM2596 DC-DC Step-Down Module
* Li-ion Batteries

## 🧠 Main Features

* Automatic bottle detection
* Weight measurement
* Metal detection
* Sensor-based classification
* Automatic mechanical sorting
* Position feedback using magnetic sensors
* LCD and LED status indication
* IoT dashboard monitoring

## 🏗️ Mechanical Design

The system uses a cylindrical bin structure with steel support bars. The sorting plate is located at the center and connected to the sorting mechanism.

The compartments are arranged at approximately **120° angles**, allowing the rotating plate to direct bottles into the appropriate section.

## 🧪 Testing & Improvements

During development, several problems were encountered and improved:

* **Unstable tank structure** → reinforced the structure with additional support.
* **Servo positioning error** → added a 360° servo with magnets and a magnetic sensor for position control.
* **Crowded wiring** → designed a larger 3D-printed electronics box.
* **Unstable voltage** → added an LM2596 voltage regulator.
* **CAD and physical dimension mismatch** → updated the SolidWorks dimensions based on actual measurements.

## 🚧 Current Status

The mechanical and electronic systems have been developed and tested, and an IoT dashboard has been created.

The remaining work includes **real-time ESP32-to-web communication, final system integration, and further performance testing**.

## 👥 Team

* Arisara Dachthongkum
* Shine Lin Htet
* Sorawit Somponwattana
* Pattanawit Suriyo

**Robotics and Automation System — Engineering Project**
