
#include <FastLED.h>
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


const unsigned long loop_period_ms = 30;
const unsigned long max_loop_period_ms = 10;  // Tolerance for timing, e.g., milliseconds
const double loop_period = ((double)loop_period_ms) / 1000.0; 
const double max_loop_period = ((double)loop_period_ms) / 1000.0 + 0.010; 
const double loop_frequency = 1.0 / (loop_period);


String touchToString(int touchType) {
  switch (touchType) {
    case NO_TOUCH:
      return "NO_TOUCH";
    case LEFT_ONLY:
      return "LEFT_ONLY";
    case RIGHT_ONLY:
      return "RIGHT_ONLY";
    case BOTH_NO_CON:
      return "BOTH_NO_CON";
    case BOTH_CON:
      return "BOTH_CON";
    default:
      return "UNKNOWN";
  }
}

void setupLED() {
  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  //FastLED.addLeds<LED_TYPE,DATA_PIN,CLK_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);
}

void setupTouch() {
  touchAttachInterrupt(TOUCH_PIN_0, nullptr, 0); // Dummy interrupt to activate touch on GPIO4
  touchAttachInterrupt(TOUCH_PIN_3, nullptr, 0); // Dummy interrupt to activate touch on GPIO15
  // we set the output pin and input to input but without
  // a pullup/down resistor so that it minimizes effect the
  // capacitive touch sensors.
  pinMode(OUTPUT_PIN, INPUT);
  pinMode(INPUT_PIN, INPUT);
}

void setup() {
  delay(1000); // 1 second delay for recovery
  setupLED();

  //Setup touch sensing
  setupTouch();

#ifdef DEBUG
  Serial.begin(115200);
#endif
}


void nbDelay(unsigned long interval) {
  unsigned long initial_milli = millis();
  while (millis() - initial_milli < interval) {
    // Loop until the interval has passed
  }
  return;
}

uint16_t connectionState()
{
//  We tried this to disable the hardware touch sensing on those pins but unfortunately it didn't work
  pinMode(GPIO_NUM_15, INPUT);
  pinMode(GPIO_NUM_4, INPUT);
  //  Allow time for pins to settle
  nbDelay(2);

  // Set the IO pins
  pinMode(OUTPUT_PIN, OUTPUT);
  pinMode(INPUT_PIN, INPUT_PULLDOWN);
  //  Settle
  nbDelay(2);

  // Set output to high
  digitalWrite(OUTPUT_PIN, HIGH);

  // Settle
  nbDelay(2);
  // Read the input
  int inputState = digitalRead(INPUT_PIN);
  // Set the output pin to low
  digitalWrite(OUTPUT_PIN, LOW);
  nbDelay(1);
  // Settle

  // we set the output pin and input to input but without
  // a pullup/down resistor so that it does not affect the
  // capacitive touch sensors.
  pinMode(OUTPUT_PIN, INPUT);
  pinMode(INPUT_PIN, INPUT);
  nbDelay(1);

  touchAttachInterrupt(TOUCH_PIN_0, nullptr, 0); // Dummy interrupt to activate touch on GPIO4
  touchAttachInterrupt(TOUCH_PIN_3, nullptr, 0); // Dummy interrupt to activate touch on GPIO15
  nbDelay(1);

  // Settle
  delay(10);

  return inputState;
}

const uint16_t history_len = 3;
uint16_t output_history[history_len] = {0}; // Correct array declaration
uint16_t current_state = 0;

uint16_t getFilteredState(uint16_t new_state) {
  // Shift the history
  for (int i = 1; i < history_len; i++) {
    output_history[i - 1] = output_history[i];
  }
  output_history[history_len - 1] = new_state;

  // Check if all elements are the same as the new state
  bool all_same = true;
  for (int i = 0; i < history_len; i++) {
    if (output_history[i] != new_state) {
      all_same = false;
      break;
    }
  }

  if (all_same) {
    current_state = new_state;
  }

  return current_state;
}

