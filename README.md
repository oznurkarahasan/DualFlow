# Motion-Driven Light (Water Physics) - Pi Pico Dual Core

## Project Overview

This project simulates fluid dynamics on a WS2812B LED strip using a Raspberry Pi Pico. The "water" reacts to the tilt of an MPU6050 sensor, featuring momentum, friction, and wall bounces.

## Features

- **Dual-Core Architecture:** \* **Core 0:** Handles sensor I/O and physics calculations (Gravity, Velocity, Friction).
  - **Core 1:** Handles LED rendering (Trail effects, Shimmer) and Serial visualization.
- **Physics Engine:** Custom logic for fluid inertia and damping.
- **Robustness:** Automatically falls back to a "Simulation Mode" (Sine wave) if the sensor is disconnected.
- **Visual Effects:** \* _Shimmer:_ Water turns whiter/frothy as velocity increases.

  - _Bounce Flash:_ Edges flash white upon high-impact collision.

  **Feature: Auto-Calibration:** On startup, the system takes 100 samples to calculate the sensor's "Zero-G Offset". This ensures that the water stays perfectly still on a flat surface, regardless of manufacturing imperfections in the sensor.

## Hardware Setup

- **MCU:** Raspberry Pi Pico
- **Sensor:** MPU6050 (SDA: GP4, SCL: GP5)
- **Output:** WS2812B LED Strip (Data: GP28)

## How to Test

1.  **With Hardware:** Connect the MPU6050 and LED strip. Tilt the sensor to move the light. Shake rapidly to see the color change (Shimmer).
2.  **Without Sensor (Robustness):** Disconnect the MPU6050. The system will detect the fault and enter simulation mode, moving the light automatically.
3.  **Serial Monitor:** Open at 115200 baud to view the ASCII visualization: `[   ~O~    ] Vel: 0.45`

## Dependencies

- Adafruit_NeoPixel
- MPU6050 (Electronic Cats)
