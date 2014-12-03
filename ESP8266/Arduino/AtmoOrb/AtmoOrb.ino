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
boolean initialStart = false;
long previousCheckIPMillis = 0; 
long checkIPInterval = 5000;


void setup()
{
  FastLED.addLeds<WS2812B, DATA_PIN, RGB>(leds, NUM_LEDS);
  for (int i = 0; i < NUM_LEDS; i++)
  {
    leds[i] = CRGB(255, 255, 255);
  }
  FastLED.show();
  
  String setupMessage = "";
  
  Serial.begin(115200);
  Serial.setTimeout(5);
  Serial1.begin(115200);
  Serial1.setTimeout(5);
  
  delay(1000);
  
  // Reset ESP8266
  Serial1.println("AT+RST");
  delay(10);
  
  // Change to Station mode
  Serial1.println("AT+CWMODE=1");
  delay(10);
  
  // Reset ESP8266
  Serial1.println("AT+RST");
  delay(10);
  
  // Disable DHCP
  if (disableDHCP == 1)
  {
    Serial1.println("AT+CWDHCP=2,1");
    delay(10);
  }
  
  // Join access point
  setupMessage = "AT+CWJAP=\"";
  setupMessage += wifiSSID;
  setupMessage += "\",\"";
  setupMessage += wifiPassword;
  setupMessage += "\"";
  Serial1.println(setupMessage);
  delay(5000);
  
  // Set static ip
  if (disableDHCP == 1)
  {
    setupMessage = "AT+CIPSTA=\"";
    setupMessage += staticIP;
    setupMessage += "\"";
    Serial1.println(setupMessage);
    delay(10);
  }
  
  // Enable multiple connections
  Serial1.println("AT+CIPMUX=1");
  delay(10);
  
  // Setup server
  setupMessage = "AT+CIPSERVER=1,";
  setupMessage += serverPort;
  Serial1.println(setupMessage);
  delay(10);
  
  // Setup client
  setupMessage = "AT+CIPSTART=2,\"UDP\",\"";
  setupMessage += broadcastIP;
  setupMessage += "\",";
  setupMessage += broadcastPort;
  Serial1.println(setupMessage);
  
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
        if (isValidIp4(message.substring(start, message.indexOf("\"", start))) > 0)
        {
          ip = message.substring(start, message.indexOf("\"", start));
          Serial.println("IP: " + ip);
          String broadcastMessage = "AtmoOrbAddress:";
          broadcastMessage += ip;
          broadcastMessage += ",";
          broadcastMessage += serverPort;
          broadcastMessage += ";";
          broadcast(broadcastMessage);
          
          if (!initialStart)
          {
            initialStart = true;
            for (int i = 0; i < NUM_LEDS; i++)
            {
              leds[i] = CRGB(0, 0, 255);
            }
            FastLED.show();
          }
        }
      }
    }
    // AT pre 0.20 ip syntax
    else if (message.indexOf("AT+CIFSR") > -1)
    {
      int start = message.indexOf("AT+CIFSR");
      start = message.indexOf("\n", start) + 1;
      if (isValidIp4(message.substring(start, message.indexOf("\n", start) - 1)) > 0)
      {
        ip = message.substring(start, message.indexOf("\n", start) - 1);
        Serial.println("IP: " + ip);
        String broadcastMessage = "AtmoOrbAddress:";
        broadcastMessage += ip;
        broadcastMessage += ",";
        broadcastMessage += serverPort;
        broadcastMessage += ";";
        broadcast(broadcastMessage);
        
        if (!initialStart)
        {
          initialStart = true;
          for (int i = 0; i < NUM_LEDS; i++)
          {
            leds[i] = CRGB(0, 0, 255);
          }
          FastLED.show();
        }
      }
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

      if (endValue == -1 || (endValue - start) != 6)
      {
        return;
      }
           
      red = hexToDec(message.substring(start, start + 2));
      green = hexToDec(message.substring(start + 2, start + 4));
      blue = hexToDec(message.substring(start + 4, start + 6));

      if (red != -1 && green != -1 && blue != -1)
      {
        for (int i = 0; i < NUM_LEDS; i++)
        {
          leds[i] = CRGB(red, green, blue);
        }
        FastLED.show();
      }
    }
    else if (message.indexOf("setcolors:") > -1)
    {
      int startPos = message.lastIndexOf("setcolors:") + 10;
      boolean success = false;
      int i = 0;
      while (startPos < message.length())
      {
        int endPos = message.indexOf(",", startPos);
        if (endPos == -1 && message.indexOf(";", startPos))
        {
          endPos = message.indexOf(";", startPos);
        }
        if (endPos == -1)
        {
          break;
        }
        if ((endPos - startPos) == 6 && i < NUM_LEDS)
        {
          success = true;
          leds[i] = CRGB(hexToDec(message.substring(startPos, startPos + 2)), hexToDec(message.substring(startPos + 2, startPos + 4)), hexToDec(message.substring(startPos + 4, startPos + 6)));
          i++;
        }
        startPos = endPos + 1;
      }
      if (success)
      {
        FastLED.show();
      }
    }
    else if (message.indexOf("getledcount;") > -1)
    {
      String broadcastMessage = "AtmoOrbLEDCount:";
      broadcastMessage += NUM_LEDS;
      broadcastMessage += ";";
      broadcast(broadcastMessage);
    }
    Serial.println(message);
  }
  if ((ip == "" || ip == "0.0.0.0") && setupDone && (millis() - previousCheckIPMillis > checkIPInterval))
  {
    Serial1.println("AT+CIFSR");
    previousCheckIPMillis = millis();
  }
}

void broadcast(String message)
{
  Serial1.print("AT+CIPSEND=2,");
  Serial1.println(message.length());
  delay(5);
  Serial1.println(message);
}

int hexToDec(String hex)
{
  char hexChar[hex.length()];
  hex.toCharArray(hexChar, hex.length() + 1);
  char* hexPos = hexChar;
  return strtol(hexPos, &hexPos, 16);
}

// http://stackoverflow.com/questions/791982/determine-if-a-string-is-a-valid-ip-address-in-c/792645#792645
int isValidIp4 (String ipString)
{
  char ipCharArray[ipString.length()];
  int segs = 0;   /* Segment count. */
  int chcnt = 0;  /* Character count within segment. */
  int accum = 0;  /* Accumulator for segment. */
    
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
