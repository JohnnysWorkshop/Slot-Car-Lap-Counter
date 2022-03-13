/*******PHASE 1.5*******

   This is an on going project.
   You are welcome to use it and share it.

   Updates:
   - detection pins have been changed to match RC
   - laps setup menu,
   - mechanical debounce,
   - sketch behaviour.
   (Big thanks to Arduino community)

   Requirements:
   - Arduino uno,
   (lane detection)
   - 4x momentary switches,(pins: 5, 6, 7, 8, GRND)
   OR 4x 4N35 Optocouplers,(same pins as above + 4x 220 ohms resistors)
   (menu buttons)
   - 3x momentary switches,(pins: 2, 3, 4, GRND)
   - 7x 10k ohms resistors,
   - 4x 10 to 120 microfarads or so to mechanicaly debounce detection
   - 20 X 4 lcd display,(regular I2C pins: A5, A4, VCC, GND)
   or 18 x 2 lcd if you only wish to display 2 lanes (will require changes within code)
   - breadboard,
   - and jump wires.
*/
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
LiquidCrystal_I2C lcd(0x3F, 20, 4);//will require changes if you use different size LCD
//This array arrangement has changed a little bit
//to allow us to change the number of lanes in use.
const byte laneCount = 4;
const char *placeNames[laneCount] = {"P1", "P2", "P3", "P4"};
const byte laneSensorPins[laneCount] = {5, 6, 7, 8};
//BUTTONS
const byte buttonCount = 3;
const byte buttonPins[buttonCount] = {2, 3, 4};
boolean buttonState[buttonCount];
boolean lastButtonState[buttonCount];
unsigned long timeStart;
bool holdButton;
int lapsLimit;
boolean setupLaps = true;
boolean raceStarted = false;
boolean raceFinished = false;
//GANTRY
const byte ledCount = 5;
const byte ledPins[ledCount] = {9, 10, 11, 12, 13};
//RELAY
const byte relayCount = 4;
const byte relayPins[relayCount] = {A0, A1, A2, A3};

boolean lanePreviousState[laneCount];
int lapCounter[laneCount];
unsigned long timeOfLastLap[laneCount];
int lanePosition[laneCount];

unsigned long carTimeLap[laneCount];
unsigned long carStart[laneCount];
unsigned long carBest[laneCount];
float lastLap[laneCount];
float bestLap[laneCount];
boolean firstLap[laneCount];
boolean newBest[laneCount];

