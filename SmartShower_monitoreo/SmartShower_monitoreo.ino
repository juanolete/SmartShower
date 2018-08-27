 /*
 Sugested TFT connections for UNO and Atmega328 based boards
   sclk 13  // Don't change, this is the hardware SPI SCLK line
   mosi 11  // Don't change, this is the hardware SPI MOSI line
   cs   10  // Chip select for TFT display
   dc   9   // Data/command line
   rst  7   // Reset, you could connect this to the Arduino reset pin
*/

//Includes
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Wire.h>
#include <Adafruit_MLX90614.h>

//Definitions
#define FLUJO_PIN           2
#define TEMP_PIN            3
#define LED_G               6
#define LED_R               5
#define drawTime            5000 //tiempo [ms] entre duchas
float                       calibrationFactor  = 7.7639;

//hardware
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
RF24 radio(7, 8); // CE, CSN

//Software
int                     totalAgua           = 0;
bool                    flag_erase          = false;
volatile byte           pulseCount;  
float                   flowRate;
float                   flowMilliLitres;
float                   flow1;
float                   flow2;

unsigned long           temperature;
unsigned long           oldTime;
unsigned long           currentTime;
const byte              address[6]          = "00001";  
char                    message[32];

int i = 1;

//Functions
//funcion para manejar interrupcion de sensor
void pulseCounter(){
  pulseCount++;
}

String generateMessage(){
  String dato = String(flowMilliLitres)+":"+String(temperature);
  return dato;
}

//funcion para enviar mensaje
bool sendMessage(String dato){
  dato.toCharArray(message, sizeof(message));
  if (radio.write(&message, sizeof(message))){
    Serial.print("Mensaje: ");
    Serial.println(message);
    return true;
  } else {
    Serial.println("ERROR");
    return false;
  }
}
void printMedidas(){
  Serial.print("Flow rate: ");
  Serial.print(flowMilliLitres);
  Serial.print("mL/Sec"); 
  Serial.print("  Water temperature: "); 
  Serial.print(temperature);
  Serial.println("*C");
}
void setup() {
  Serial.begin(9600);
  //Inicio Hardware
  pinMode(FLUJO_PIN, INPUT_PULLUP);
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  mlx.begin();
  radio.begin();
  //Configuracion RF
  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_MIN);
  radio.stopListening();
  //Inicializacion de variables
  pulseCount        = 0;
  flowRate          = 0.0;
  flowMilliLitres   = 0;
  flow1             = 0;
  flow2             = 0; 
  i                 = 1; 
  temperature       = mlx.readObjectTempC();  
  attachInterrupt(digitalPinToInterrupt(FLUJO_PIN), pulseCounter, RISING);
  
  //Envia dato vacio
  oldTime           = millis(); 
  Serial.println("INICIO DE TRANSMISION: "+String(oldTime)+"ms");
  if(sendMessage(generateMessage())){
     digitalWrite(LED_G,HIGH);
     digitalWrite(LED_R,LOW);
  } else {
     digitalWrite(LED_G,LOW);
     digitalWrite(LED_R,HIGH);
  }
}

void loop(){
  currentTime = millis();
  if((currentTime - oldTime) > 450)    // Only process counters once per second
  { 
    detachInterrupt(FLUJO_PIN);
    flowRate = 2*((1000.0 / (float)(currentTime - oldTime)) * (float)pulseCount) / (float)calibrationFactor;
    flowRate = (flowRate / 60) * 1000;
    if (i){
      flow1 = flowRate;
      i = 0;
    } else {
      flow2 = flowRate;
      flowMilliLitres = (flow1 + flow2)/2;
      temperature = mlx.readObjectTempC();
      if(sendMessage(generateMessage())){
        digitalWrite(LED_G,HIGH);
        digitalWrite(LED_R,LOW);
      } else {
        digitalWrite(LED_G,LOW);
        digitalWrite(LED_R,HIGH);
      }
      i = 1;
    }
    //printMedidas();
    // Reset the pulse counter so we can start incrementing again
    pulseCount = 0;

    attachInterrupt(digitalPinToInterrupt(FLUJO_PIN), pulseCounter, RISING);
    oldTime = millis();
  }
}
