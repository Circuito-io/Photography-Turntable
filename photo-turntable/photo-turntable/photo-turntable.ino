// Include Libraries
#include "Arduino.h"
#include "LiquidCrystal_PCF8574.h"
#include "IRremote.h"
#include "Encoder.h"
#include "Button.h"
#include "math.h"

// Pin Definitions
#define RotaryEncoder_PIN_D	4
#define RotaryEncoder_PIN_CLK	2
#define RotaryEncoder_PIN_S1	5
#define STEPPER_PIN_STEP	6
#define STEPPER_PIN_DIR	7

#define SLOW 5000
#define FAST 500

// set default parameters
int speed = 100;
int angles = 4;
const int fullRotation = 18300;

// Global variables and defines
// There are several different versions of the LCD I2C adapter, each might have a different address.
// Try the given addresses by Un/commenting the following rows until LCD works follow the serial monitor prints.
// To find your LCD address go to: http://playground.arduino.cc/Main/I2cScanner and run example.
#define LCD_ADDRESS 0x3F
//#define LCD_ADDRESS 0x27

// Define LCD characteristics
#define LCD_ROWS 2
#define LCD_COLUMNS 16
#define SCROLL_DELAY 150
#define BACKLIGHT 255
long rotaryEncDOldPosition  = 0;
// object initialization
Encoder rotaryEncD(RotaryEncoder_PIN_D, RotaryEncoder_PIN_CLK);
Button rotaryEncDButton(RotaryEncoder_PIN_S1);
LiquidCrystal_PCF8574 lcdI2C;
IRsend ir_led;

enum menuState {HOME, VIDEO, STILLS, VIDSTART, STILSTART, SPEED, ANGLES, VIDBACK, STILBACK, CHANGESPEED, CHANGEANGLES} state = VIDEO, oldState = 10;

// Setup the essentials for your circuit to work. It runs first every time your circuit is powered with electricity.
void setup()
{
  // Setup Serial which is useful for debugging
  // Use the Serial Monitor to view printed messages
  Serial.begin(9600);
  while (!Serial) ; // wait for serial port to connect. Needed for native USB
  Serial.println("start");

  // initialize the lcd
  lcdI2C.begin(LCD_COLUMNS, LCD_ROWS, LCD_ADDRESS, BACKLIGHT);
  rotaryEncDButton.init();
  pinMode(RotaryEncoder_PIN_S1, INPUT_PULLUP);

  lcdI2C.print("PHOTO BOOTH");
  delay(500);
  lcdI2C.print("");

  pinMode(STEPPER_PIN_STEP, OUTPUT);
}

// Main logic of your circuit. It defines the interaction between the components you selected. After setup, it runs over and over again, in an eternal loop.
void loop()
{
  //Read encoder new position
  long rotaryEncDNewPosition = rotaryEncD.read() / 2;
  bool select = rotaryEncDButton.onPress();
  char menuDir = ' ';
  if (rotaryEncDNewPosition != rotaryEncDOldPosition) {
    //Serial.println(rotaryEncDNewPosition);
    if (rotaryEncDNewPosition < rotaryEncDOldPosition)
    {
      menuDir = 'L';
    }
    else if (rotaryEncDNewPosition >= rotaryEncDOldPosition)
    {
      menuDir = 'R';
    }
    rotaryEncDOldPosition = rotaryEncDNewPosition;
  }

  if (state != oldState)
  {
    lcdI2C.clear();

    oldState = state;
  }


  lcdI2C.selectLine(1);
  lcdI2C.print(printState(state));

  switch (state)
  {
    case VIDEO:
      if (menuDir == 'L' || menuDir == 'R')
        state = STILLS;

      if (select)
        state = VIDSTART ;

      break;

    case STILLS:
      if (menuDir == 'L' || menuDir == 'R')
        state = VIDEO;

      if (select)
        state = STILSTART;

      break;

    case VIDSTART:
      if (menuDir == 'R')
        state = SPEED;

      else if (menuDir == 'L')
        state = VIDBACK;

      if (select) {
        video();

      }
      break;

    case SPEED:
      if (menuDir == 'L')
        state = VIDSTART;

      else if (menuDir == 'R')
        state = VIDBACK;

      if (select)

        state = CHANGESPEED;

      break;

    case CHANGESPEED:
      if (menuDir == 'L')
      {
        speed-=5;
        lcdI2C.selectLine(2);
        lcdI2C.print("     ");
      }
      else if (menuDir == 'R')
      {
        speed+=5;
        lcdI2C.selectLine(2);
        lcdI2C.print("     ");
      }
      speed=constrain(speed,0,100);
      lcdI2C.selectLine(2);
      lcdI2C.print(speed);
      lcdI2C.print("%");


      if (select)
        state = VIDSTART;

      break;
    case VIDBACK:
      if (menuDir == 'L')
        state = SPEED;

      else if (menuDir == 'R')
        state = VIDSTART;


      if (select)
        state = VIDEO;

      break;


    case STILSTART:
      if (menuDir == 'R')
        state = ANGLES;

      else if (menuDir == 'L')
        state = STILBACK;

      if (select)
        stills();
      break;

    case ANGLES:
      if (menuDir == 'L')
        state = STILSTART;

      else if (menuDir == 'R')
        state = STILBACK;


      if (select)
        state = CHANGEANGLES;

      break;

    case CHANGEANGLES:
      if (menuDir == 'L')
      {
        angles--;
        lcdI2C.selectLine(2);
        lcdI2C.print("     ");
      }
      else if (menuDir == 'R')
      {
        angles++;
        lcdI2C.selectLine(2);
        lcdI2C.print("     ");
      }

      lcdI2C.selectLine(2);
      lcdI2C.print(angles);

      if (select)
        state = STILSTART;
      break;

    case STILBACK:
      if (menuDir == 'L')
        state = ANGLES;

      else if (menuDir == 'R')
        state = STILSTART;

      if (select)
        state = STILLS;
      break;
  }




}

