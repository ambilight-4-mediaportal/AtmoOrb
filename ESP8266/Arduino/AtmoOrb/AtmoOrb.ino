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

#define NUM_LEDS 27 // Number of leds
#define DATA_PIN 15 // Data pin for leds
#define LED_ARRANGEMENT "ring" // Arrangement of the leds (ring, ring2, ring3, matrix, snakematrix)
#define ID "1" // Id of this lamp (can be string or integer)
#define WIFI_SSID "Your WiFi SSID" // SSID of your wifi network
#define WIFI_PASSWORD "Your WiFi Password" // Password to your wifi network
#define DISABLE_DHCP 0 // Disable DHCP
#define STATIC_IP "192.168.1.42" // Static ip if DHCP is disabled
#define BROADCAST_IP "192.168.1.255" // Broadcast ip of your network
#define PORT 30003 // Port you want to use for broadcasting and for the server
#define SMOOTH_STEPS 40 // Steps to take for smoothing colors
#define SMOOTH_DELAY 5 // Delay between smoothing steps
#define DEBUG 1 // Debug via serial port
#define STARTUP_RED 0
#define STARTUP_GREEN 0
#define STARTUP_BLUE 200

CRGB leds[NUM_LEDS];
char serialBuffer[500];
String ip;
boolean setupDone = false;
boolean initialStart = false;
unsigned long previousCheckIP = 0; 
int checkIPInterval = 5000;

byte nextColor[3];
byte prevColor[3];
byte smoothStep = SMOOTH_STEPS;
unsigned long smoothMillis;

char* readyString = "ready";
char* okString = "OK";
char* noChangeString = "no change";


void setup()
{
  TXLED1;

  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);

  Serial.begin(115200);
  Serial.setTimeout(5);
  Serial1.begin(115200);
  Serial1.setTimeout(5000); 

  // Reset ESP8266
  Serial1.println(F("AT+RST"));
  Serial1.find(readyString);

  // Change to Station mode
  Serial1.println(F("AT+CWMODE=1"));
  Serial1.find(noChangeString);

  // Reset ESP8266
  Serial1.println(F("AT+RST"));
  Serial1.find(readyString);

  // Disable DHCP
  if (DISABLE_DHCP == 1)
  {
    Serial1.println(F("AT+CWDHCP=2,1"));
    Serial1.find(okString);
  }

  // Join access point
  Serial1.setTimeout(10000);
  Serial1.print(F("AT+CWJAP=\""));
  Serial1.print(WIFI_SSID);
  Serial1.print(F("\",\""));
  Serial1.print(WIFI_PASSWORD);
  Serial1.println(F("\""));
  Serial1.find(okString);
  Serial1.setTimeout(5000);

  // Set static ip
  if (DISABLE_DHCP == 1)
  {
    Serial1.print(F("AT+CIPSTA=\""));
    Serial1.print(STATIC_IP);
    Serial1.println(F("\""));
    Serial1.find(okString);
  }

  // Enable multiple connections
  Serial1.println(F("AT+CIPMUX=1"));
  Serial1.find(okString);

  // Setup server
  Serial1.print(F("AT+CIPSERVER=1,"));
  Serial1.println(PORT);
  Serial1.find(okString);

  // Setup client
  Serial1.print(F("AT+CIPSTART=2,\"UDP\",\""));
  Serial1.print(BROADCAST_IP);
  Serial1.print(F("\","));
  Serial1.println(PORT);
  Serial1.find(okString);
  Serial1.setTimeout(5);  
  setupDone = true;
}

