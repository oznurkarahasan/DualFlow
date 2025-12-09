#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <MPU6050.h>
#include <math.h> 

#define PIN_NEOPIXEL 28   // Data IN pin for WS2812B (GP28)
#define NUM_PIXELS 60     // Number of LEDs in the ring
#define SDA_PIN 4         // MPU6050 SDA (GP4)
#define SCL_PIN 5         // MPU6050 SCL (GP5)

#define FRICTION 0.64     // Damping factor (Lower = thicker liquid, stops faster)
#define SENSITIVITY 0.03  // Reactivity to tilt
#define DEADZONE 0.15     // Noise filter threshold (radians)

Adafruit_NeoPixel strip(NUM_PIXELS, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);
MPU6050 mpu;

// GLOBAL VARIABLES (Volatile for Dual-Core Safety) 
volatile float water_angle = 0.0;        // Current position of water (in Radians)
volatile float angular_velocity = 0.0;   // Current speed of rotation
volatile bool is_sensor_connected = false;

volatile bool is_calibrating = true; 

// Calibration Offsets
float offset_x = 0;
float offset_y = 0;

// CORE 0: PHYSICS & SENSOR LOGIC

void setup()
{
  Serial.begin(115200);
  Wire.setSDA(SDA_PIN);
  Wire.setSCL(SCL_PIN);
  Wire.begin();

  Serial.println(">> System Initializing...");
  mpu.initialize();
  is_sensor_connected = mpu.testConnection();

  if (is_sensor_connected)
  {
    Serial.println(">> MPU6050 Connected!");
    Serial.println(">> STARTING 2-SECOND CALIBRATION (PLEASE HOLD STILL)...");
    
    is_calibrating = true; 
    
    long sum_x = 0;
    long sum_y = 0;
    
    // Take 200 samples (~2 seconds)
    for (int i = 0; i < 200; i++) 
    {
      int16_t ax, ay, az;
      mpu.getAcceleration(&ax, &ay, &az);
      sum_x += ax;
      sum_y += ay;
      delay(10); 
    }
    
    offset_x = sum_x / 200.0;
    offset_y = sum_y / 200.0;
    
    // INITIAL POSITION FIX 
    // Instantly teleport water to the correct gravity location after calibration
    int16_t ax, ay, az;
    mpu.getAcceleration(&ax, &ay, &az);
    float start_ax = ax - offset_x;
    float start_ay = ay - offset_y;
    water_angle = atan2(-start_ax, -start_ay);

    is_calibrating = false; 
    Serial.println(">> Calibration Complete. Physics Engine Started.");
  }
  else
  {
    is_calibrating = false; 
    Serial.println(">> NO SENSOR FOUND! Demo Mode Activated.");
  }
}

void loop()
{
  float target_angle = 0;

  if (is_sensor_connected)
  {
    int16_t raw_ax, raw_ay, raw_az;
    mpu.getAcceleration(&raw_ax, &raw_ay, &raw_az);

    // Apply Calibration
    float ax = raw_ax - offset_x;
    float ay = raw_ay - offset_y;
    
    // Calculate 2D Angle using atan2 (Vector Math)
    // Note: If movement is inverted, remove the minus signs
    target_angle = atan2(-ax, -ay); 
  }
  else
  {
    // Demo Mode (Sine Wave)
    target_angle = sin(millis() / 1000.0) * PI;
  }

  // PHYSICS CALCULATIONS
  
  // Calculate shortest path to target angle
  float diff = target_angle - water_angle;
  while (diff <= -PI) diff += 2*PI;
  while (diff > PI) diff -= 2*PI;
  
  // Apply Deadzone (Anti-Jitter)
  if (abs(diff) > DEADZONE) {
     angular_velocity += diff * SENSITIVITY;
  } else {
     angular_velocity *= 0.5; // Slow down if inside deadzone
  }

  angular_velocity *= FRICTION;   
  water_angle += angular_velocity; 

  // Normalize Angle (-PI to +PI)
  if (water_angle > PI) water_angle -= 2*PI;
  if (water_angle <= -PI) water_angle += 2*PI;

  delay(15); 
}

// CORE 1: RENDERING & VISUALIZATION

void setup1()
{
  strip.begin();
  strip.setBrightness(30); 
  strip.show();
}

void loop1()
{
  // CALIBRATION INDICATOR (Orange Cross)
  if (is_calibrating) {
    strip.clear(); 
    uint32_t calib_color = strip.Color(255, 140, 0); // Orange
    
    // Draw Cross
    strip.setPixelColor(0, calib_color);              
    strip.setPixelColor(NUM_PIXELS/4, calib_color);   
    strip.setPixelColor(NUM_PIXELS/2, calib_color);   
    strip.setPixelColor((NUM_PIXELS*3)/4, calib_color); 
    
    strip.show();
    delay(50); 
    return; 
  }

  // TRAIL EFFECT (Dimming)
  // Dim old pixels by 20% to create a fluid trail
  for (int i = 0; i < NUM_PIXELS; i++)
  {
    uint32_t c = strip.getPixelColor(i);
    uint8_t r = (uint8_t)(c >> 16);
    uint8_t g = (uint8_t)(c >> 8);
    uint8_t b = (uint8_t)c;
    strip.setPixelColor(i, strip.Color(r * 0.80, g * 0.80, b * 0.80));
  }

  // POSITION MAPPING 
  // Convert Angle (Radians) to Pixel Index (0-59)
  float normalized_angle = water_angle + PI; 
  int center_pixel = (int)((normalized_angle / (2*PI)) * NUM_PIXELS) % NUM_PIXELS;

  // CREATIVE OCEAN ENGINE (Color Dynamics)
  
  // Calculate Speed Factor (0-255)
  int speed = constrain(abs(angular_velocity) * 600, 0, 255);

  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;

  if (speed < 50) {
    // Gently pulse the color when water is still
    int breath = (sin(millis() / 500.0) + 1) * 20; 
    
    r = 0;
    g = 10 + breath;      
    b = 150 + (breath*2); 
    
  } else {
    // ACTIVE STATE (Turquoise -> White Foam)
    // As velocity increases, mix in Red and Green to create white froth
    r = speed;             
    g = 100 + (speed / 2); 
    b = 255;              
  }

  uint32_t water_color = strip.Color(r, g, b);


  // RENDER BLOB (Soft Edges) 
  
  // Center Pixel (Brightest)
  strip.setPixelColor(center_pixel, water_color);
  
  int left = (center_pixel - 1 + NUM_PIXELS) % NUM_PIXELS;
  int right = (center_pixel + 1) % NUM_PIXELS;
  
  // Dim neighbors to 70% brightness for depth
  uint32_t side_color = strip.Color(r*0.7, g*0.7, b*0.7);
  
  strip.setPixelColor(left, side_color);
  strip.setPixelColor(right, side_color);

  strip.show();

  // SERIAL MONITOR VISUALIZATION 
  String visual = "[";
  for (int i = 0; i < NUM_PIXELS; i++)
  {
    if (i == center_pixel) visual += "O";
    else if (i == left || i == right) visual += "~";
    else visual += " ";
  }
  visual += "]";

  Serial.print("Vel: ");
  Serial.print(angular_velocity);
  Serial.print(" \t");
  Serial.println(visual);

  delay(30); 
}
