#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <MPU6050.h>
#include <math.h> 

#define PIN_NEOPIXEL 26  // Data IN pin for WS2812B (GP26)
#define NUM_PIXELS 60     // Number of LEDs in the ring
#define SDA_PIN 4         // MPU6050 SDA (GP4)
#define SCL_PIN 5         // MPU6050 SCL (GP5)

// --- PHYSICS AND FILTER SETTINGS ---
#define FRICTION 0.64     // Damping factor (Lower = thicker liquid, stops faster)
#define SENSITIVITY 0.03  // Reactivity to tilt
#define DEADZONE 0.15     // Noise filter threshold (radians)

// FILTER SETTING (Between 0.05 - 1.0)
// 0.10 - 0.20 is ideal for water simulation.
#define FILTER_ALPHA 0.15 

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

// Memory variables for filtering
volatile float filtered_ax = 0;
volatile float filtered_ay = 0;

// SORTING ALGORITHM 
// Standard Bubble Sort to organize sensor readings from low to high
void sortArray(int16_t *arr, int n) {
  for (int i = 0; i < n - 1; i++) {
    for (int j = 0; j < n - i - 1; j++) {
      if (arr[j] > arr[j + 1]) {
        int16_t temp = arr[j];
        arr[j] = arr[j + 1];
        arr[j + 1] = temp;
      }
    }
  }
}

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
    Serial.println(">> STARTING ROBUST CALIBRATION (SORTING & TRIMMING)...");
    Serial.println(">> Please hold the sensor still.");
    
    is_calibrating = true; 
    
    // Define sample size and storage arrays
    const int num_samples = 200;
    int16_t readings_x[num_samples];
    int16_t readings_y[num_samples];
    
    // Collect raw data to analyze noise patterns
    for (int i = 0; i < num_samples; i++) 
    {
      int16_t ax, ay, az;
      mpu.getAcceleration(&ax, &ay, &az);
      readings_x[i] = ax;
      readings_y[i] = ay;
      delay(5); 
    }
    
    // Sort arrays to push outliers (spikes/noise) to the edges
    sortArray(readings_x, num_samples);
    sortArray(readings_y, num_samples);

    // We only average the middle 20% of the data (indices 80 to 120)
    // This ignores extreme low and high values caused by vibration.
    long sum_x = 0;
    long sum_y = 0;
    int valid_sample_count = 0;

    // Start from index 80 and end at 120 (taking the most stable center)
    for (int i = 80; i < 120; i++) 
    {
      sum_x += readings_x[i];
      sum_y += readings_y[i];
      valid_sample_count++;
    }
    
    offset_x = sum_x / (float)valid_sample_count;
    offset_y = sum_y / (float)valid_sample_count;
    
    // INITIAL POSITION FIX & FILTER INIT
    // Initialize filter with current stable value to prevent startup "lag"
    int16_t ax, ay, az;
    mpu.getAcceleration(&ax, &ay, &az);
    
    filtered_ax = ax - offset_x;
    filtered_ay = ay - offset_y;

    water_angle = atan2(-filtered_ax, -filtered_ay);

    is_calibrating = false; 
    Serial.println(">> Calibration Complete. Physics Engine Started.");
  }
  else
  {
    is_calibrating = false; 
    Serial.println(">> NO SENSOR FOUND! Advanced Simulation Mode Activated.");
  }
}
void loop()
{
  float target_angle = 0;

  if (is_sensor_connected)
  {
    // --- REAL SENSOR READING AND FILTERING ---
    int16_t raw_ax, raw_ay, raw_az;
    mpu.getAcceleration(&raw_ax, &raw_ay, &raw_az);

    // 1. Calibration (Apply Offset)
    float current_ax = raw_ax - offset_x;
    float current_ay = raw_ay - offset_y;
    
    // 2. Exponential Moving Average (EMA) Filter
    filtered_ax = (FILTER_ALPHA * current_ax) + ((1.0 - FILTER_ALPHA) * filtered_ax);
    filtered_ay = (FILTER_ALPHA * current_ay) + ((1.0 - FILTER_ALPHA) * filtered_ay);
    
    // 3. Calculate angle using FILTERED data
    target_angle = atan2(filtered_ax, filtered_ay); 
  }
  else
  {
    // --- SIMULATION MODE (GHOST HAND) ---    
    float time_val = millis() / 1000.0;
    
    // Mixing sine waves at two different frequencies:
    // Wave 1: Main gravity change (Slow)
    // Wave 2: Hand jitter / Noise (Fast)
    float simulated_gravity = sin(time_val * 1.5) * 0.6 + sin(time_val * 3.7) * 0.3;
    
    target_angle = simulated_gravity * PI; 
  }

  // --- PHYSICS ENGINE ---
  
  // Calculate shortest rotation path
  float diff = target_angle - water_angle;
  while (diff <= -PI) diff += 2*PI;
  while (diff > PI) diff -= 2*PI;
  
  if (abs(diff) > DEADZONE) { 
      angular_velocity += diff * SENSITIVITY;
  } else {
      angular_velocity *= 0.5; // Slow down inside deadzone
  }

  angular_velocity *= FRICTION;    
  water_angle += angular_velocity; 

  // Normalize Angle (-PI to +PI)
  if (water_angle > PI) water_angle -= 2*PI;
  if (water_angle <= -PI) water_angle += 2*PI;

  delay(15); 
}

