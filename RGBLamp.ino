#include <FastLED.h> //for disco mode
#include <Adafruit_NeoPixel.h> //other modes

//The amount of LEDs in the setup
#define NUM_LEDS 144

//The pin that controls the LEDs for Disco (FastLED)
#define LED_PIN 1 // = D1 since ESP is used for the project

//The pin that is used for other modes. Use GPIO description of the pin here 
#define BUTTON_GPIO 12

//Arduino loop delay
#define DELAY 1

//The pin that we read sensor values form
#define ANALOG_READ 0

//Confirmed microphone low value, and max value
#define MIC_LOW 0.0
#define MIC_HIGH 1024.0
/** Other macros */
//How many previous sensor values effects the operating average?
#define AVGLEN 5
//How many previous sensor values decides if we are on a peak/HIGH (e.g. in a song)
#define LONG_SECTOR 20

//Mneumonics
#define HIGH  3
#define NORMAL 2

//How long do we keep the "current average" sound, before restarting the measuring
#define MSECS 2000 //default is 30 * 1000
#define CYCLES MSECS / DELAY

/*Sometimes readings are wrong or strange. How much is a reading allowed
to deviate from the average to not be discarded? **/
#define DEV_THRESH 0.8

float fscale( float originalMin, float originalMax, float newBegin, float newEnd, float inputValue, float curve);
void insert(int val, int *avgs, int len);
int compute_average(int *avgs, int len);
void visualize_music();

//How many LEDs to we display
int curshow = NUM_LEDS;

//Showing different colors based on the mode.
int songmode = NORMAL;

//Average sound measurement the last CYCLES
unsigned long song_avg;

//The amount of iterations since the song_avg was reset
int iter = 0;

//The speed the LEDs fade to black if not relit
float fade_scale = 1.2;

//Led array
CRGB leds[NUM_LEDS];

/*Short sound avg used to "normalize" the input values.
We use the short average instead of using the sensor input directly */
int avgs[AVGLEN] = {-1};

//Longer sound avg
int long_avg[LONG_SECTOR] = {-1};

//Keeping track how often, and how long times we hit a certain mode
struct time_keeping {
  unsigned long times_start;
  short times;
};

//How much to increment or decrement each color every cycle
struct color {
  int r;
  int g;
  int b;
};

struct time_keeping high;
struct color Color; 

//Boolean for choosing disco mode
boolean twoDirections;
//Boolean indicating if button was pressed
boolean buttonPressed = false;
//Intial mode we have when starting up
int selectedMode = 2;

void setup() {
  Serial.begin(9600);

  //Initialize the pushbutton pin as an input:
  pinMode(BUTTON_GPIO, INPUT);
  
  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness(80);

  //Turn off all LEDs
  for (int i = 0; i < NUM_LEDS; i++) 
    leds[i] = CRGB(0, 0, 0);
  FastLED.show(); 
  delay(1000);

  //Bootstrap average with some low values
  for (int i = 0; i < AVGLEN; i++) {  
    insert(250, avgs, AVGLEN);
  }

  //Initial values
  high.times = 0;
  high.times_start = millis();
  Color.r = 0;  
  Color.g = 0;
  Color.b = 1;  
}

void loop() {
  
  //If button is pressed change mode
  if(digitalRead(BUTTON_GPIO) && !buttonPressed){
    buttonPressed = true;

    //Turn off all LEDs
    for (int i = 0; i < NUM_LEDS; i++) 
      leds[i] = CRGB(0, 0, 0);
    
    if(selectedMode >= 4){ //Put number of modes here
      selectedMode = 0;
    } else {
      selectedMode++;
    }

    //Serial.print("Mode is ");
    //Serial.println(selectedMode);
  }

 
  /* 
  0: cold white light 
  1:  warm white light
  2: rainbow
  3: disco mode v1
  4: disco mode v2
  */

  switch (selectedMode) {
    case 0: 
      lightOn(255, 250, 250);
      break;
    case 1: 
      lightOn(255, 147, 41); //Good source: http://planetpixelemporium.com/tutorialpages/light.html
      break;
    case 2: 
      rainbowCycle(20);
      buttonPressed = false;
      break;
    case 3: 
      twoDirections = true;
      visualize_music(twoDirections);
      break;
     case 4:
      twoDirections = false;
      visualize_music(twoDirections);
    default:
      break;
  }
  
  //If button was released
  if(!digitalRead(BUTTON_GPIO)){
    buttonPressed = false;
  }

  //Delay in between reads for stability   
  delay(DELAY);       
}