String printState(int curstate)
{
  switch (curstate) {
    case 0:
      return "HOME";
    case 1:
      return "VIDEO";
    case 2:
      return "STILLS";
    case 3:
      return "VIDEO START";
    case 4:
      return "STILLS START";
    case 5:
      return "SET SPEED";
    case 6:
      return "SET ANGLES";
    case 7:
      return "BACK";
    case 8:
      return "BACK";
    case 9:
      return "SET SPEED:";
    case 10:
      return "SET ANGLES:";
  }

}


void stills() {

  for (int i = 0 ; i < angles ; i++) {
    delay(300);
    irStillsShot();
    delay(750);
    int delay_time = SLOW;
    int numOfSteps = fullRotation / angles;
    float accSteps = 600;
    for (int i = 0; i < numOfSteps; i++ )
    {

      if (i < accSteps)
      {
        delay_time = (SLOW - FAST) * pow(float(i - accSteps), 2) / float(accSteps * accSteps) + FAST;
      }
      else if (i > numOfSteps - accSteps)
      {
        delay_time = (SLOW - FAST) * pow(float(i - numOfSteps + accSteps), 2) / float(accSteps * accSteps) + FAST;
      }
      else
      {
        delay_time = FAST;
      }

      digitalWrite(STEPPER_PIN_STEP, HIGH);   // turn the LED on (HIGH is the voltage level)
      delayMicroseconds(delay_time);                       // wait for a second
      digitalWrite(STEPPER_PIN_STEP, LOW);    // turn the LED off by making the voltage LOW
      delayMicroseconds(delay_time);                       // wait for a second

    }
  }
}


void video() {
  delay(300);
  irVideoShot();
  delay(750);
  int delay_time = SLOW;
  int fast = 300 + 12*(100-speed);
  int numOfSteps = fullRotation;
  float accSteps = 600;
  for (int i = 0; i < numOfSteps; i++ )
  {

    if (i < accSteps)
    {
      delay_time = (SLOW - fast) * pow(float(i - accSteps), 2) / float(accSteps * accSteps) + fast;
    }
    else if (i > numOfSteps - accSteps)
    {
      delay_time = (SLOW - fast) * pow(float(i - numOfSteps + accSteps), 2) / float(accSteps * accSteps) + fast;
    }
    else
    {
      delay_time = fast;
    }
    digitalWrite(STEPPER_PIN_STEP, HIGH);   // turn the LED on (HIGH is the voltage level)
    delayMicroseconds(delay_time);                       // wait for a second
    digitalWrite(STEPPER_PIN_STEP, LOW);    // turn the LED off by making the voltage LOW
    delayMicroseconds(delay_time);                       // wait for a second
  }
  delay(750);
  irVideoShot();
  delay(500);

}





void irVideoShot() {
  int khz = 33; // 33kHz frequency
  unsigned int irSignal[] = {488};
  ir_led.sendRaw(irSignal, sizeof(irSignal) / sizeof(irSignal[0]), khz);
  delayMicroseconds(5360);
  ir_led.sendRaw(irSignal, sizeof(irSignal) / sizeof(irSignal[0]), khz);
}

void irStillsShot() {
  int khz = 33; // 33kHz frequency
  unsigned int irSignal[] = {480};
  ir_led.sendRaw(irSignal, sizeof(irSignal) / sizeof(irSignal[0]), khz);
  delayMicroseconds(7330);
  ir_led.sendRaw(irSignal, sizeof(irSignal) / sizeof(irSignal[0]), khz);
}