void loop()
{
  digitalWrite(17, HIGH);
  if (Serial1.available() > 0)
  {
    int len = Serial1.readBytes(serialBuffer, sizeof(serialBuffer));
    String message = String(serialBuffer).substring(0,len-1);

    // AT 0.20 ip syntax
    if (message.indexOf(F("+CIFSR:")) > -1)
    {
      if (message.indexOf(F("+CIFSR:STAIP")) > -1)
      {
        int start = message.indexOf(F("+CIFSR:STAIP"));
        start = message.indexOf(F("\""), start) + 1; 
        if (isValidIp4(message.substring(start, message.indexOf(F("\""), start))) > 0)
        {
          ip = message.substring(start, message.indexOf(F("\""), start));
          ipReceived();
        }
      }
    }
    // AT pre 0.20 ip syntax
    else if (message.indexOf(F("AT+CIFSR")) > -1)
    {
      int start = message.indexOf(F("AT+CIFSR"));
      start = message.indexOf(F("\n"), start) + 1;
      if (isValidIp4(message.substring(start, message.indexOf(F("\n"), start) - 1)) > 0)
      {
        ip = message.substring(start, message.indexOf(F("\n"), start) - 1);
        ipReceived();
      }
    }
    // Receiving boradcast messages not working with 0020000903, but with 0018000902-AI03
    // Not sure why. But static IP is supported.
    else if (message.indexOf(F("M-SEARCH")) > -1)
    {
      Serial1.println(F("AT+CIFSR"));
    }
    else if (message.indexOf(F("setcolor:")) > -1)
    {
      byte red = -1;
      byte green = -1;
      byte blue = -1;
      int start = message.lastIndexOf(F("setcolor:")) + 9;
      int endValue = message.indexOf(';', start);

      if (endValue == -1 || (endValue - start) != 6)
      {
        return;
      }

      red = hexToDec(message.substring(start, start + 2));
      green = hexToDec(message.substring(start + 2, start + 4));
      blue = hexToDec(message.substring(start + 4, start + 6));

      if (red != -1 && green != -1 && blue != -1)
      {    
        setSmoothColor(red, green, blue);
      }
    }
    else if (message.indexOf(F("getledcount;")) > -1)
    {
      String tempString = F("AtmoOrb:");
      tempString += ID;
      tempString += F(":ledcount:");
      tempString += NUM_LEDS;
      tempString += F(";");
      broadcast(tempString);
    }
    else if (message.indexOf(F("getledgeo;")) > -1)
    {
      String tempString = F("AtmoOrb:arrangement:");
      tempString += ID;
      tempString += F(":arrangement:");
      tempString += LED_ARRANGEMENT;
      tempString += F(";");
      broadcast(tempString);
    }
    if (DEBUG == 1)
    {
      Serial.println(message);
    }
  }
  if (DEBUG == 1)
  {
    if (Serial.available() > 0)
    {
      int len = Serial.readBytes(serialBuffer, sizeof(serialBuffer));
      String message = String(serialBuffer).substring(0,len-1);
      Serial1.println(message);
    }
  }
  if (smoothStep < SMOOTH_STEPS && millis() >= (smoothMillis + (SMOOTH_DELAY * (smoothStep + 1))))
  {
    smoothColor();
  }
  if ((ip == "" || ip == F("0.0.0.0")) && setupDone && (millis() - previousCheckIP > checkIPInterval))
  {
    Serial1.println(F("AT+CIFSR"));
    previousCheckIP = millis();
  }
}

void broadcast(String message)
{
  Serial1.print(F("AT+CIPSEND=2,"));
  Serial1.println(message.length());
  Serial1.find(">");
  Serial1.println(message);
}

byte hexToDec(String hex)
{
  char hexChar[hex.length()];
  hex.toCharArray(hexChar, hex.length() + 1);
  char* hexPos = hexChar;
  return strtol(hexPos, &hexPos, 16);
}

// http://stackoverflow.com/questions/791982/determine-if-a-string-is-a-valid-ip-address-in-c/792645#792645
byte isValidIp4 (String ipString)
{
  char ipCharArray[ipString.length()];
  byte segs = 0;   /* Segment count. */
  byte chcnt = 0;  /* Character count within segment. */
  byte accum = 0;  /* Accumulator for segment. */

  ipString.toCharArray(ipCharArray, ipString.length());

  char* str = ipCharArray;

  /* Catch NULL pointer. */

  if (str == NULL)
  {
    return 0;
  }

  /* Process every character in string. */
  while (*str != '\0')
  {
    /* Segment changeover. */
    if (*str == '.')
    {
      /* Must have some digits in segment. */
      if (chcnt == 0)
      {
        return 0;
      }

      /* Limit number of segments. */
      if (++segs == 4)
      {
        return 0;
      }

      /* Reset segment values and restart loop. */
      chcnt = accum = 0;
      str++;
      continue;
    }

    /* Check numeric. */
    if ((*str < '0') || (*str > '9'))
    {
      return 0;
    }

    /* Accumulate and check segment. */
    if ((accum = accum * 10 + *str - '0') > 255)
    {
      return 0;
    }

    /* Advance other segment specific stuff and continue loop. */
    chcnt++;
    str++;
  }

  /* Check enough segments and enough characters in last segment. */
  if (segs != 3)
  {
    return 0;
  }

  if (chcnt == 0)
  {
    return 0;
  }

  /* Address okay. */
  return 1;
}

void setSmoothColor(byte red, byte green, byte blue)
{
  if (smoothStep == SMOOTH_STEPS)
  {
    prevColor[0] = nextColor[0];
    prevColor[1] = nextColor[1];
    prevColor[2] = nextColor[2];
    
    nextColor[0] = red;
    nextColor[1] = green;
    nextColor[2] = blue;
    
    smoothMillis = millis();
    smoothStep = 0;
  }
}

void smoothColor()
{ 
  smoothStep++;

  byte red = prevColor[0] + (((nextColor[0] - prevColor[0]) * smoothStep) / SMOOTH_STEPS);
  byte green = prevColor[1] + (((nextColor[1] - prevColor[1]) * smoothStep) / SMOOTH_STEPS);
  byte blue = prevColor[2] + (((nextColor[2] - prevColor[2]) * smoothStep) / SMOOTH_STEPS);

  for (byte i = 0; i < NUM_LEDS; i++)
  {
    leds[i] = CRGB(red, green, blue);
  }
  FastLED.show();
}

void ipReceived()
{
  if (DEBUG == 1)
  {
    Serial.print(F("IP: "));
    Serial.println(ip);
  }
  String tempString = F("AtmoOrb:");
  tempString += ID;
  tempString += F(":address:");
  tempString += ip;
  tempString += F(",");
  tempString += PORT;
  tempString += F(";");
  broadcast(tempString);

  if (!initialStart)
  {
    initialStart = true;
    setSmoothColor(STARTUP_RED, STARTUP_GREEN, STARTUP_BLUE);
  }
}