void lightOn(byte r, byte g, byte b){
  for (int i = 0; i < NUM_LEDS; i++){ 
      leds[i] = CRGB(r, g, b);    
  }
  FastLED.show(); 
}


void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  //Assume that button was not released when entering funcion
  boolean buttonPressed = true;
  
  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel

    //If button is released
    if (!digitalRead(BUTTON_GPIO)) {
      buttonPressed = false;
    }
    
    //Break loop if button press was detected
    if (digitalRead(BUTTON_GPIO) && !buttonPressed){
      return;
    }
    
    for(i=0; i< NUM_LEDS; i++) {
      leds[i] = wheel(((i * 256 / NUM_LEDS + j) & 255));
    }
    FastLED.show();
    delay(wait);
  }
}

//Input a value 0 to 255 to get a color value.
//The colours are a transition r - g - b - back to r.
CRGB wheel(byte wheelPos) {
  wheelPos = 255 - wheelPos;
  if(wheelPos < 85) {
    return CRGB(255 - wheelPos * 3, 0, wheelPos * 3);
  }
  if(wheelPos < 170) {
    wheelPos -= 85;
    return CRGB(0, wheelPos * 3, 255 - wheelPos * 3);
  }
  wheelPos -= 170;
  return CRGB(wheelPos * 3, 255 - wheelPos * 3, 0);
}


/**Funtion to check if the lamp should either enter a HIGH mode,
or revert to NORMAL if already in HIGH. If the sensors report values
that are higher than 1.1 times the average values, and this has happened
more than 30 times the last few milliseconds, it will enter HIGH mode. 
TODO: Not very well written, remove hardcoded values, and make it more
reusable and configurable.  */
void check_high(int avg) {
  if (avg > (song_avg/iter * 1.1))  {
    if (high.times != 0) {
      if (millis() - high.times_start > 200.0) {
        high.times = 0;
        songmode = NORMAL;
      } else {
        high.times_start = millis();  
        high.times++; 
      }
    } else {
      high.times++;
      high.times_start = millis();

    }
  }
  if (high.times > 30 && millis() - high.times_start < 50.0)
    songmode = HIGH;
  else if (millis() - high.times_start > 200) {
    high.times = 0;
    songmode = NORMAL;
  }
}

