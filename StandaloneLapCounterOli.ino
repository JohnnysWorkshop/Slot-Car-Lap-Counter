/*******Oli's 3 Lanes Lap-Counter*******/

#include <LiquidCrystal_I2C.h>
#include <Wire.h>
LiquidCrystal_I2C lcd(0x3F, 20, 4);//will require changes if you use different size LCD

const byte laneCount = 3;                                 //Fixed amount of lanes
const char *placeNames[laneCount] = {"P1", "P2", "P3"};   //Live cars position
const byte laneSensorPins[laneCount] = {5, 6, 7};         //Pins used for each lane switch

const byte buttonCount = 3;                               //Fixed amount of buttons
const byte buttonPins[buttonCount] = {2, 3, 4};           //Pins used for each button
boolean buttonState[buttonCount];                         //Hold button function
boolean lastButtonState[buttonCount];                     //
unsigned long timeStart;                                  //
bool holdButton;                                          //

int lapsLimit = 10;                                       //Default number of lap when powered up
boolean setupLaps = true;                                 //Different race stages function
boolean raceStarted = false;                              //
boolean raceFinished = false;                             //

const byte ledCount = 5;                                  //Fixed amount of LEDs
const byte ledPins[ledCount] = {9, 10, 11, 12, 13};       //Pins used for each pair of LEDs

boolean lanePreviousState[laneCount];                     //Lap counting functions
int lapCounter[laneCount];                                //
unsigned long timeOfLastLap[laneCount];                   //
int lanePosition[laneCount];                              //
unsigned long carTimeLap[laneCount];                      //
unsigned long carStart[laneCount];                        //
unsigned long carBest[laneCount];                         //
float lastLap[laneCount];                                 //
float bestLap[laneCount];                                 //
boolean firstLap[laneCount];                              //
boolean newBest[laneCount];                               //

//ANOTHER ONE BITES THE DUST notes
#define NOTE_E2  82
#define NOTE_G2  98
#define NOTE_A2  110
#define NOTE_B2  123
#define NOTE_C3  131
#define NOTE_CS3 139
#define NOTE_D3  147
#define NOTE_G3  196
#define REST 0

