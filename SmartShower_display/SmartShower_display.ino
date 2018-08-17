
/* 2.2 TFT SPI 240x320 ILI9341 Setup

 Typical setup for ESP8266 NodeMCU ESP-12 is :

 Display SDO/MISO  to NodeMCU pin D6 (or leave disconnected if not reading TFT)
 Display LED       to NodeMCU pin VIN (or 5V, see below)
 Display SCK       to NodeMCU pin D5
 Display SDI/MOSI  to NodeMCU pin D7
 Display DC (RS/AO)to NodeMCU pin D1
 Display RESET     to NodeMCU pin RST 
 Display CS        to NodeMCU pin D2
 Display GND       to NodeMCU pin GND (0V)
 Display VCC       to NodeMCU 5V or 3.3V
/*

/* NRF24L01 Setup

Typical setup for ESP8266 NodeMCU ESP-12 is :

 RF GND       to NodeMCU pin GND (0V)
 RF VCC       to NodeMCU 5V or 3.3V
 RF SDO/MISO  to NodeMCU pin D6
 RF SCK       to NodeMCU pin D5
 RF SDI/MOSI  to NodeMCU pin D7
 RF CE        to NodeMCU pin D4 
 RF CS        to NodeMCU pin D8

*/
/*
* Arduino Wireless Communication Tutorial
*       Example 1 - Receiver Code
*                
* by Dejan Nedelkovski, www.HowToMechatronics.com
* 
* Library: TMRh20/RF24, https://github.com/tmrh20/RF24/
*/

#include "Free_Fonts.h" // Include the header file attached to this sketch
#include <user_interface.h>;//Biblioteca necessaria para acessar os Timer`s.
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "TFT_eSPI.h"
#include <Ticker.h>
#include <FS.h>
#include <Arduino.h>
#include <Stream.h>
#include <ArduinoJson.h>
#include <EEPROM.h>

// Includes for WiFi configure
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager

#define         tMaxInterrupt         10 //segundos
#define         TOGGLE_PIN            0
//Software
char*           flowMilliLitres;
int             flowAnterior = 0; 
float           totalMilliLitres = 0; 
float           totalAnterior = 0;
char*           temperature;
os_timer_t      tmr0;
const byte      address[6] = "00001";
Ticker          reset;

//manejar AP
boolean         m_conn_state = false;
boolean         m_ap_state = false;
int             flag_addr = 0;
long            flag_ap = 0;

//Hardware
TFT_eSPI        tft = TFT_eSPI();
RF24            radio(PIN_D4, PIN_D8); // CE, CSN
//for EEPROM use
int             memory = 8;

//To reset WifiManager values and Access Point config
void ISR_Reset() {
  wifiManager.resetSettings();
  delay(1000);
  ESP.reset();
}

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  Serial.println(myWiFiManager->getConfigPortalSSID());
  //entered config mode, make led toggle faster
  m_conn_state = false;
  m_ap_state = true;
}

/* Funcion para lanzar la interrupcion con el boton del rele, se tienen 3 acciones.
    Reset de ESP > 8seg, Reset de Wifi parameters > 3seg y accionar Rele  */

void ICACHE_RAM_ATTR button_interrupt() {
  button_state = digitalRead(TOGGLE_PIN);
  if (button_state != button_state_ant) {
    Serial.println("LANZAMIENTO DE INTERRUPCION");
    button_state_ant = button_state;

    Serial.print("Estado de boton= ");
    Serial.println(button_state);
    if (!button_state) {
      Serial.println("Boton presionado");
      push_time = millis();
      return;
    } else {
      Serial.println("Boton soltado");
      release_time = millis();
      Serial.println(release_time - push_time );

      if (release_time - push_time >= 5000) {
        Serial.println("Boton presionado 5 segundos");
        Serial.println("##ESP reboot");
        ESP.reset();
        return;

      } else if (release_time - push_time >= 3000) {
        Serial.println("Boton presionado 3 segundos");
        Serial.println("##WiFi configuration reset");
        Serial.println("##Rebooting");
        flag_ap = 1;
        eWriteLong(flag_addr, flag_ap);
        ISR_Reset();
        return;
      }
    }
  }
}

void WifiReconnect() {
  m_conn_state = false;
  while (WiFi.status() != WL_CONNECTED) {
    wifiManager.connectWifi("", "");
  }
  return;
}

