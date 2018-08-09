/*
Liquid flow rate sensor -DIYhacking.com Arvind Sanjeev

Measure the liquid/water flow rate using this code. 
Connect Vcc and Gnd of sensor to arduino, and the 
signal line to arduino digital pin 2.
 
 */

/* Typical setup for ESP8266 NodeMCU ESP-12 is :

 Display SDO/MISO  to NodeMCU pin D6 (or leave disconnected if not reading TFT)
 Display LED       to NodeMCU pin VIN (or 5V, see below)
 Display SCK       to NodeMCU pin D5
 Display SDI/MOSI  to NodeMCU pin D7
 Display DC (RS/AO)to NodeMCU pin D3
 Display RESET     to NodeMCU pin D4 (or RST, see below)
 Display CS        to NodeMCU pin D8 (or GND, see below)
 Display GND       to NodeMCU pin GND (0V)
 Display VCC       to NodeMCU 5V or 3.3V
*/





#include "Free_Fonts.h" // Include the header file attached to this sketch

#include <Wire.h>
#include <Adafruit_MLX90614.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <Ticker.h>

// Use hardware SPI
TFT_eSPI tft = TFT_eSPI();
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
Ticker ticker;

int                     totalAgua = 0;
bool                    flag_erase = false;
unsigned long           drawTime = 0;
byte                    sensorInterrupt = 2;  // D4
float                   calibrationFactor = 4.5; // The hall-effect flow sensor outputs approximately 4.5 pulses per second per litre/minute of flow.
volatile byte           pulseCount;  
float                   flowRate;
unsigned long           flowMilliLitres;
unsigned long           totalMilliLitres;
unsigned long           oldTime;

//funcion para manejar interrupcion de sensor
void pulseCounter()
{
  // Increment the pulse counter
  pulseCount++;
  digitalWrite(LED_BUILTIN, LOW);
  digitalWrite(LED_BUILTIN, HIGH);
}

//funcion para ticker
void saveTotal(){
  if( totalMilliLitres - totalAgua > 100 ){
    totalAgua = totalMilliLitres;
  } else {
    totalAgua = 0;
    totalMilliLitres = 0;
  }
}

void escribirAgua(){
    tft.setFreeFont(FF43);
    tft.drawString(String(totalMilliLitres)+ " ml", 120, 90, GFXFF);// Print the string name of the font
    tft.setFreeFont(FF42);
    tft.drawString(String(flowMilliLitres)+ " ml", 120, 160, GFXFF);// Print the string name of the font
    //tft.setFreeFont(FF43);
    tft.drawString(String(mlx.readObjectTempC())+ " *C", 120, 270, GFXFF);// Print the string name of the font
}

void borrarAgua(){
    tft.setFreeFont(FF43);
    tft.drawString("                          ", 120, 90, GFXFF);  
    tft.setFreeFont(FF43);
    tft.drawString("                          ", 120, 160, GFXFF); 
    tft.setFreeFont(FF43);
    tft.drawString("                          ", 120, 270, GFXFF);  
}

void setup()
{
  
  // Initialize a serial connection for reporting values to the host
  Serial.begin(115200);
  tft.begin();
  mlx.begin();
  tft.setRotation(2);

  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  tft.fillScreen(TFT_BLACK);

  tft.setFreeFont(FF43);                 // Select the font
  tft.drawString("AGUA", 120, 30, GFXFF);// Print the string name of the font
  tft.drawString("CONSUMIDA", 120, 60, GFXFF);// Print the string name of the font

  tft.setFreeFont(FF42);
  tft.drawString("FLUJO DE AGUA", 120, 130, GFXFF);// Print the string name of the font

  tft.setFreeFont(FF42);                 // Select the font
  tft.drawString("TEMPERATURA", 120, 240, GFXFF);// Print the string name of the font
  
  // Set up the status LED line as an output
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);  // We have an active-low LED attached
  
  pulseCount        = 0;
  flowRate          = 0.0;
  flowMilliLitres   = 0;
  totalMilliLitres  = 0;
  oldTime           = 0;

  // The Hall-effect sensor is connected to pin 2 which uses interrupt 0.
  // Configured to trigger on a FALLING state change (transition from HIGH
  // state to LOW state)
  ticker.attach(60 /*segundos */,saveTotal);
  attachInterrupt(sensorInterrupt, pulseCounter, FALLING);
}

/**
 * Main program loop
 */
void loop()
{
  if((millis() - oldTime) > 500)    // Only process counters once per second
  { 
    // Disable the interrupt while calculating flow rate and sending the value to
    // the host
    detachInterrupt(sensorInterrupt);
    //Borrar el dato anterior de la pantalla
    borrarAgua();  
    // Because this loop may not complete in exactly 1 second intervals we calculate
    // the number of milliseconds that have passed since the last execution and use
    // that to scale the output. We also apply the calibrationFactor to scale the output
    // based on the number of pulses per second per units of measure (litres/minute in
    // this case) coming from the sensor.
    flowRate = ((1000.0 / (millis() - oldTime)) * pulseCount) / calibrationFactor;
    
    // Note the time this processing pass was executed. Note that because we've
    // disabled interrupts the millis() function won't actually be incrementing right
    // at this point, but it will still return the value it was set to just before
    // interrupts went away.
    oldTime = millis();
    
    // Divide the flow rate in litres/minute by 60 to determine how many litres have
    // passed through the sensor in this 1 second interval, then multiply by 1000 to
    // convert to millilitres.
    flowMilliLitres = (flowRate / 60) * 1000;
    
    // Add the millilitres passed in this second to the cumulative total
    totalMilliLitres += flowMilliLitres;
      
    unsigned int frac;
    
    // Print the flow rate for this second in litres / minute
    Serial.print("Flow rate: ");
    Serial.print(int(flowRate));  // Print the integer part of the variable
    Serial.print(".");             // Print the decimal point
    // Determine the fractional part. The 10 multiplier gives us 1 decimal place.
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
    
    // Reset the pulse counter so we can start incrementing again
    pulseCount = 0;
    escribirAgua();
  
    // Enable the interrupt again now that we've finished sending output
    
    attachInterrupt(sensorInterrupt, pulseCounter, FALLING);
  }
}

