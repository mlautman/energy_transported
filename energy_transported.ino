
#include <FastLED.h>
FASTLED_USING_NAMESPACE

// Touch Sensing
#define TOUCH_PIN_0 T0 // Typically GPIO4
#define TOUCH_PIN_3 T3 // Typically GPIO15
#define OUTPUT_PIN GPIO_NUM_32
#define INPUT_PIN GPIO_NUM_36
#define TOUCH_THRESHOLD 15

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


#define DEBUG 1

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

void setupLED(){
  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  //FastLED.addLeds<LED_TYPE,DATA_PIN,CLK_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);
}

void setupTouch(){
  touchAttachInterrupt(TOUCH_PIN_0, nullptr, 0); // Dummy interrupt to activate touch on GPIO4
  touchAttachInterrupt(TOUCH_PIN_3, nullptr, 0); // Dummy interrupt to activate touch on GPIO15
  // we set the output pin and input to input but without 
  // a pullup/down resistor so that it minimizes effect the 
  // capacitive touch sensors.
  pinMode(OUTPUT_PIN, INPUT);
  pinMode(INPUT_PIN, INPUT);  
}

void setup() {
  delay(3000); // 3 second delay for recovery
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
  //  Allow time for pins to settle
  nbDelay(1);

  // Set the IO pins
  pinMode(OUTPUT_PIN, OUTPUT);
  pinMode(INPUT_PIN, INPUT);
  //  Settle
  nbDelay(1);

  // Set output to high   
  digitalWrite(OUTPUT_PIN, HIGH);
 
  // Settle 
  nbDelay(1);
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
  
  // Settle  
  delay(2);
  
  return inputState;
}

uint16_t touchState()
{
  uint16_t T0_val = touchRead(TOUCH_PIN_0);
  uint16_t T3_val = touchRead(TOUCH_PIN_3);

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
  else if (T0_val <= TOUCH_THRESHOLD && T3_val <= TOUCH_THRESHOLD) 
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

unsigned long previousMillis = 0;
const long interval = 10; // Interval of 100 milliseconds
const long maxInterval = 10; // Tolerance for timing, e.g., 25 milliseconds


void maintainTiming(){
  while (millis() - previousMillis < interval) {
    // Wait until the interval has passed
  }
  previousMillis = millis();
}

void checkTimingViolation() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis > maxInterval) {
    #ifdef DEBUG
    Serial.println("Timing violation: Loop running too slow!");
    #endif
  }
}

typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = {confetti, leftTwinkle,  rightTwinkle,  bothTwinkle, juggle};
uint8_t gHue = 0; // rotating "base color" used by many of the patterns
uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current

void setAnimation(uint16_t state){
  gCurrentPatternNumber = state;
}

void showAnimation(){
  // Call the current pattern function once, updating the 'leds' array
  gPatterns[gCurrentPatternNumber]();

  // send the 'leds' array out to the actual LED strip
  FastLED.show();  
  // insert a delay to keep the framerate modest
//  FastLED.delay(1000/FRAMES_PER_SECOND); 
}

void loop() {
  maintainTiming(); 
  uint16_t touchStatus = touchState();

  setAnimation(touchStatus);
  showAnimation();

  // do some periodic updates
  //  TODO: how does this work?
//  EVERY_N_MILLISECONDS( 20 ) { gHue++; } // slowly cycle the "base color" through the rainbow

  #ifdef DEBUG
  String touchString = touchToString(touchStatus);
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
  if( random8() < chanceOfGlitter) {
    leds[ random16(NUM_LEDS) ] += CRGB::White;
  }
}

void confetti() 
{
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy( leds, NUM_LEDS, 10);
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV( gHue + random8(64), 200, 255);
}

void sinelon()
{
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy( leds, NUM_LEDS, 20);
  int pos = beatsin16( 13, 0, NUM_LEDS-1 );
  leds[pos] += CHSV( gHue, 255, 192);
}

void bpm()
{
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t BeatsPerMinute = 62;
  CRGBPalette16 palette = PartyColors_p;
  uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
  for( int i = 0; i < NUM_LEDS; i++) { //9948
    leds[i] = ColorFromPalette(palette, gHue+(i*2), beat-gHue+(i*10));
  }
}

void juggle() {
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy( leds, NUM_LEDS, 20);
  uint8_t dothue = 0;
  for( int i = 0; i < 8; i++) {
    leds[beatsin16( i+7, 0, NUM_LEDS-1 )] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
}

#define LEFT_START 5
#define RIGHT_START 40
#define TWINKLE_RANGE 15

void bothTwinkle() 
{
  FastLED.clear(); 
  applyTwinkleEffect(LEFT_START, TWINKLE_RANGE);
  applyTwinkleEffect(RIGHT_START, -TWINKLE_RANGE); // Negative k for reverse effect
}

void leftTwinkle()
{
  FastLED.clear(); 
  applyTwinkleEffect(LEFT_START, TWINKLE_RANGE);
}
void rightTwinkle()
{
  FastLED.clear(); 
  applyTwinkleEffect(RIGHT_START, -TWINKLE_RANGE); // Negative k for reverse effect  
}

CRGB getVariedWhiteColor() {
  // Base white color
  byte red = 255;
  byte green = 255;
  byte blue = 255;

  // Randomize a bit for variation
  red -= random8(0, 20);   // Subtract up to 20 from 255
  green -= random8(0, 20); 
  blue -= random8(0, 20);

  return CRGB(red, green, blue);
}

void applyTwinkleEffect(int startLed, int range) {
  for (int i = 0; i <= abs(range); i++) {
    int ledIndex = startLed + i * (range > 0 ? 1 : -1);
    if (ledIndex >= 0 && ledIndex < NUM_LEDS) {
      float brightness = 255 * (1 - (float)i / abs(range));
      leds[ledIndex] = getVariedWhiteColor();
      leds[ledIndex].fadeToBlackBy(255 - (int)brightness);
    }
  }
}
