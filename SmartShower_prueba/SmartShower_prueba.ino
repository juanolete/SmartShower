/*
 Display all the fonts.

 This sketch uses the GLCD (font 1) and fonts 2, 4, 6, 7, 8

 Sugested TFT connections for UNO and Atmega328 based boards
   sclk 13  // Don't change, this is the hardware SPI SCLK line
   mosi 11  // Don't change, this is the hardware SPI MOSI line
   cs   10  // Chip select for TFT display
   dc   9   // Data/command line
   rst  7   // Reset, you could connect this to the Arduino reset pin

  #########################################################################
  ###### DON'T FORGET TO UPDATE THE User_Setup.h FILE IN THE LIBRARY ######
  ######       TO SELECT THE FONTS AND PINS YOU USE, SEE ABOVE       ######
  #########################################################################
 */

// New background colour
#define TFT_BROWN 0x38E0

//Includes
#include <Wire.h>
#include <Adafruit_MLX90614.h>
#include <TFT_ILI9341.h> 
#include <SPI.h>
#include <MsTimer2.h>


//DEFINITIONS
#define FLUJO_PIN             2
#define TEMP_PIN              3
#define drawTime              60000 //tiempo [ms] entre duchas 
#define calibrationFactor     7

//hardware
TFT_ILI9341 tft = TFT_ILI9341();
Adafruit_MLX90614 mlx = Adafruit_MLX90614();

//Software
int                     totalAgua = 0;
bool                    flag_erase = false;
volatile byte           pulseCount;  
float                   flowRate;
unsigned long           flowMilliLitres;
unsigned long           totalMilliLitres;
unsigned long           oldTime;
unsigned long           currentTime;

//funcion para manejar interrupcion de sensor
void pulseCounter(){
  pulseCount++;
}

//funcion timer que guarda el total de agua por un tiempo t y lo borra despues de ese tiempo
void saveTotal(){
  if( totalMilliLitres - totalAgua > 100 ){
    totalAgua = totalMilliLitres;
  } else {
    totalAgua = 0;
    totalMilliLitres = 0;
  }
}

char tMl[6+3];
char fMl[5+3];
char T[3+3];
void escribirAgua(){
    String totalMl = String(totalMilliLitres)+ " ml";
    String flujoMl = String(flowMilliLitres)+ " ml";
    String temp = String(mlx.readObjectTempC())+ " *C";
    totalMl.toCharArray(tMl, 6+3);
    flujoMl.toCharArray(fMl, 5+3);
    temp.toCharArray(T, 3+3);
   
    tft.drawCentreString(tMl, 120, 70, 4);
    tft.drawCentreString(fMl, 120, 160, 4);
    tft.drawCentreString(T, 120, 250, 4);
}

void borrarAgua(){
    tft.drawCentreString("                          ", 120, 70, 4);  
    tft.drawCentreString("                          ", 120, 160, 4); 
    tft.drawCentreString("                          ", 120, 250, 4);  
}

void flash(){
  tft.drawCentreString("                          ", 120, 70, 4);
  tft.drawCentreString("0", 120, 70, 4);
}

void setup(void) {

  Serial.begin(9600);
  mlx.begin();
  tft.init();
  
  MsTimer2::set(drawTime, saveTotal); // 10000ms period
  MsTimer2::start();
  
  tft.setRotation(2);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.fillScreen(TFT_WHITE);
  tft.setTextSize(1);

  tft.drawCentreString("AGUA", 120, 10, 4);
  tft.drawCentreString("CONSUMIDA", 120, 40, 4);

  tft.drawCentreString("FLUJO DE AGUA", 120, 130, 4);
  tft.drawCentreString("TEMPERATURA", 120, 220, 4);

  pinMode(FLUJO_PIN, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  
  pulseCount        = 0;
  flowRate          = 0.0;
  flowMilliLitres   = 0;
  totalMilliLitres  = 0;
  oldTime           = millis();
  
  //timer1.attach(60 /*segundos */,saveTotal);
  attachInterrupt(digitalPinToInterrupt(FLUJO_PIN), pulseCounter, RISING);
}

void printFlujo(){
  Serial.print("Flow rate: ");
  Serial.print(int(flowRate));  // Print the integer part of the variable
  Serial.print(".");             // Print the decimal point
  // Determine the fractional part. The 10 multiplier gives us 1 decimal place.
  unsigned int frac;
  frac = (flowRate - int(flowRate)) * 10;
  Serial.print(frac, DEC) ;      // Print the fractional part of the variable
  Serial.print("L/min");
  // Print the number of litres flowed in this second
  Serial.print("  Current Liquid Flowing: ");             // Output separator
  Serial.print(flowMilliLitres);
  Serial.print("mL/Sec");

  // Print the cumulative total of litres flowed since starting
  Serial.print("  Output Liquid Quantity: ");             // Output separator
  Serial.print(totalMilliLitres);
  Serial.println("mL"); 
}

void loop()
{
  currentTime = millis();
  if((currentTime - oldTime) > 500)    // Only process counters once per second
  { 
    detachInterrupt(FLUJO_PIN);
    flowRate = ((1000.0 / (currentTime - oldTime)) * pulseCount) / calibrationFactor;
    flowMilliLitres = (flowRate / 60) * 1000;
    totalMilliLitres += flowMilliLitres;
      
    printFlujo();
    // Reset the pulse counter so we can start incrementing again
    pulseCount = 0;
    borrarAgua(); 
    escribirAgua();

    attachInterrupt(digitalPinToInterrupt(FLUJO_PIN), pulseCounter, RISING);
    oldTime = millis();
  }
}