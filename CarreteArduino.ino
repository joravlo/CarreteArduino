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

WiFiUDP Udp;

int getDepth(String sentence); // Función que devuelve la profundida
String getValue(String data, char separator, int index);
void printLcd(String message);
void printMeters(String sentence);


void setup() {
  Serial.begin(4800);
  
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
  // if there's data available, read a packet
  int packetSize = Udp.parsePacket();
  if (packetSize) {
    // read the packet into packetBufffer
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

}

void printLcd(String message){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(message);
}

void printMeters(String sentence){
  if (sentence.substring(0,6) == "$INDPT"){
    String depth = getValue(sentence, ',', 1);
    String message = "Metros: " + depth;
    lastDepth = getDepth(sentence);
    printLcd(message);
   }
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