void eWriteLong(int pos, long val) {
  byte* p = (byte*) &val;
  EEPROM.write(pos, *p);
  EEPROM.write(pos + 1, *(p + 1));
  EEPROM.write(pos + 2, *(p + 2));
  EEPROM.write(pos + 3, *(p + 3));
  EEPROM.write(pos + 4, *(p + 4));
  EEPROM.write(pos + 5, *(p + 5));
  EEPROM.write(pos + 6, *(p + 6));
  EEPROM.write(pos + 7, *(p + 7));
  EEPROM.commit();
}

//Read 8byte long, EEPROM
long eGetLong(int pos) {
  long val;
  byte* p = (byte*) &val;
  *p        = EEPROM.read(pos);
  *(p + 1)  = EEPROM.read(pos + 1);
  *(p + 2)  = EEPROM.read(pos + 2);
  *(p + 3)  = EEPROM.read(pos + 3);
  *(p + 4)  = EEPROM.read(pos + 4);
  *(p + 5)  = EEPROM.read(pos + 5);
  *(p + 6)  = EEPROM.read(pos + 6);
  *(p + 7)  = EEPROM.read(pos + 7);
  return val;
}


void decodeMessage(char* message){
  flowMilliLitres = strtok(message,":");
  temperature = strtok(NULL,":");
}

void escribirAgua(){
  tft.setFreeFont(FF43);
  tft.drawString(String(totalMilliLitres)+ " ml", 120, 90, GFXFF);// Print the string name of the font
  tft.setFreeFont(FF42);
  tft.drawString(String(flowMilliLitres)+ " ml", 120, 160, GFXFF);// Print the string name of the font
  //tft.setFreeFont(FF43);
  tft.drawString(String(temperature)+ " *C", 120, 270, GFXFF);// Print the string name of the font
}

void resetAgua(){
  borrarAgua();
  tft.drawString(String(0)+ " ml", 120, 90, GFXFF);// Print the string name of the font
  tft.setFreeFont(FF42);
  tft.drawString(String(0)+ " ml", 120, 160, GFXFF);// Print the string name of the font
  //tft.setFreeFont(FF43);
  tft.drawString(String(20)+ " *C", 120, 270, GFXFF);// Print the string name of the font
}

void borrarAgua(){
    tft.setFreeFont(FF43);
    tft.drawString("                          ", 120, 90, GFXFF);  
    tft.drawString("                          ", 120, 160, GFXFF);
    tft.drawString("                          ", 120, 270, GFXFF);  
}

void aguaHandler(){
  borrarAgua();
  tft.drawString("No data", 120, 90, GFXFF);
  tft.setFreeFont(FF42);
  tft.drawString("No data", 120, 160, GFXFF);
  tft.drawString("No data", 120, 270, GFXFF);
  //serverPost();
}

int first_start = 1;

void setup() {
  eeprom.BEGIN(memory);
  Serial.begin(9600);

  if (first_start) {
    eWriteLong(flag_addr, 1);
    first_start = 0;
  }
  flag_ap = eGetLong(flag_addr)
  attachInterrupt(digitalPinToInterrupt(TOGGLE_PIN), button_interrupt, CHANGE);
  wifiManager.setAPCallback(configModeCallback);

  if (flag_ap == 0) {
    WifiReconnect();
  } else {
    if (!wifiManager.autoConnect("SmartShower", "123456789")) { //you can change the pass "lapassmundial" to another one
      delay(3000);
      ESP.reset();
    }
  }
  //ACA SE DEBE DESCONECTAR DEL WIFI UNA VEZ CONECTADO
  flag_ap = 0;
  eWriteLong(flag_addr, flag_ap);

  tft.begin();
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

  resetAgua();
  
  radio.begin();
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_MIN);
  radio.startListening();
  radio.setPayloadSize(100);

  reset.once(10,aguaHandler);
}

void printData(){
  Serial.print("Flow: "); Serial.print(flowMilliLitres); Serial.print(" ml/s");
  Serial.print("  Total: "); Serial.print(totalMilliLitres); Serial.print(" ml");
  Serial.print("  Temperature: "); Serial.print(temperature); Serial.println(" *C");
}

void loop() {
  if (radio.available()) {
    reset.detach();
    reset.once(tMaxInterrupt,aguaHandler);
    char message[100] = "";
    radio.read(&message, sizeof(message));
    Serial.println(message);
    decodeMessage(message); 
    totalMilliLitres += atof(flowMilliLitres)/2;
    totalAnterior = totalMilliLitres;
    borrarAgua();
    escribirAgua();
    printData();
  }
}
