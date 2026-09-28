#include <TFT_eSPI.h>
#include <SPI.h>
#include <Wire.h>
#include "AHT20.h"
#include "LIS3DHTR.h"

LIS3DHTR<TwoWire> lis;
AHT20 AHT;

TFT_eSPI tft = TFT_eSPI();
void setup() {
  //LCD
  tft = TFT_eSPI();
  tft.init();
  tft.setRotation(3);
  tft.setCursor(0, 0, 2);
  tft.fillScreen(TFT_GREEN);
  tft.setTextColor(TFT_BLACK, TFT_GREEN);
  tft.setTextSize(5);

  // put your setup code here, to run once:
  Serial.begin(19200);
  // buttons
  pinMode(WIO_KEY_A, INPUT_PULLUP);
  pinMode(WIO_KEY_B, INPUT_PULLUP);
  pinMode(WIO_KEY_C, INPUT_PULLUP);
  //accelerometer
  AHT.begin();
  lis.begin(Wire1); 
  lis.setOutputDataRate(LIS3DHTR_DATARATE_25HZ); // Setting output data rage to 25Hz, can be set up tp 5kHz 
  lis.setFullScaleRange(LIS3DHTR_RANGE_2G); // Setting scale range to 2g, select from 2,4,8,16g
  //light sensor
  pinMode(WIO_LIGHT, INPUT);
  //buzzer
  pinMode(WIO_BUZZER, OUTPUT);
  //interrupt
  attachInterrupt(digitalPinToInterrupt(WIO_KEY_A), apress, FALLING);
  attachInterrupt(digitalPinToInterrupt(WIO_KEY_B), bpress, FALLING);
  attachInterrupt(digitalPinToInterrupt(WIO_KEY_C), cpress, FALLING);
}

//variables declared here largely for memory reasons
int ret; //for reading humidity and temperature
float tempF;
float temp; // for temp
float humi; // for humidity
float x_values, y_values, z_values; // for accelerometer
int hours; //hour variable
int mins; //minute variable
int secs; // second variable
volatile bool bhasbeenpressed = false; //flag showing whether or not b has been pressed
volatile bool ahasbeenpressed = false; //flag showing whether or not a has been pressed
volatile bool chasbeenpressed = false; //flag showing whether or not c has been pressed
bool alarmMode = false;
bool alarmOn = false;
unsigned long startmillis; // for timing purposes
unsigned long currmillis; // for timing purposes
String displayTime;
String displayTemp;
String displayHumd;

//alarm variables
int aHours = -1;
int aMins = -1;

//watchdog timer variables to turn off screen
int screenOffTimer = 0;
bool screenOn = true;

//display variables
int xPos;
int yPos;

//buzzbuzz
unsigned long buzzermillisstart; // for timing purposes
unsigned long buzzermillisend; // for timing purposes

//tilting
float refX = 0, refY = 0, refZ = 0;
float refMag;
float currMag;
float dotProduct;
float angle;


void bpress() //flag b as true
{
  bhasbeenpressed = true;
  return;
}

void apress() //flag a as true
{
  ahasbeenpressed = true;
  return;
}

void cpress() //flag c as true
{
  chasbeenpressed = true;
  return;
}

void doA() // up minute
{
  if(alarmMode) //in alarm mode
  {
    aMins ++;
    if(aMins > 59) //check if mins > 59
    {
      aMins = 0;
    }
  }
  else //not in alarm mode
  {
    mins ++;
    if(mins > 59) //check if mins > 59
    {
      mins = 0;
    }
  }
  ahasbeenpressed = false;
  buzzbuzz();
  screenOffTimer = 0;
  return;
}

void doB() //up hour
{
  if (alarmMode) //in alarm mode
  {
    aHours ++;
    if (aHours > 23)  //check if hours > 23
    {
      aHours = 0;
    }
  }
  else //not in alarm mode
  {
    hours ++;
    if (hours > 23)  //check if hours > 23
    {
      hours = 0;
    }
  }
  bhasbeenpressed = false;
  buzzbuzz();
  screenOffTimer = 0;
  return;
}

void doC() //alarm mode
{
  if(alarmMode)
  {
    alarmMode = false;
  }
  else alarmMode = true;
  chasbeenpressed = false;
  if(alarmOn)
  {
    alarmOn = false;
    analogWrite(WIO_BUZZER, 0);
  }
  screenOffTimer = 0;
  return;
}