// CORE 1: VISUALIZATION AND LED CONTROL
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
    
    strip.setPixelColor(0, calib_color);               
    strip.setPixelColor(NUM_PIXELS/4, calib_color);    
    strip.setPixelColor(NUM_PIXELS/2, calib_color);    
    strip.setPixelColor((NUM_PIXELS*3)/4, calib_color); 
    
    strip.show();
    delay(50); 
    return; 
  }

  // TRAIL EFFECT - Dim old pixels by 20%
  for (int i = 0; i < NUM_PIXELS; i++)
  {
    uint32_t c = strip.getPixelColor(i);
    uint8_t r = (uint8_t)(c >> 16);
    uint8_t g = (uint8_t)(c >> 8);
    uint8_t b = (uint8_t)c;
    strip.setPixelColor(i, strip.Color(r * 0.80, g * 0.80, b * 0.80));
  }

  // POSITION MAPPING 
  float normalized_angle = water_angle + PI; 
  int center_pixel = (int)((normalized_angle / (2*PI)) * NUM_PIXELS) % NUM_PIXELS;

  // COLOR DYNAMICS (Color change based on speed)
  int speed = constrain(abs(angular_velocity) * 600, 0, 255);

  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;

  if (speed < 50) {
    int breath = (sin(millis() / 500.0) + 1) * 20; 
    
    r = 0;
    g = 10 + breath;       
    b = 150 + (breath*2); 
    
  } else {
    r = speed;             
    g = 100 + (speed / 2); 
    b = 255;               
  }

  uint32_t water_color = strip.Color(r, g, b);

  // DRAW WATER DROP (Soft-edged 3 pixels)
  strip.setPixelColor(center_pixel, water_color);
  
  int left = (center_pixel - 1 + NUM_PIXELS) % NUM_PIXELS;
  int right = (center_pixel + 1) % NUM_PIXELS;
  
  uint32_t side_color = strip.Color(r*0.7, g*0.7, b*0.7);
  
  strip.setPixelColor(left, side_color);
  strip.setPixelColor(right, side_color);

  strip.show();

// SERIAL MONITOR VISUALIZATION (ASCII)
  String visual = "[";
  for (int i = 0; i < NUM_PIXELS; i++)
  {
    if (i == center_pixel) visual += "O";
    else if (i == left || i == right) visual += "~";
    else visual += " ";
  }
  visual += "]";

  Serial.print("X: ");
  Serial.print(filtered_ax, 1);
  Serial.print("\t Y: ");
  Serial.print(filtered_ay, 1);
  Serial.print("\t Vel: ");
  Serial.print(angular_velocity, 2);
  Serial.print("\t ");
  Serial.println(visual);

  delay(30);
}