void setup()
{
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);                //this displays a welcome message
  lcd.print("********************");  //
  lcd.setCursor(1, 1);                //
  lcd.print("Johnny's  Workshop");    //
  lcd.setCursor(0, 2);                //
  lcd.print("Slot Car Lap Counter");  //
  lcd.setCursor(0, 3);                //
  lcd.print("********************");  //************************************
  delay(2000);  //This delay() and clear() are only useful
  lcd.clear();  //if you wish to display a welcome message.

  for (int lane = 0; lane < laneCount; lane++)
  {
    pinMode(laneSensorPins[lane], INPUT);

    lapCounter[lane] = -1;
    carStart[lane] = millis();
    carBest[lane] = 9999;
    lastLap[lane] = 00.00;
    bestLap[lane] = 00.00;
    firstLap[lane] = true;
    newBest[lane] = false;
  }
  for (int button = 0; button < buttonCount; button++)
  {
    pinMode(buttonPins[button], INPUT_PULLUP);
  }
  holdButton = false;
  for (int led = 0; led < ledCount; led++)
  {
    pinMode(ledPins[led], OUTPUT);
  }
  for (int relay = 0; relay < relayCount; relay++)
  {
    pinMode(relayPins[relay], OUTPUT);
  }
}
void loop()
{
  if (setupLaps)
  {
    raceFinished = false;
    lapsLimit = constrain(lapsLimit, 1, 1000);//defines a minimum and maximum laps (x, min, max)
    lcd.setCursor(5, 0);
    lcd.print("RACE SETUP");
    lcd.setCursor(0, 2);
    lcd.print("number of laps:");
    lcd.setCursor(16, 2);
    lcd.print(lapsLimit);
    if (lapsLimit < 1000)
    {
      lcd.setCursor(19, 2);
      lcd.print(" ");
    }
    if (lapsLimit < 100)
    {
      lcd.setCursor(18, 2);
      lcd.print(" ");
    }
    if (lapsLimit < 10)
    {
      lcd.setCursor(17, 2);
      lcd.print(" ");
    }
    for (int button = 0; button < buttonCount; button++)
    {
      boolean buttonState = digitalRead(buttonPins[button]) == LOW;//HIGH
      if (buttonState != lastButtonState[button])
      {
        lastButtonState[button] = buttonState;
        if ((buttonState))
        {
          if (digitalRead(2) == HIGH)lapsLimit--;
          if (digitalRead(3) == HIGH)lapsLimit++;
          if (digitalRead(4) == LOW)startGantry();
          timeStart = millis();
          holdButton = true;
        }
        else holdButton = false;
      }
      if (holdButton)
      {
        if ((millis() - timeStart) >= 1000)
        {
          if (digitalRead(2) == HIGH)lapsLimit--;
          if (digitalRead(3) == HIGH)lapsLimit++;
        }
      }
    }
  }
  if (raceFinished)
  {
    for (int button = 0; button < buttonCount; button++)
    {
      boolean buttonState = digitalRead(buttonPins[button]) == LOW;//high
      if (buttonState != lastButtonState[button])
      {
        lastButtonState[button] = buttonState;
        if ((buttonState))
        {
          lcd.clear();
          resetData();
          setupLaps = true;
        }
      }
    }
  }
  //************************************************
  if (raceStarted)
  {
    //*******************************************
    //Enter relay code here to power lanes!
    for (int relay = 0; relay < relayCount; relay++)
    {
      digitalWrite(relayPins[relay], HIGH);
    }
    //**********************************************
    for (int lane = 0; lane < laneCount; lane++)
    {
      if (lapCounter[lane] >= lapsLimit)
      {
        raceFinished = true;
        if (raceFinished) raceStarted = false;
        //***********************************
        //Enter relay codes to stop each lane!
        for (int relay = 0; relay < relayCount; relay++)
        {
          digitalWrite(relayPins[relay], LOW);
        }
        //**************************************
      }
    }
    boolean newData = false;
    for (int lane = 0; lane < laneCount; lane++)
    {
      boolean laneState = digitalRead(laneSensorPins[lane]) == HIGH;//LOW;
      if (laneState != lanePreviousState[lane])
      {
        lanePreviousState[lane] = laneState;
        if (laneState)
        {
          lapCounter[lane]++;
          timeOfLastLap[lane] = millis();
          newData = true;
          lcd.setCursor(0, lane);
          lcd.print(lapCounter[lane]);
          carTimeLap[lane] = millis() - carStart[lane];
          carStart[lane] = millis();
          lastLap[lane] = carTimeLap[lane] / 1000.00;
          if (firstLap[lane] == true)
          {
            lcd.setCursor(10, lane);
            lcd.print("-");
            lcd.setCursor(5, lane);
            lcd.print("HOT");
            lcd.setCursor(12, lane);
            lcd.print("LAP");
          }
          if (firstLap[lane] != true) {
            lcd.setCursor(4, lane);
            lcd.print("      ");
            lcd.setCursor(4, lane);
            lcd.print(lastLap[lane]);
          }
          if (carTimeLap[lane] < carBest[lane] && firstLap[lane] != true) {
            carBest[lane] = carTimeLap[lane];
            bestLap[lane] = lastLap[lane];
            lcd.setCursor(12, lane);
            lcd.print("      ");
            lcd.setCursor(12, lane);
            lcd.print(bestLap[lane]);
            newBest[lane] = true;
          }
          firstLap[lane] = false;
          if (newBest[lane] == true)newBest[lane] = false;
        }
      }
    }
    if (!newData)
      return;
    for (int lane = 0; lane < laneCount; lane++)
    {
      int aheadOfMe = 0;
      for (int otherLane = 0; otherLane < laneCount; otherLane++)
      {
        if (lane == otherLane)
          continue;
        if (lapCounter[lane] < lapCounter[otherLane])
        {
          aheadOfMe++;
          continue;
        }
        else if (lapCounter[lane] == lapCounter[otherLane] &&
                 timeOfLastLap[lane] > timeOfLastLap[otherLane])
        {
          aheadOfMe++;
          continue;
        }
      }
      lanePosition[lane] = aheadOfMe;
      lcd.setCursor(18, lane);
      lcd.print(placeNames[lanePosition[lane]]);
    }
  }
}
void startGantry()
{
  long randomGo;
  randomGo = random(1000, 5000);//random Green Flag (from, to) in milliseconds

  setupLaps = false;//disable setup laps menu
  lcd.clear();
  raceStarted = true;
  //GANTRY
  delay(1000);
  digitalWrite(9, HIGH);
  delay(1000);
  digitalWrite(10, HIGH);
  delay(1000);
  digitalWrite(11, HIGH);
  delay(1000);
  digitalWrite(12, HIGH);
  delay(1000);
  digitalWrite(13, HIGH);
  delay(randomGo);// take the random value generated
  for (int led = 0; led < ledCount; led++)
  {
    digitalWrite(ledPins[led], LOW);
  }
  //GANTRY END
}
void resetData()
{
  for (int lane = 0; lane < laneCount; lane++)
  {
    lanePreviousState[lane] = 0;
    lapCounter[lane] = -1;
    timeOfLastLap[lane] = 0;
    lanePosition[lane] = 0;
    carBest[lane] = 9999;
    lastLap[lane] = 00.00;
    bestLap[lane] = 00.00;
    firstLap[lane] = true;
    newBest[lane] = false;
  }
}
