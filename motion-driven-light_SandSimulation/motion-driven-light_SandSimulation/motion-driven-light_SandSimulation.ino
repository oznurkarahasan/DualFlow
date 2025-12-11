#include <Adafruit_NeoPixel.h>
#include <Adafruit_MPU6050.h>
#include <Wire.h>

// --- HARDWARE SETTINGS ---
#define PIN_NEOPIXEL 28
#define NUM_LEDS 16

// --- PHYSICS SETTINGS ---
#define TILT_THRESHOLD 1.5    
#define FALL_SPEED 35        

// --- OBJECTS ---
Adafruit_NeoPixel strip(NUM_LEDS, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);
Adafruit_MPU6050 mpu;

// --- SHARED VARIABLES ---
volatile float clean_tilt = 0;        
volatile bool sensor_ready = false;   

// --- HOURGLASS STATE ---
const int TOTAL_SAND = 8;  
int sandLeft = 4;          
int sandRight = 4;         
int fallingPixelPos = -1;  
int fallingDirection = 0;  

// Drop Color
uint32_t currentDropColor = 0; 

// CORE 0: SENSOR & CALIBRATION
,float calibration_offset = 0;

void setup() {
  Serial.begin(115200);
  delay(2000); // Wait for Serial to initialize
  
  Serial.println("\n\n=================================");
  Serial.println(">> SYSTEM STARTUP: INITIALIZING...");
  
  Wire.setSDA(4);
  Wire.setSCL(5);
  Wire.begin();

  if (!mpu.begin()) {
    Serial.println(">> ERROR: MPU6050 NOT FOUND!");
    Serial.println(">> CHECK WIRING (SDA->GP4, SCL->GP5)");
    while(1) delay(100); 
  }

  Serial.println(">> MPU6050 CONNECTED.");
  Serial.println(">> CALIBRATING SENSOR (KEEP FLAT)...");

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  long sum = 0;
  for (int i = 0; i < 100; i++) {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    sum += a.acceleration.x * 100; 
    delay(5);
  }
  
  calibration_offset = (sum / 100.0) / 100.0; 
  sensor_ready = true;
  
  Serial.print(">> CALIBRATION DONE. OFFSET: ");
  Serial.println(calibration_offset);
  Serial.println(">> ANIMATION STARTING...");
  Serial.println("=================================\n");
}

void loop() {
  if (sensor_ready) {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    clean_tilt = a.acceleration.x - calibration_offset; 
  }
  delay(10); 
}

// CORE 1: ANIMATION & SERIAL OUTPUT
void setup1() {
  strip.begin();
  strip.show();
  strip.setBrightness(20); 
  
  while(!sensor_ready) delay(100);
  strip.clear();
  randomSeed(analogRead(26)); 
}

void playExplosionEffect(int impactPixel, int direction) {
  // Color Palette
  uint32_t colorFlash = strip.Color(255, 255, 255); // WHITE (Impact)
  uint32_t colorHeat  = strip.Color(255, 100, 0);   // ORANGE (Energy)
  uint32_t colorCool  = strip.Color(0, 255, 255);   // TURQUOISE (Cooling)
  uint32_t colorWater = strip.Color(0, 0, 255);     // BLUE (Water)

  strip.setPixelColor(impactPixel, colorFlash);
  int splashPixel = impactPixel - direction; 
  if (splashPixel >= 0 && splashPixel < NUM_LEDS) {
      strip.setPixelColor(splashPixel, colorHeat);
  }
  strip.show();
  delay(40); 

  strip.setPixelColor(impactPixel, colorHeat);
  if (splashPixel >= 0 && splashPixel < NUM_LEDS) {
      strip.setPixelColor(splashPixel, colorCool); 
  }
  strip.show();
  delay(40);

  strip.setPixelColor(impactPixel, colorCool);
  if (splashPixel >= 0 && splashPixel < NUM_LEDS) {
      strip.setPixelColor(splashPixel, 0); 
  }
  strip.show();
  delay(40);
}

void loop1() {
  strip.clear();
  
  uint32_t colorWater = strip.Color(0, 0, 255); // Static Water Color

  for(int i=0; i<sandLeft; i++) strip.setPixelColor(i, colorWater);
  for(int i=0; i<sandRight; i++) strip.setPixelColor(NUM_LEDS - 1 - i, colorWater);

  if (fallingPixelPos == -1) {
    
    if (clean_tilt > TILT_THRESHOLD && sandRight > 0) {
      sandRight--;          
      fallingPixelPos = NUM_LEDS - 1 - sandRight; 
      fallingDirection = -1;       
      currentDropColor = strip.ColorHSV(random(0, 65536), 255, 255); 
    }
    else if (clean_tilt < -TILT_THRESHOLD && sandLeft > 0) {
      sandLeft--;           
      fallingPixelPos = sandLeft; 
      fallingDirection = 1;        
      currentDropColor = strip.ColorHSV(random(0, 65536), 255, 255); 
    }
    
  } else {
   
    // Moving Right
    if (fallingDirection == 1) {
      fallingPixelPos++;
      int targetIndex = NUM_LEDS - 1 - sandRight; 
      
      if (fallingPixelPos >= targetIndex) {
        // --- EXPLOSION ---
        playExplosionEffect(fallingPixelPos, 1);
        sandRight++;        
        fallingPixelPos = -1; 
      } else {
        strip.setPixelColor(fallingPixelPos, currentDropColor);
        delay(FALL_SPEED);
      }
    }
    
    // Moving Left
    else if (fallingDirection == -1) {
      fallingPixelPos--;
      int targetIndex = sandLeft; 
      
      if (fallingPixelPos <= targetIndex) {
        // --- EXPLOSION ---
        playExplosionEffect(fallingPixelPos, -1);
        sandLeft++;         
        fallingPixelPos = -1; 
      } else {
        strip.setPixelColor(fallingPixelPos, currentDropColor);
        delay(FALL_SPEED);
      }
    }
  }

  strip.show();

  // SERIAL MONITOR OUTPUT 
  static unsigned long lastPrintTime = 0;
  // Print every 100ms to avoid flooding the Serial Monitor
  if (millis() - lastPrintTime > 100) {
    Serial.print("TILT: "); 
    Serial.print(clean_tilt, 1); // Print tilt with 1 decimal
    Serial.print("\t| STRIP: [");

    // ASCII Visualization Loop
    for(int i=0; i<NUM_LEDS; i++) {
        if (i < sandLeft) {
            Serial.print("O"); 
        } 
        else if (i >= NUM_LEDS - sandRight) {
            Serial.print("O"); 
        }
        else if (i == fallingPixelPos) {
            Serial.print("*"); 
        }
        else {
            Serial.print("."); 
        }
    }
    Serial.print("] | L:");
    Serial.print(sandLeft);
    Serial.print(" R:");
    Serial.println(sandRight);
    
    lastPrintTime = millis();
  }
}
