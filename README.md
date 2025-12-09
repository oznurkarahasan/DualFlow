# Motion-Driven Light (Water Physics) - Pi Pico Dual Core

## Project Overview

This project is a virtual fluid physics engine running on a Raspberry Pi Pico. Using an MPU6050 sensor and a ring with 60 LEDs, it simulates an interactive “digital water balance” that responds in real time to the laws of physics (inertia, momentum, friction).

## Features

- **Realistic Physics:** Water does not stop instantly; it accelerates, decelerates, and flows according to gravity.
- **Dual-Core Architecture:** Physics calculations (Core 0) and LED visualization (Core 1) run simultaneously on separate cores for maximum performance.
- **Live Visuals:** The color of the water changes dynamically based on speed (Still Blue -> Moving Turquoise -> Foamy White).
- **Full Reliability:** The system automatically calibrates itself during startup. This ensures that the water remains perfectly stable on a flat surface, regardless of manufacturing defects in the sensor. Furthermore, even if a sensor fails, it detects the situation and automatically switches to “Simulation Mode” to continue operating.

## Hardware Setup

The project uses a Raspberry Pi Pico as the main controller. Below is the pin mapping for the peripherals.

![System](assets/hardwaresetup.png)

**Power Note:** The WS2812B LEDs require 5V (VBUS). Do not power them from the 3V3 pin, as it may damage the voltage regulator on the Pico when using high brightness.

## How to Test

You can verify the system's functionality and robustness using the following scenarios:

1. **Standard Operation & Calibration**

- Step 1: Place the Ring and Sensor on a flat, stable surface.

- Step 2: Power on the Pico via USB.

- Observation (Calibration): You will see an Orange Cross (+) blinking on the LED ring for exactly 2 seconds. Do not move the sensor during this phase.

- Observation (Ready): Once calibration is complete, the orange light turns off, and a Deep Blue point appears at the lowest gravity point.

- Action: Tilt the sensor in any direction (360°). The light should flow smoothly towards the lowest point, simulating liquid physics.

2. Visual Dynamics (The "Living Water" Effect)

Action: Tilt the sensor gently vs. rapidly.

Observation:

- Still: The water in deep blue.
- Moving: The color shifts to Turquoise.
- Shaking: Shake the sensor vigorously. The water turns White, simulating foam/turbulence.

3. Robustness Test (Fail-Safe Mode)

Objective: To prove the system does not crash when hardware fails.

- Step 1: Unplug the Pico.

- Step 2: Disconnect the MPU6050 sensor completely (remove VCC/GND/SDA/SCL wires).

- Step 3: Power on the Pico.

- Observation: The system detects the sensor failure, skips calibration, and automatically enters "Demo Mode". The light will oscillate back and forth driven by a generated sine wave, proving the software is robust against hardware faults.

4. Headless Testing (Serial Monitor)

Even without the LED Ring, you can visualize the physics engine.

-Setup: Connect Pico to PC and open Arduino Serial Monitor.

    Settings: Baud Rate 115200.

    Output: You will see a real-time ASCII representation of the ring:

    Vel: 0.00 	[       O        ]  <-- Stationary
    Vel: 1.50 	[         ~O~    ]  <-- Moving Right
    Vel: 4.20 	[           =O=  ]  <-- High Speed

## Dependencies

- Adafruit_NeoPixel
- MPU6050 (Electronic Cats)