//Main function for visualizing the sounds in the lamp
void visualize_music(boolean twoDirections) {
  int sensor_value, mapped, avg, longavg;
  
  //Actual sensor value
  sensor_value = analogRead(ANALOG_READ);
  
  //If 0, discard immediately. Probably not right and save CPU.
  if (sensor_value == 0)
    return;

  //Discard readings that deviates too much from the past avg.
  mapped = (float)fscale(MIC_LOW, MIC_HIGH, MIC_LOW, (float)MIC_HIGH, (float)sensor_value, 2.0);
  avg = compute_average(avgs, AVGLEN);

  if (((avg - mapped) > avg*DEV_THRESH)) //|| ((avg - mapped) < -avg*DEV_THRESH))
    return;
  
  //Insert new avg. values
  insert(mapped, avgs, AVGLEN); 
  insert(avg, long_avg, LONG_SECTOR); 

  //Compute the "song average" sensor value
  song_avg += avg;
  iter++;
  if (iter > CYCLES) {  
    song_avg = song_avg / iter;
    iter = 1;
  }
    
  longavg = compute_average(long_avg, LONG_SECTOR);

  //Check if we enter HIGH mode 
  check_high(longavg);  

  if (songmode == HIGH) {
    fade_scale = 3;
    Color.r =  8;
    Color.g = 1;
    Color.b = -2;
  }
  else if (songmode == NORMAL) {
    fade_scale = 3;
    Color.r = -1;
    Color.b = 6;
    Color.g = -2;
  }
  if (twoDirections == false) {
    //Decides how many of the LEDs will be lit
    curshow = fscale(MIC_LOW, MIC_HIGH, 0.0, (float)NUM_LEDS, (float)avg, -1);
  
    /*Set the different leds. Control for too high and too low values.
            Fun thing to try: Dont account for overflow in one direction, 
      some interesting light effects appear! */
    for (int i = 0; i < NUM_LEDS; i++) 
      //The leds we want to show
      if (i < curshow) {
        if (leds[i].r + Color.r > 255)
          leds[i].r = 255;
        else if (leds[i].r + Color.r < 0)
          leds[i].r = 0;
        else
          leds[i].r = leds[i].r + Color.r;
            
        if (leds[i].g + Color.g > 255)
          leds[i].g = 255;
        else if (leds[i].g + Color.g < 0)
          leds[i].g = 0;
        else 
          leds[i].g = leds[i].g + Color.g;
  
        if (leds[i].b + Color.b > 255)
          leds[i].b = 255;
        else if (leds[i].b + Color.b < 0)
          leds[i].b = 0;
        else 
          leds[i].b = leds[i].b + Color.b;  
        
      //All the other LEDs begin their fading journey to eventual total darkness
      } else {
        leds[i] = CRGB(leds[i].r/fade_scale, leds[i].g/fade_scale, leds[i].b/fade_scale);
      }
    FastLED.show(); 
  } else {
    int middle = NUM_LEDS / 2;
    //Decides how many of the LEDs will be lit
    curshow = fscale(MIC_LOW, MIC_HIGH, 0.0, (float)middle, (float)avg, -1);
  
    /*Set the different leds. Control for too high and too low values.
            Fun thing to try: Dont account for overflow in one direction, 
      some interesting light effects appear! */
      
    for (int i = 0; i < middle ; i++){
      //The leds we want to show
      if (i < curshow) {
        if (leds[middle+i].r + Color.r > 255)
          leds[middle+i].r = 255;
        else if (leds[middle +i].r + Color.r < 0)
          leds[middle+i].r = 0;
        else
          leds[middle+i].r = leds[middle+i].r + Color.r;
            
        if (leds[middle+i].g + Color.g > 255)
          leds[middle+i].g = 255;
        else if (leds[middle+i].g + Color.g < 0)
          leds[middle+i].g = 0;
        else 
          leds[middle+i].g = leds[middle+i].g + Color.g;
  
        if (leds[middle+i].b + Color.b > 255)
          leds[middle+i].b = 255;
        else if (leds[middle+i].b + Color.b < 0)
          leds[middle+i].b = 0;
        else 
          leds[middle+i].b = leds[middle+i].b + Color.b;  
        
      //All the other LEDs begin their fading journey to eventual total darkness
      } else {
        leds[middle+i] = CRGB(leds[middle+i].r/fade_scale, leds[middle+i].g/fade_scale, leds[middle+i].b/fade_scale);
      }
      leds[middle - i] = leds[middle + i];
    }
    FastLED.show();
  }
}
//Compute average of a int array, given the starting pointer and the length
int compute_average(int *avgs, int len) {
  int sum = 0;
  for (int i = 0; i < len; i++)
    sum += avgs[i];

  return (int)(sum / len);

}

//Insert a value into an array, and shift it down removing
//the first value if array already full 
void insert(int val, int *avgs, int len) {
  for (int i = 0; i < len; i++) {
    if (avgs[i] == -1) {
      avgs[i] = val;
      return;
    }  
  }

  for (int i = 1; i < len; i++) {
    avgs[i - 1] = avgs[i];
  }
  avgs[len - 1] = val;
}

//Function imported from the arduino website.
//Basically map, but with a curve on the scale (can be non-uniform).
float fscale( float originalMin, float originalMax, float newBegin, float
    newEnd, float inputValue, float curve){

  float OriginalRange = 0;
  float NewRange = 0;
  float zeroRefCurVal = 0;
  float normalizedCurVal = 0;
  float rangedValue = 0;
  boolean invFlag = 0;


  // condition curve parameter
  // limit range

  if (curve > 10) curve = 10;
  if (curve < -10) curve = -10;

  curve = (curve * -.1) ; // - invert and scale - this seems more intuitive - postive numbers give more weight to high end on output 
  curve = pow(10, curve); // convert linear scale into lograthimic exponent for other pow function

  // Check for out of range inputValues
  if (inputValue < originalMin) {
    inputValue = originalMin;
  }
  if (inputValue > originalMax) {
    inputValue = originalMax;
  }

  // Zero Refference the values
  OriginalRange = originalMax - originalMin;

  if (newEnd > newBegin){ 
    NewRange = newEnd - newBegin;
  }
  else
  {
    NewRange = newBegin - newEnd; 
    invFlag = 1;
  }

  zeroRefCurVal = inputValue - originalMin;
  normalizedCurVal  =  zeroRefCurVal / OriginalRange;   // normalize to 0 - 1 float

  // Check for originalMin > originalMax  - the math for all other cases i.e. negative numbers seems to work out fine 
  if (originalMin > originalMax ) {
    return 0;
  }

  if (invFlag == 0){
    rangedValue =  (pow(normalizedCurVal, curve) * NewRange) + newBegin;

  }
  else     // invert the ranges
  {   
    rangedValue =  newBegin - (pow(normalizedCurVal, curve) * NewRange); 
  }

  return rangedValue;
}
