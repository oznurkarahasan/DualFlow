#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <MPU6050.h>

// --- HARDWARE CONFIGURATION ---
#define PIN_NEOPIXEL 28 // Data IN pin for WS2812B (GP28)
#define NUM_PIXELS 16    // Number of LEDs in your strip/ring
#define SDA_PIN 4       // MPU6050 SDA (GP4)
#define SCL_PIN 5       // MPU6050 SCL (GP5)

// --- PHYSICS ENGINE SETTINGS ---
#define FRICTION 0.94    // Damping factor (Lower = thick liquid, Higher = watery)
#define SENSITIVITY 0.12 // Sensor sensitivity multiplier
#define BOUNCE -0.5      // Wall bounce coefficient (Negative for direction flip)

// --- OBJECTS ---
Adafruit_NeoPixel strip(NUM_PIXELS, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);
MPU6050 mpu;

// --- SHARED VARIABLES (Volatile for Dual-Core Safety) ---
volatile float water_position = NUM_PIXELS / 2.0; // Start in the middle
volatile float water_velocity = 0.0;
volatile bool is_sensor_connected = false;

// Flags for visual effects triggered by physics
volatile bool hit_left_wall = false;
volatile bool hit_right_wall = false;

float calibration_offset = 0;
// ==========================================
// CORE 0: PHYSICS & SENSOR LOGIC
// ==========================================
void setup()
{
  Serial.begin(115200);
  Wire.setSDA(SDA_PIN);
  Wire.setSCL(SCL_PIN);
  Wire.begin();

  // Robustness Check: Initialize sensor
  Serial.println(">> System initializing...");
  mpu.initialize();
  is_sensor_connected = mpu.testConnection();

  if (is_sensor_connected)
  {
    Serial.println(">> MPU6050 Connection Successful!");
    // Read the sensor's current incorrect position 100 times and take the average
    long sum = 0;
    for (int i = 0; i < 100; i++)
    {
      int16_t ax, ay, az;
      mpu.getAcceleration(&ax, &ay, &az);
      sum += ax;
      delay(5);
    }
    calibration_offset = sum / 100.0;
    Serial.print(">> calibration is done. Offset: ");
    Serial.println(calibration_offset);
  }
  else
  {
    Serial.println(">> WARNING: MPU6050 not found! Simulation Mode Activated.");
  }
}

void loop()
{
  float tilt = 0;

  // 1. Input Handling (Sensor vs. Simulation)
  if (is_sensor_connected)
  {
    int16_t ax, ay, az;
    mpu.getAcceleration(&ax, &ay, &az);
    // Map accelerometer X-axis to tilt value (Adjust map range if direction is inverted)
    // Subtract the calibration offset from the raw data. This way, the result is exactly 0 when the sensor is on the table.
    float adjusted_ax = ax - calibration_offset;

    tilt = map(adjusted_ax, -17000, 17000, 10, -10) / 10.0;
  }
  else
  {
    tilt = sin(millis() / 700.0) * 2.5;
  }

  // 2. Physics Calculations (Lagrangian mechanics simplified)
  // Apply Force (F = ma)
  if (abs(tilt) > 0.1)
  { // Deadzone filter for noise
    water_velocity += tilt * SENSITIVITY;
  }

  water_velocity *= FRICTION;       // Apply damping
  water_position += water_velocity; // Update position

  // 3. Boundary Checks & Bounce Logic
  if (water_position < 0)
  {
    water_position = 0;
    water_velocity *= BOUNCE;
    // Trigger visual flash if impact is hard enough
    if (abs(water_velocity) > 0.1)
      hit_left_wall = true;
  }

  if (water_position > NUM_PIXELS - 1)
  {
    water_position = NUM_PIXELS - 1;
    water_velocity *= BOUNCE;
    // Trigger visual flash if impact is hard enough
    if (abs(water_velocity) > 0.1)
      hit_right_wall = true;
  }

  delay(15); // Physics loop rate (~60Hz)
}

// ==========================================
// CORE 1: RENDERING & VISUALIZATION
// ==========================================
// Note: setup1() and loop1() automatically run on the second core of the Pi Pico.

void setup1()
{
  strip.begin();
  strip.setBrightness(30); // Adjust brightness (0-255)
  strip.show();
}

void loop1()
{
  // A. TRAIL EFFECT (Dimming)
  // Instead of clearing, dim existing pixels to create a motion blur trail
  for (int i = 0; i < NUM_PIXELS; i++)
  {
    uint32_t c = strip.getPixelColor(i);
    uint8_t r = (uint8_t)(c >> 16);
    uint8_t g = (uint8_t)(c >> 8);
    uint8_t b = (uint8_t)c;
    strip.setPixelColor(i, strip.Color(r * 0.6, g * 0.6, b * 0.6));
  }

  // B. VELOCITY SHIMMER
  // Change color based on speed: Blue (Slow) -> Turquoise/White (Fast/Frothy)
  int speed_factor = constrain(abs(water_velocity) * 150, 0, 255);
  uint32_t water_color = strip.Color(speed_factor, 100 + (speed_factor / 2), 255);

  // Draw the water center
  int center_pixel = (int)water_position;

  // Safety check to prevent out-of-bounds errors
  if (center_pixel >= 0 && center_pixel < NUM_PIXELS)
  {
    strip.setPixelColor(center_pixel, water_color);

    // Anti-aliasing / Bleeding to adjacent pixels for smoothness
    if (center_pixel > 0)
      strip.setPixelColor(center_pixel - 1, water_color);
    if (center_pixel < NUM_PIXELS - 1)
      strip.setPixelColor(center_pixel + 1, water_color);
  }

  // C. WALL BOUNCE FLASH
  // Check flags set by Core 0
  if (hit_left_wall)
  {
    strip.setPixelColor(0, strip.Color(255, 255, 255)); // Flash White
    hit_left_wall = false;
  }
  if (hit_right_wall)
  {
    strip.setPixelColor(NUM_PIXELS - 1, strip.Color(255, 255, 255)); // Flash White
    hit_right_wall = false;
  }

  strip.show();

  // D. ASCII VISUALIZATION (Serial Monitor)
  // Render visual representation without using line indexes
  String visual = "[";
  for (int i = 0; i < NUM_PIXELS; i++)
  {
    if (i == center_pixel)
      visual += "O";
    else if (i == center_pixel - 1 || i == center_pixel + 1)
      visual += "~"; // Wave
    else
      visual += " ";
  }
  visual += "]";

  // Print single line status
  Serial.print("Vel: ");
  Serial.print(water_velocity);
  Serial.print(" \t");
  Serial.println(visual);

  delay(30); // Visual update rate (~30FPS)
}
