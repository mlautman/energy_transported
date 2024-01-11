#include <memory>

#include <FastLED.h>
#include <Arduino.h>

#include "./TouchState.hpp"
#include "./util.hpp"
FASTLED_USING_NAMESPACE


// Touch Sensing
#define TOUCH_PIN_0 T0 // Typically GPIO4
#define TOUCH_PIN_3 T3 // Typically GPIO15
#define OUTPUT_PIN GPIO_NUM_32
#define INPUT_PIN GPIO_NUM_36
#define TOUCH_THRESHOLD 12

// FAST LED
#define DATA_PIN    22
#define LED_TYPE    WS2811
#define COLOR_ORDER RGB


#define NUM_LEDS    100
CRGB leds[NUM_LEDS];

#define BRIGHTNESS          96
#define FRAMES_PER_SECOND  120

#define NO_TOUCH 0
#define LEFT_ONLY 1
#define RIGHT_ONLY 2
#define BOTH_NO_CON 3
#define BOTH_CON 4

#define DEBUG


const unsigned long loop_period_ms = 50;
const unsigned long max_loop_period_ms = 75;  // Tolerance for timing, e.g., milliseconds
const double loop_period = ((double)loop_period_ms) / 1000.0; 
const double max_loop_period = ((double)loop_period_ms) / 1000.0 + 0.010; 
const double loop_frequency = 1.0 / (loop_period);


void setupLED() {
  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);
}
TouchState* ts;
void setup() {
  delay(1000); // 1 second delay for recovery
  setupLED();

  //Setup touch sensing
  ts =  new TouchState(TOUCH_PIN_0, TOUCH_PIN_3, INPUT_PIN, OUTPUT_PIN);

#ifdef DEBUG
  Serial.begin(115200);
#endif
}

uint8_t gHue = 0; // rotating "base color" used by many of the patterns
typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = {rainbowWithGlitter, bpm,  juggle};
uint visualization_count = 0;

void loop() {
  maintainTiming();
  bool state_updated = ts->update();
  #ifdef DEBUG
  ts->printFullState();
  #endif
  if (state_updated)
    visualization_count = random16(3);
  showAnimation(ts);

  // do some periodic updates
  //  TODO: how does this work?
  EVERY_N_MILLISECONDS( 2 ) {
    gHue+=3;  // slowly cycle the "base color" through the rainbow
  }

  checkTimingViolation();
}

unsigned long previous_millis_ms = 0;
volatile uint16_t count;
void maintainTiming() {
  while (millis() - previous_millis_ms < loop_period_ms) {
    // Wait until the interval has passed
    count++;
  }
  previous_millis_ms = millis();
}

void checkTimingViolation() {
  unsigned long current_millis = millis();
  if (current_millis - previous_millis_ms > max_loop_period_ms) {
#ifdef DEBUG
    Serial.print("Timing violation: Loop running too slow!\t");
    Serial.print(current_millis);
    Serial.print(" - ");
    Serial.print(previous_millis_ms);
    Serial.print(" > ");
    Serial.println(max_loop_period_ms);
    
#endif
  }
}

void showAnimation(TouchState* ts) {
  if (ts->getState() == NO_TOUCH) {
    occasionalTwinkle();
  } else if (ts->getState() == LEFT_ONLY) {
    leftTwinkle(ts->getLeftCapValue());
  } else if (ts->getState() == RIGHT_ONLY) {
    rightTwinkle(ts->getRightCapValue());
  } else if (ts->getState() == BOTH_NO_CON) {
    bothTwinkle(ts->getLeftCapValue(), ts->getRightCapValue());
  } else if (ts->getState() == BOTH_CON) {
    gPatterns[visualization_count]();
  } else {
    // This should never happen
    occasionalTwinkle();
  }

  // send the 'leds' array out to the actual LED strip
  FastLED.show();
  // insert a delay to keep the framerate modest
//  FastLED.delay(1);
}


void rainbow()
{
  // FastLED's built-in rainbow generator
  fill_rainbow( leds, NUM_LEDS, gHue, 7);
}

void rainbowWithGlitter()
{
  // built-in FastLED rainbow, plus some random sparkly glitter
  rainbow();
  addGlitter(80);
}

void addGlitter( fract8 chanceOfGlitter)
{
  if ( random8() < chanceOfGlitter) {
    leds[ random16(NUM_LEDS) ] += CRGB::White;
  }
}

void sinelon()
{
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy( leds, NUM_LEDS, 20);
  int pos = beatsin16( 13, 0, NUM_LEDS - 1 );
  leds[pos] += CHSV( gHue, 255, 192);
}

void bpm()
{
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t BeatsPerMinute = 62;
  CRGBPalette16 palette = PartyColors_p;
  uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
  for ( int i = 0; i < NUM_LEDS; i++) { //9948
    leds[i] = ColorFromPalette(palette, gHue + (i * 2), beat - gHue + (i * 10));
  }
}

void juggle() {
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy( leds, NUM_LEDS, 10);
  uint8_t dothue = 0;
  for ( int i = 0; i < 8; i++) {
    leds[beatsin16( i + 7, 0, NUM_LEDS - 1 )] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
}

#define LEFT_START 0
#define RIGHT_START 100
#define TWINKLE_RANGE 50

void bothTwinkle(double left, double right)
{
  leftTwinkle(left);
  rightTwinkle(right);
}

void leftTwinkle(double scale)
{
  applyTwinkleEffect(scale, LEFT_START, TWINKLE_RANGE);
}
void rightTwinkle(double scale)
{
  applyTwinkleEffect(scale, RIGHT_START, -TWINKLE_RANGE); // Negative k for reverse effect
}

CRGB getVariedWhiteColor() {
  // Base white color
  byte red = 255;
  byte green = 255;
  byte blue = 255;

  // Randomize a bit for variation
  int random_range = 100;
  red -= random8(0, random_range);   // Subtract up to 20 from 255
  green -= random8(0, random_range);
  blue -= random8(0, random_range);

  return CRGB(red, green, blue);
}

void occasionalTwinkle()
{
  applyTwinkleEffect(1./25., 0, NUM_LEDS);
}

void applyTwinkleEffect(double scale, int startLed, int range) {
  // always slowly fade
  fadeToBlackBy( leds, NUM_LEDS, 50);

  // randomly keep twinkles per sec reasonable

  double twinkles_per_sec = 25 * scale;
  int loops_per_twinkle = loop_frequency / twinkles_per_sec;
  #ifdef DEBUG
  Serial.print("loop_frequency:\t");
  Serial.print(loop_frequency);
  Serial.print("\ttwinkles_per_sec:\t");
  Serial.print(twinkles_per_sec);
  Serial.print("\tloops_per_twinkle:\t");
  Serial.println(loops_per_twinkle);
  #endif

  if ( loops_per_twinkle < 1 || random16(loops_per_twinkle) == 0) {
    int pos = random16(abs(range));
    int ledIndex = startLed + pos * (range > 0 ? 1 : -1);
    
    double brightnessFactor = cos(PI * (double(pos) / abs(range)));
    brightnessFactor = (brightnessFactor + 1) / 2; // Adjust to be strictly positive

    CRGB color = getVariedWhiteColor();
    color.nscale8_video((uint8_t)(brightnessFactor * 255));
    leds[ledIndex] = color;  }
}
