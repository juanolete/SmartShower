//Includes
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_MLX90614.h>
#include <TFT_ILI9341.h> 
#include <MsTimer2.h>

//Definitions
#define FLUJO_PIN           2
#define TEMP_PIN            3
#define drawTime            45000 //tiempo [ms] entre duchas
#define calibrationFactor   8

//hardware
TFT_ILI9341 tft = TFT_ILI9341();
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
RF24 radio(7, 8); // CE, CSN

//Software
int                     totalAgua           = 0;
bool                    flag_erase          = false;
volatile byte           pulseCount;  
float                   flowRate;
unsigned long           flowMilliLitres;
unsigned long           totalMilliLitres;
unsigned long           temperature;
unsigned long           oldTime;
unsigned long           currentTime;
const byte              address[6]          = "00001";  
char                    message[32];
StaticJsonDocument<200> doc;
JsonObject              root;

//Functions
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
//funcion para generar el mensaje con las variables
String generateMessage(){
  root = doc.to<JsonObject>();
  root["f"] = String(flowMilliLitres);
  root["t"] = String(totalMilliLitres);
  root["T"] = String(temperature);
  String dato;
  serializeJson(root,dato);
  return dato;
}

String generateMessage2(){
  String dato = String(flowMilliLitres)+":"+String(totalMilliLitres)+":"+String(temperature);
  return dato;
}

//funcion para enviar mensaje
void sendMessage(String dato){
  dato.toCharArray(message, sizeof(message));
  //char text[] = "Hello World";
  if (radio.write(&message, sizeof(message))){
    Serial.print("Mensaje: ");
    Serial.println(message);
  } else {
    Serial.println("ERROR");
  }
}
void printMedidas(){
  Serial.print("Flow rate: ");
  Serial.print(flowMilliLitres);
  Serial.print("mL/Sec"); 

  // Print the cumulative total of litres flowed since starting
  Serial.print("  Output Liquid Quantity: ");             // Output separator
  Serial.print(totalMilliLitres);
  Serial.print("mL");

  Serial.print("  Water temperature: ");             // Output separator
  Serial.print(temperature);
  Serial.println("*C");
}

void setup() {
  Serial.begin(9600);
  //Inicio Hardware
  mlx.begin();
  tft.init();
  radio.begin();
  pinMode(FLUJO_PIN, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  //Configuracion RF
  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_MIN);
  radio.stopListening();
  //Timer para saveTotal
  MsTimer2::set(drawTime, saveTotal); // 10000ms period
  MsTimer2::start();
  //Inicializacion de variables
  pulseCount        = 0;
  flowRate          = 0.0;
  flowMilliLitres   = 0;
  totalMilliLitres  = 0;
  oldTime           = millis();   

  attachInterrupt(digitalPinToInterrupt(FLUJO_PIN), pulseCounter, RISING); 
}

void loop(){
  currentTime = millis();
  if((currentTime - oldTime) > 500)    // Only process counters once per second
  { 
    detachInterrupt(FLUJO_PIN);
    flowRate = ((1000.0 / (currentTime - oldTime)) * pulseCount) / calibrationFactor;
    flowMilliLitres = (flowRate / 60) * 1000;
    totalMilliLitres += flowMilliLitres;
    temperature = mlx.readObjectTempC();
    //printMedidas();
    sendMessage(generateMessage2());
    // Reset the pulse counter so we can start incrementing again
    pulseCount = 0;

    attachInterrupt(digitalPinToInterrupt(FLUJO_PIN), pulseCounter, RISING);
    oldTime = millis();
  }
}