uint16_t getTouchState()
{
  uint16_t T0_val = touchRead(TOUCH_PIN_0);
  uint16_t T3_val = touchRead(TOUCH_PIN_3);
  uint16_t touch_state = 0;

  if (T0_val > TOUCH_THRESHOLD && T3_val > TOUCH_THRESHOLD)
  {
    return NO_TOUCH;
  }
  else if (T0_val <= TOUCH_THRESHOLD && T3_val > TOUCH_THRESHOLD)
  {
    return LEFT_ONLY;
  }
  else if (T0_val > TOUCH_THRESHOLD && T3_val <= TOUCH_THRESHOLD)
  {
    return RIGHT_ONLY;
  }
  else // if (T0_val <= TOUCH_THRESHOLD && T3_val <= TOUCH_THRESHOLD)
  {
    // Both are getting touched. Now run the connection sensing
    uint16_t complete_circuit = connectionState();
    if (complete_circuit)
    {
      return BOTH_CON;
    }
    else
    {
      return BOTH_NO_CON;
    }
  }
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

typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = {noAnimation, leftTwinkle,  rightTwinkle,  bothTwinkle, bpm};
uint8_t gHue = 0; // rotating "base color" used by many of the patterns
uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current

void setAnimation(uint16_t state) {
  gCurrentPatternNumber = state;
}

void showAnimation() {
  // Call the current pattern function once, updating the 'leds' array
  gPatterns[gCurrentPatternNumber]();

  // send the 'leds' array out to the actual LED strip
  FastLED.show();
  // insert a delay to keep the framerate modest
//  FastLED.delay(1);
}


void loop() {
  maintainTiming();
#ifdef DEBUG
  Serial.print("loop start at:\t");
  Serial.println(millis());
#endif
  uint16_t touchState = getTouchState();
  touchState = getFilteredState(touchState);

  setAnimation(touchState);
  showAnimation();

  // do some periodic updates
  //  TODO: how does this work?
  EVERY_N_MILLISECONDS( 20 ) {
    gHue+=10;  // slowly cycle the "base color" through the rainbow
  }

#ifdef DEBUG
  String touchString = touchToString(touchState);
  Serial.println(touchString);
#endif
  checkTimingViolation();
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
//void bpmGlitter()
//{
//  bpm();
//  addGlitter(80);
//}

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

void bothTwinkle()
{
  #ifdef DEBUG 
  Serial.println("applying both twinkles");
  #endif
  applyTwinkleEffect(LEFT_START, TWINKLE_RANGE);
  applyTwinkleEffect(RIGHT_START, -TWINKLE_RANGE); // Negative k for reverse effect
}

void leftTwinkle()
{
  #ifdef DEBUG 
  Serial.println("applying left twinkle ");
  #endif
  applyTwinkleEffect(LEFT_START, TWINKLE_RANGE);
}
void rightTwinkle()
{
  #ifdef DEBUG 
  Serial.println("applying right twinkle ");
  #endif
  applyTwinkleEffect(RIGHT_START, -TWINKLE_RANGE); // Negative k for reverse effect
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

void noAnimation()
{
  fadeToBlackBy(leds, NUM_LEDS, 50);
  //  FastLED.clear();
}

void applyTwinkleEffect(int startLed, int range) {
  #ifdef DEBUG 
  Serial.println("applying twinkle effect");
  #endif
  // always slowly fade
  fadeToBlackBy( leds, NUM_LEDS, 50);

  // randomly keep twinkles per sec reasonable
  double twinkles_per_sec = 30;
  int loops_per_twinkle = loop_frequency / twinkles_per_sec;
  #ifdef DEBUG
  Serial.print("loop_frequency:\t");
  Serial.println(loop_frequency);
  Serial.print("loops_per_twinkle:\t");
  Serial.println(loops_per_twinkle);
  #endif

  if ( loops_per_twinkle < 1 || random16(loops_per_twinkle) == 0) {
    int pos = random16(abs(range));
    int ledIndex = startLed + pos * (range > 0 ? 1 : -1);
    
    double brightnessFactor = cos(PI * (double(pos) / abs(range)));
    brightnessFactor = (brightnessFactor + 1) / 2; // Adjust to be strictly positive
//    if (range<0)
//      brightnessFactor=0;
    CRGB color = getVariedWhiteColor();
    color.nscale8_video((uint8_t)(brightnessFactor * 255));
    leds[ledIndex] = color;  }
}