int tempo = 110;                                          //Song tempo
int buzzer = 8;                                           //Speaker pin
int melody[] = {                                          //ANOTHER ONE BITES THE DUST chart
  NOTE_A2, 16, NOTE_G2, 16, NOTE_E2, 8, REST, 8, NOTE_E2, 8, REST, 8, NOTE_E2, 8, REST, 4,
  NOTE_E2, 32, REST, 32, NOTE_E2, 12, REST, 32, NOTE_E2, 8, NOTE_G2, 8, NOTE_E2, 16, NOTE_A2, 16, REST, 3,
  NOTE_A2, 16, NOTE_G2, 16, NOTE_E2, 8, REST, 8, NOTE_E2, 8, REST, 8, NOTE_E2, 8, REST, 4,
  NOTE_E2, 32, REST, 32, NOTE_E2, 12, REST, 32, NOTE_E2, 8, NOTE_G2, 8, NOTE_E2, 16, NOTE_A2, 16, REST, 3,

  NOTE_C3, 16, NOTE_C3, 8, NOTE_C3, 4, NOTE_CS3, 16, NOTE_D3, 16, NOTE_G3, 16, NOTE_G2, 8, REST, 4,
  NOTE_C3, 16, NOTE_C3, 8, NOTE_C3, 4, NOTE_CS3, 16, NOTE_D3, 8, REST, 8, NOTE_G2, 8, REST, 8,
  NOTE_C3, 16, NOTE_C3, 8, NOTE_C3, 4, NOTE_CS3, 16, NOTE_D3, 16, NOTE_G3, 16, NOTE_G2, 8, REST, 4,
  NOTE_A2, 16, NOTE_A2, 8, NOTE_A2, 4, NOTE_A2, 16, NOTE_B2, 16, REST, 3,
};
int notes = sizeof(melody) / sizeof(melody[0]) / 2;       //Song playing functions
int wholenote = (60000 * 4) / tempo;                      //
int divider = 0, noteDuration = 0;                        //

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
    pinMode(laneSensorPins[lane], INPUT_PULLUP);

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
  pinMode(buzzer, OUTPUT);
}
void loop()
{
  if (setupLaps)
  {
    raceFinished = false;
    lapsLimit = constrain(lapsLimit, 1, 1000);        //Threshold of laps (x, min, max)
    lcd.setCursor(5, 0);                              //Menu display
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
      boolean buttonState = digitalRead(buttonPins[button]) == HIGH;
      if (buttonState != lastButtonState[button])
      {
        lastButtonState[button] = buttonState;
        if ((buttonState))
        {
          tone(buzzer, 300, 100);
          if (digitalRead(4) == HIGH)lapsLimit++;
          if (digitalRead(3) == HIGH)lapsLimit--;
          if (digitalRead(2) == HIGH)startGantry();
          timeStart = millis();
          holdButton = true;
        }
        else holdButton = false;
      }
      if (holdButton)
      {
        if ((millis() - timeStart) >= 1000)
        {
          if (digitalRead(4) == HIGH)lapsLimit++;
          if (digitalRead(3) == HIGH)lapsLimit--;
        }
      }
    }
  }
  if (raceFinished)
  {
    //ALL LEDs ON
    for (int led = 0; led < ledCount; led++)
    {
      digitalWrite(ledPins[led], HIGH);
    }
    //MUSIC PLAYING at the end of race
    delay(100);
    for (int thisNote = 0; thisNote < notes * 2; thisNote = thisNote + 2) {
      divider = melody[thisNote + 1];
      if (divider > 0) {
        noteDuration = (wholenote) / divider;
      } else if (divider < 0) {
        noteDuration = (wholenote) / abs(divider);
        noteDuration *= 1.5;
      }
      tone(buzzer, melody[thisNote], noteDuration * 0.9);
      delay(noteDuration);
      noTone(buzzer);
      //QUIT BACK TO LAP SETUP
      if (digitalRead(2) == HIGH || digitalRead(3) == HIGH || digitalRead(4) == HIGH)
      {
        for (int led = 0; led < ledCount; led++)
        {
          digitalWrite(ledPins[led], LOW);
        }
        lcd.clear();
        resetData();
        setupLaps = true;
        break;
      }
    }
  }
  //************************************************
  if (raceStarted)
  {
    //*******************************************
    //Enter relay code here to power lanes!
    //**********************************************
    for (int lane = 0; lane < laneCount; lane++)
    {
      if (lapCounter[lane] >= lapsLimit)
      {
        raceFinished = true;
        if (raceFinished) raceStarted = false;
        //***********************************
        //Enter relay codes to stop each lane!
        //**************************************
      }
    }
    boolean newData = false;
    for (int lane = 0; lane < laneCount; lane++)
    {
      boolean laneState = digitalRead(laneSensorPins[lane]) == LOW;//LOW;
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
          if (newBest[lane] == true) {
            newBest[lane] = false;
            tone(buzzer, 900, 100);
          }
          else {
            tone(buzzer, 300, 100);
          }
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
  randomGo = random(1000, 5000);                    //random Green Flag (from, to) in milliseconds

  setupLaps = false;                                //disable setup laps menu
  lcd.clear();
  raceStarted = true;
  
  //GANTRY
  delay(1000);
  digitalWrite(9, HIGH);
  tone(buzzer, 500, 200);
  delay(1000);
  digitalWrite(10, HIGH);
  tone(buzzer, 600, 200);
  delay(1000);
  digitalWrite(11, HIGH);
  tone(buzzer, 700, 200);
  delay(1000);
  digitalWrite(12, HIGH);
  tone(buzzer, 800, 200);
  delay(1000);
  digitalWrite(13, HIGH);
  tone(buzzer, 900, 200);
  delay(randomGo);// take the random value generated
  for (int led = 0; led < ledCount; led++)
  {
    digitalWrite(ledPins[led], LOW);
  }
  tone(buzzer, 1000, 200);
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
