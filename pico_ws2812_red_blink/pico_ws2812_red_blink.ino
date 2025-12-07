#include <Adafruit_NeoPixel.h>

#define PIN_NEOPIXEL 28 // Pico GP28 (Pin 34)
#define NUM_PIXELS 12   // led count

Adafruit_NeoPixel strip(NUM_PIXELS, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);
// Blink red on all WS2812 LEDs every second
void setup()
{
  strip.begin();
  strip.setBrightness(50);
  strip.show();
}

void loop()
{
  for (int i = 0; i < NUM_PIXELS; i++)
  {
    strip.setPixelColor(i, strip.Color(255, 0, 0));
  }
  strip.show();
  delay(1000);

  strip.clear();
  strip.show();
  delay(1000);
}