void updateClock() //updates clock and temp/humidity
{
  // get updated time
  if (secs == 59)
  {
    if (mins == 59)
    {
      hours ++;
      if (hours > 23)
      {
        hours = 0;
      }
      mins = 0;
    }
    else mins ++;
    secs = 0;
  }
  else secs ++;

  // update humi and temp
  ret = AHT.getSensor(&humi, &temp);
  tempF = 1.8 * temp + 32;

  // Clear screen
  tft.fillScreen(TFT_GREEN);
  displayTime = "";
  //when in alarm mode, display just hours and minutes that are set to alarm
  if(alarmMode)
  {

    if (aHours < 10)
    {
      displayTime = "0";
    }
    displayTime = displayTime + String(aHours);
    if(aMins < 10)
    {
      displayTime = displayTime + ":0" + String(aMins);
    }
    else
    {
      displayTime = displayTime + String(aHours) + ":" + String(aMins);
    }

    // Calculate centered coordinates
    xPos = (tft.width() - (displayTime.length()*32)) / 2;
    yPos = (tft.height() - 48) / 2;

    // Dsiplay alarm time
    tft.drawString(displayTime, xPos, 0);
    Serial.println(displayTime);
  }
  else
  {
    //when not in alarm mode, display, current hours : mins : secs, humidity and temp
     if (hours < 10)
    {
      displayTime = "0";
    }
    displayTime = displayTime + String(hours);
    if(mins < 10)
    {
      displayTime = displayTime + ":0" + String(mins) + ":";
    }
    else
    {
      displayTime = displayTime + String(hours) + ":" + String(mins) + ":";
    }
    if(secs < 10)
    {
      displayTime = displayTime + "0";
    }
    displayTime = displayTime + String(secs);
    

    // Calculate centered coordinates
    xPos = (tft.width() - (displayTime.length()*32)) / 2;
    yPos = (tft.height() - 48) / 2;

    // Display
    tft.drawString(displayTime, 0, 0);
    Serial.print(displayTime);

    //display temp
    yPos = tft.height() - 72;
    displayTemp = String((int)temp) + "C/" + String((int)tempF) + "F";
    tft.drawString(displayTemp, 0, 100);
    Serial.print(displayTemp);
    //display humi
    displayHumd = String((int)humi) + "%";
    xPos = tft.width() - (displayHumd.length()*32);
    tft.drawString(displayHumd, xPos - 25, yPos);
    Serial.println(displayHumd);
  }
  return;
}

void buzzbuzz()
{
  buzzermillisstart = millis();
  analogWrite(WIO_BUZZER, 128);
  do
  {
    buzzermillisend = millis();
  }while((buzzermillisend - buzzermillisstart) < (1000));
  analogWrite(WIO_BUZZER, 0);
  
  return;
}


void loop() {
  // put your main code here, to run repeatedly:
  //update every second
  startmillis = millis();
  do{
    currmillis = millis();
    if (ahasbeenpressed) //check if a has been pressed
    {
      doA();
    }

    if (bhasbeenpressed) //check if b has been pressed
    {
      doB();
    }

    if (chasbeenpressed) //check if c has been pressed
    {
      doC();
    }

    if((hours == aHours)&&(mins == aMins))
    {
      alarmOn = true;
    }
   
  else 
  {
    screenOn = false;
  } 
  x_values = lis.getAccelerationX();
  
    if(alarmOn)
    {
      analogWrite(WIO_BUZZER, 128);
    }

  }while((currmillis - startmillis) < 999);
  updateClock(); //update clock and display 
  screenOffTimer ++;
  if(lis.getAccelerationX() > x_values - .2 && lis.getAccelerationX() < x_values + .2) 
   {
    if (screenOffTimer > 5) 
    {
      screenOn = false;
      digitalWrite(LCD_BACKLIGHT, LOW);

    }
  } 
  Serial.println(screenOffTimer);

  
  /*float refX = 0, refY = 0, refZ = 0, newX = 0, newY = 0, newZ = 0;
const float tiltThreshold = 50.0;*/
  

 
  
  if(screenOffTimer < 5)
    {
      screenOn = true;
      digitalWrite(LCD_BACKLIGHT, HIGH);
    }
  
}
