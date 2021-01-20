/*
  UDPSendReceive.pde:
   Este boceto recibe cadenas de mensajes UDP, las imprime en el puerto serie
   y envía una cadena de "reconocimiento" al remitente

   Se incluye un boceto de procesamiento al final del archivo que se puede usar para enviar
   y recibió mensajes para probarlos con una computadora.

   creado el 21 de agosto de 2010
   por Michael Margolis

   Este código es de dominio público.

   adaptado de ejemplos de bibliotecas de Ethernet
*/


#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#ifndef STASSID
#define STASSID "NMEA2WIFI"
#define STAPSK  "12345678"
#endif

LiquidCrystal_I2C lcd(0x27, 16, 2);

unsigned int localPort = 2000;      // local port to listen on

// buffers for receiving and sending data
char packetBuffer[UDP_TX_PACKET_MAX_SIZE + 1]; //buffer to hold incoming packet,
char  ReplyBuffer[] = "acknowledged\r\n";       // a string to send back
int lastDepth = 0;
int countDepth = 0;
int countOk = 0;
float countCable = 0;
boolean isManual = false;
const long interval = 1000; 
unsigned long previousMillis = 0; 

//Pines
const int pinButtonUp = D6;
const int pinButtonDown = D7;
const int pinButtonOk = D8;
const int pinCountCable = D5;
const int pinMotorLeft = D3;
const int pinMotorRight = D4;
const int pinMotorPWM = D2;
const int pinModo = D0;
const int pinDirManual = D9;

WiFiUDP Udp;

int getDepth(String sentence); // Función que devuelve la profundida
String getValue(String data, char separator, int index);
void printLcd(String message);
void printMeters(String sentence);
void printTable(String meters, String count, float countCable);
void rotateLeft();
void rotateRight();
void stopMotor();


void setup() {
  Serial.begin(4800);

  pinMode(pinButtonUp, INPUT);
  pinMode(pinButtonDown, INPUT);
  pinMode(pinButtonOk, INPUT);
  pinMode(pinMotorLeft, OUTPUT);
  pinMode(pinMotorRight, OUTPUT);
  pinMode(pinModo, INPUT);
  pinMode(pinDirManual, INPUT);
  
  Wire.begin(4, 5);
  lcd.init();
  lcd.backlight();
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(STASSID, STAPSK);
  
  while (WiFi.status() != WL_CONNECTED) {
    printLcd("Conectando...");
    delay(2000);
  }

  printLcd("Conectado");
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());
  Serial.printf("UDP server on port %d\n", localPort);
  Udp.begin(localPort);
}

void loop() {
  int stateButtonUp = digitalRead(pinButtonUp);
  int stateButtonDown = digitalRead(pinButtonDown);
  int stateButtonOk = digitalRead(pinButtonOk);
  int stateCountCable = digitalRead(pinCountCable);

  // if there's data available, read a packet
  unsigned long currentMillis = millis();
  int packetSize = Udp.parsePacket();
  if (packetSize && currentMillis - previousMillis >= interval) {
    // read the packet into packetBufffer
    previousMillis = currentMillis;
    int n = Udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
    packetBuffer[n] = 0;
    Serial.println("Contents:");
    String sentence = String(packetBuffer);
    Serial.println(sentence);
    printMeters(sentence);
    
    // send a reply, to the IP address and port that sent us the packet we received
    Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
    Udp.write(ReplyBuffer);
    Udp.endPacket();
  }

  if (pinModo == LOW) {
    isManual = true;
  } else {
    isManual = false;  
  }

  if(stateButtonUp == 1) {
    countDepth++;
    String count = String(countDepth);
    String lastDepthString = String(lastDepth);
    printTable(lastDepthString, count, countCable);
    delay(500);
  }

  if(stateButtonDown == 1) {
    countDepth--;
    String count = String(countDepth);
    String lastDepthString = String(lastDepth);
    printTable(lastDepthString, count, countCable);
    delay(500);
  }

  if(stateButtonOk == 1){
      countOk = countDepth;
      delay(500);
    }

 if (stateCountCable == LOW) {
    if (!isManual && pinDirManuel == HIGH) {
        countCable = countCable + 0.25;
        String count = String(countDepth);
        String lastDepthString = String(lastDepth);
        printTable(lastDepthString, count, countCable);
     } else if (!isManual && pinDirManuel == LOW) {
      countCable = countCable - 0.25;
        String count = String(countDepth);
        String lastDepthString = String(lastDepth);
        printTable(lastDepthString, count, countCable);  
      }

     while(stateCountCable == LOW){                 //antirebote
        stateCountCable = digitalRead(pinCountCable);
      }
  }   

}

void printLcd(String message){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(message);
}

void printMeters(String sentence){
  if (sentence.substring(0,6) == "$INDPT"){
    String depth = getValue(sentence, ',', 1);
    lastDepth = getDepth(sentence);
    String count = String(countDepth);
    printTable(depth, count, countCable);
   }
}

void printTable(String meters, String count, float countCable) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sonda");
  lcd.setCursor(1, 1);
  lcd.print(meters);
  lcd.setCursor(6, 0);
  lcd.print("Fondo");
  lcd.setCursor(8, 1);
  lcd.print(count);
  lcd.setCursor(12, 0);
  lcd.print("Cable");
  lcd.setCursor(12, 1);
  String countCableString = String(countCable);
  lcd.print(countCableString);
}

int getDepth(String sentence) {
  int depth;
  if (sentence.substring(0,6) == "$INDPT"){
    depth = getValue(sentence, ',', 1).toInt();
   }
    
  return depth;
}

String getValue(String data, char separator, int index)
{
    int found = 0;
    int strIndex[] = { 0, -1 };
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void rotateLeft(){
  digitalWrite (pinMotorLeft, HIGH);
  digitalWrite (pinMotorRight, LOW); 
  }


void rotateRight(){
  digitalWrite (pinMotorRight, HIGH);
  digitalWrite (pinMotorLeft, LOW); 
  }

void stopMotor() {
  digitalWrite (pinMotorRight, LOW);
  digitalWrite (pinMotorLeft, LOW);
  }
