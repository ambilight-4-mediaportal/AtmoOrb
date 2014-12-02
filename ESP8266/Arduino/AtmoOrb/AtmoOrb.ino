// AtmoOrb by Lightning303
//
// Developed for:
// "Arduino" Micro Pro
// ESP8266 Module Version 02
// Firmware Version: 0020000903 (AT 0.20 & SDK 0.9.3)
//
// Pin Connections:
//
// ESP8266
// UTXD           Arduino RX1
// URXD           Arduino TX0 (ESP8266 is 3.3V!)
// VCC            VCC (ESP8266 is 3.3V!)
// GND            GND
// CH_PD          VCC (ESP8266 is 3.3V!)
//
// Arduino
// RX1            ESP8266 UTXD
// TX0            ESP8266 URXD (ESP8266 is 3.3V!)
// VCC            VCC
// GND            GND
// 15             WS2812B Data
//
//
// We are using a baud rate of 115200 here.
// You can either change the ESP8266 baudrate with "AT+CIOBAUD=..."
// or change the program to use another one by editing "Serial1.begin(115200);"

#include "FastLED.h"

#define NUM_LEDS 24
#define DATA_PIN 15

#define wifiSSID "Your SSID"
#define wifiPassword "Your WiFi Password"
#define disableDHCP 0
#define staticIP "192.168.1.42"
#define serverPort 30003
#define broadcastIP "192.168.1.255"
#define broadcastPort 30003

CRGB leds[NUM_LEDS];

char serialBuffer[1000];
String ip;
boolean setupDone = false;
long previousCheckIPMillis = 0; 
long checkIPInterval = 5000;


void setup()
{
  FastLED.addLeds<WS2812B, DATA_PIN, RGB>(leds, NUM_LEDS);
  for (int i = 0; i < NUM_LEDS; i++)
  {
    leds[i] = CRGB(255,255,255);
  }
  FastLED.show();
  
  String setupMessage = "";
  
  Serial.begin(115200);
  Serial.setTimeout(5);
  Serial1.begin(115200);
  Serial1.setTimeout(5);
  
  delay(1000);
  Serial1.println("AT+RST");
  delay(10);
  Serial1.println("AT+CWMODE=1");
  delay(10);
  Serial1.println("AT+RST");
  delay(10);
  if (disableDHCP == 1)
  {
    Serial1.println("AT+CWDHCP=2,1");
    delay(10);
  }
  setupMessage = "AT+CWJAP=\"";
  setupMessage += wifiSSID;
  setupMessage += "\",\"";
  setupMessage += wifiPassword;
  setupMessage += "\"";
  Serial1.println(setupMessage);
  delay(5000);
  if (disableDHCP == 1)
  {
    setupMessage = "AT+CIPSTA=\"";
    setupMessage += staticIP;
    setupMessage += "\"";
    Serial1.println(setupMessage);
    delay(10);
  }
  Serial1.println("AT+CIPMUX=1");
  delay(10);
  setupMessage = "AT+CIPSERVER=1,";
  setupMessage += serverPort;
  Serial1.println(setupMessage);
  delay(10);
  setupDone = true;
}

void loop()
{
  if (Serial.available() > 0)
  {
     int len = Serial.readBytesUntil('<\n>', serialBuffer, sizeof(serialBuffer));
     String message = String(serialBuffer).substring(0,len-1);
     Serial1.println(message);
  }
  if (Serial1.available() > 0)
  {
    int len = Serial1.readBytesUntil('<\n>', serialBuffer, sizeof(serialBuffer));
    String message = String(serialBuffer).substring(0,len-1);

    // AT 0.20 ip syntax
    if (message.indexOf("+CIFSR:") > -1)
    {
      if (message.indexOf("+CIFSR:STAIP") > -1)
      {
        int start = message.indexOf("+CIFSR:STAIP");
        start = message.indexOf("\"", start) + 1; 
        ip = message.substring(start, message.indexOf("\"", start));
        Serial.println("IP: " + ip);
        broadcast();
      }
    }
    // AT pre 0.20 ip syntax
    else if (message.indexOf("AT+CIFSR") > -1)
    {
      int start = message.indexOf("AT+CIFSR");
      start = message.indexOf("\n", start) + 1;
      ip = message.substring(start, message.indexOf("\n", start) - 1);
      Serial.println("IP: " + ip);
      broadcast();
    }
    // Receiving boradcast messages not working with 0020000903, but with 0018000902-AI03
    // Not sure why. But static IP is supported.
    else if (message.indexOf("M-SEARCH") > -1)
    {
      Serial1.println("AT+CIFSR");
    }
    else if (message.indexOf("setcolor:") > -1)
    {
      int red = -1;
      int green = -1;
      int blue = -1;
      int start = message.lastIndexOf("setcolor:") + 9;
      int endValue = message.indexOf(';', start);

      if (start == 8 || endValue == -1 || (endValue - start) != 6)
      {
        return;
      }
           
      red = hexToDec(message.substring(start, start + 2));
      green = hexToDec(message.substring(start + 2, start + 4));
      blue = hexToDec(message.substring(start + 4, start + 6));

      if (red != -1 && green != -1 && blue != -1)
      {
        // Change LED color
        Serial.print("Red:");
        Serial.print(red);
        Serial.print(" Green:");
        Serial.print(green);
        Serial.print(" Blue:");
        Serial.println(blue);
      }
    }
    Serial.println(message);
  }
  if ((ip == "" || ip == "0.0.0.0") && setupDone && (millis() - previousCheckIPMillis > checkIPInterval))
  {
    Serial1.println("AT+CIFSR");
    previousCheckIPMillis = millis();
  }
}

void broadcast()
{
  if ((ip == "" || ip == "0.0.0.0") && setupDone)
  {
    return;
  }
  String broadcastConnection = "AT+CIPSTART=3,\"UDP\",\"";
  broadcastConnection += broadcastIP;
  broadcastConnection += "\",";
  broadcastConnection += broadcastPort;
  Serial1.println(broadcastConnection);
  delay(100);
  String broadcastReply = "AtmoOrb:";
  broadcastReply += ip;
  broadcastReply += ",";
  broadcastReply += serverPort;
  broadcastReply += ";";
  Serial1.print("AT+CIPSEND=3,");
  Serial1.println(broadcastReply.length());
  delay(5);
  Serial1.println(broadcastReply);
}

int hexToDec(String hex)
{
  char hexChar[hex.length()];
  hex.toCharArray(hexChar, hex.length() + 1);
  char* hexPos = hexChar;
  return strtol(hexPos, &hexPos, 16);
}
