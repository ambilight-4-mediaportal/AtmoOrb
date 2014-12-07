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

#define NUM_LEDS 27
#define DATA_PIN 15
// "ring" or "matrix"
#define LED_GEO "ring"

#define WIFI_SSID "Your SSID"
#define WIFI_PASSWORD "Your WiFi Password"
#define DISABLE_DHCP 0
#define STATIC_IP "192.168.1.42"
#define BROADCAST_IP "192.168.1.255"
#define PORT 30003

CRGB leds[NUM_LEDS];

char serialBuffer[500];
String ip;
boolean setupDone = false;
boolean initialStart = false;
long previousCheckIPMillis = 0; 
long checkIPInterval = 5000;
int tempRGB[NUM_LEDS][3];
String tempString;


#define SMOOTH_STEPS 10
#define SMOOTH_DELAY 5

int prevColor[NUM_LEDS][3];
int nextColor[NUM_LEDS][3];
int smoothStep = SMOOTH_STEPS;
unsigned long smoothMillis = 0;


void setup()
{
  FastLED.addLeds<WS2812B, DATA_PIN, RGB>(leds, NUM_LEDS);
    
  Serial.begin(115200);
  Serial.setTimeout(5);
  Serial1.begin(115200);
  Serial1.setTimeout(5);
  
  FastLED.addLeds<WS2812B, DATA_PIN, RGB>(leds, NUM_LEDS);
  
  
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
  if (DISABLE_DHCP == 1)
  {
    Serial1.println("AT+CWDHCP=2,1");
    delay(10);
  }
  
  // Join access point
  tempString = "AT+CWJAP=\"";
  tempString += WIFI_SSID;
  tempString += "\",\"";
  tempString += WIFI_PASSWORD;
  tempString += "\"";
  Serial1.println(tempString);
  delay(5000);
  
  // Set static ip
  if (DISABLE_DHCP == 1)
  {
    tempString = "AT+CIPSTA=\"";
    tempString += STATIC_IP;
    tempString += "\"";
    Serial1.println(tempString);
    delay(10);
  }
  
  // Enable multiple connections
  Serial1.println("AT+CIPMUX=1");
  delay(10);
  
  // Setup server
  tempString = "AT+CIPSERVER=1,";
  tempString += PORT;
  Serial1.println(tempString);
  delay(10);
  
  // Setup client
  tempString = "AT+CIPSTART=2,\"UDP\",\"";
  tempString += BROADCAST_IP;
  tempString += "\",";
  tempString += PORT;
  Serial1.println(tempString);
  
  setupDone = true;
}

void loop()
{
  if (Serial.available() > 0)
  {
     int len = Serial.readBytesUntil('<\n>', serialBuffer, sizeof(serialBuffer));
     tempString = String(serialBuffer).substring(0,len-1);
     Serial1.println(tempString);
  }
  if (Serial1.available() > 0)
  {
    int len = Serial1.readBytesUntil('<\n>', serialBuffer, sizeof(serialBuffer));
    tempString = String(serialBuffer).substring(0,len-1);

    // AT 0.20 ip syntax
    if (tempString.indexOf("+CIFSR:") > -1)
    {
      if (tempString.indexOf("+CIFSR:STAIP") > -1)
      {
        int start = tempString.indexOf("+CIFSR:STAIP");
        start = tempString.indexOf("\"", start) + 1; 
        if (isValidIp4(tempString.substring(start, tempString.indexOf("\"", start))) > 0)
        {
          ip = tempString.substring(start, tempString.indexOf("\"", start));
          Serial.println("IP: " + ip);
          tempString = "AtmoOrbAddress:";
          tempString += ip;
          tempString += ",";
          tempString += PORT;
          tempString += ";";
          broadcast(tempString);
          
          if (!initialStart)
          {
            initialStart = true;
            for (int i = 0; i < NUM_LEDS; i++)
            {
              tempRGB[i][0] = 0;
              tempRGB[i][1] = 0;
              tempRGB[i][2] = 255;
            }
            setSmoothColors(tempRGB);
          }
        }
      }
    }
    // AT pre 0.20 ip syntax
    else if (tempString.indexOf("AT+CIFSR") > -1)
    {
      int start = tempString.indexOf("AT+CIFSR");
      start = tempString.indexOf("\n", start) + 1;
      if (isValidIp4(tempString.substring(start, tempString.indexOf("\n", start) - 1)) > 0)
      {
        ip = tempString.substring(start, tempString.indexOf("\n", start) - 1);
        Serial.println("IP: " + ip);
        tempString = "AtmoOrbAddress:";
        tempString += ip;
        tempString += ",";
        tempString += PORT;
        tempString += ";";
        broadcast(tempString);
        
        if (!initialStart)
        {
          initialStart = true;
          for (int i = 0; i < NUM_LEDS; i++)
          {
            tempRGB[i][0] = 0;
            tempRGB[i][1] = 0;
            tempRGB[i][2] = 255;
          }
          setSmoothColors(tempRGB);
        }
      }
    }
    // Receiving boradcast messages not working with 0020000903, but with 0018000902-AI03
    // Not sure why. But static IP is supported.
    else if (tempString.indexOf("M-SEARCH") > -1)
    {
      Serial1.println("AT+CIFSR");
    }
    else if (tempString.indexOf("setcolor:") > -1)
    {
      int red = -1;
      int green = -1;
      int blue = -1;
      int start = tempString.lastIndexOf("setcolor:") + 9;
      int endValue = tempString.indexOf(';', start);

      if (endValue == -1 || (endValue - start) != 6)
      {
        return;
      }
           
      red = hexToDec(tempString.substring(start, start + 2));
      green = hexToDec(tempString.substring(start + 2, start + 4));
      blue = hexToDec(tempString.substring(start + 4, start + 6));

      if (red != -1 && green != -1 && blue != -1)
      {
          for (int i = 0; i < NUM_LEDS; i++)
          {
            tempRGB[i][0] = red;
            tempRGB[i][1] = green;
            tempRGB[i][2] = blue;
          }
          setSmoothColors(tempRGB);
      }
    }
    else if (tempString.indexOf("setcolors:") > -1)
    {
      int startPos = tempString.lastIndexOf("setcolors:") + 10;
      boolean success = false;
      int i = 0;
      while (startPos < tempString.length())
      {
        int endPos = tempString.indexOf(",", startPos);
        if (endPos == -1 && tempString.indexOf(";", startPos))
        {
          endPos = tempString.indexOf(";", startPos);
        }
        if (endPos == -1)
        {
          break;
        }
        if ((endPos - startPos) == 6 && i < NUM_LEDS)
        {
          success = true;
          tempRGB[i][0] = hexToDec(tempString.substring(startPos, startPos + 2));
          tempRGB[i][1] = hexToDec(tempString.substring(startPos + 2, startPos + 4));
          tempRGB[i][2] = hexToDec(tempString.substring(startPos + 4, startPos + 6));
          i++;
        }
        startPos = endPos + 1;
      }
      if (success)
      {
        setSmoothColors(tempRGB);
      }
    }
    else if (tempString.indexOf("getledcount;") > -1)
    {
      tempString = "AtmoOrbLEDCount:";
      tempString += NUM_LEDS;
      tempString += ";";
      broadcast(tempString);
    }
    else if (tempString.indexOf("getledgeo;") > -1)
    {
      tempString = "AtmoOrbLEDGeo:";
      tempString += LED_GEO;
      tempString += ";";
      broadcast(tempString);
    }
    Serial.println(tempString);
  }
  if ((ip == "" || ip == "0.0.0.0") && setupDone && (millis() - previousCheckIPMillis > checkIPInterval))
  {
    Serial1.println("AT+CIFSR");
    previousCheckIPMillis = millis();
  }
  if (smoothStep < SMOOTH_STEPS && millis() >= (smoothMillis + (SMOOTH_DELAY * (smoothStep + 1))))
  {
    smoothColors();
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

void setSmoothColors(int rgb[NUM_LEDS][3])
{
  for (int i = 0; i < NUM_LEDS; i++)
  {
    prevColor[i][0] = nextColor[i][0];
    prevColor[i][1] = nextColor[i][1];
    prevColor[i][2] = nextColor[i][2];
    
    nextColor[i][0] = rgb[i][0];
    nextColor[i][1] = rgb[i][1];
    nextColor[i][2] = rgb[i][2];
  }
  smoothStep = 0;
  smoothMillis = millis();
}

void smoothColors()
{ 
  smoothStep++;
  for (int i = 0; i < NUM_LEDS; i++)
  {    
    leds[i] = CRGB(prevColor[i][0] + (int)(((float)(nextColor[i][0] - prevColor[i][0]) / SMOOTH_STEPS) * smoothStep), prevColor[i][1] + (int)(((float)(nextColor[i][1] - prevColor[i][1]) / SMOOTH_STEPS) * smoothStep), prevColor[i][2] + (int)(((float)(nextColor[i][2] - prevColor[i][2]) / SMOOTH_STEPS) * smoothStep));
    
    if (smoothStep >= SMOOTH_STEPS)
    {
      prevColor[i][0] = nextColor[i][0];
      prevColor[i][1] = nextColor[i][1];
      prevColor[i][2] = nextColor[i][2];
    }
  }
  FastLED.show();
}
