// AtmoOrb by Lightning303
//
// ESP8266 Standalone Version

#include <ESP8266WiFi.h>
#include <WiFiUDP.h>
#include <FastLED.h>
#include <RCSwitch.h>

#define NUM_LEDS 27 // Number of leds
#define DATA_PIN 13 // Data pin for leds
#define SERIAL_DEBUG 0

#define ID 1 // Id of this lamp

// Smoothing
#define SMOOTH_STEPS 50 // Steps to take for smoothing colors
#define SMOOTH_DELAY 4 // Delay between smoothing steps
#define SMOOTH_BLOCK 0 // Block incoming colors while smoothing

// Startup color
#define STARTUP_RED 255
#define STARTUP_GREEN 175
#define STARTUP_BLUE 100

// White adjustment
#define RED_CORRECTION 220
#define GREEN_CORRECTION 255
#define BLUE_CORRECTION 180

// RC Switch
#define RC_SWITCH 0
#if RC_SWITCH == 1
  #define RC_SLEEP_DELAY 900000 // Delay until RF transmitter send signals
  #define RC_CODE_0 10001
  #define RC_CODE_1 00010
  RCSwitch mySwitch = RCSwitch();
  boolean remoteControlled = false;
#endif

// Network settings
const char* ssid = "...";
const char* password = "...";
const IPAddress multicastIP(239, 15, 18, 2);
const int multicastPort = 49692;

CRGB leds[NUM_LEDS];
WiFiUDP Udp;
byte nextColor[3];
byte prevColor[3];
byte currentColor[3];
byte smoothStep = SMOOTH_STEPS;
unsigned long smoothMillis;

void setColor(byte red, byte green, byte blue);
void setSmoothColor(byte red, byte green, byte blue);
void smoothColor();
void clearSmoothColors();

void setup()
{
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  //FastLED.setCorrection(TypicalSMD5050);
  FastLED.setCorrection(CRGB(RED_CORRECTION, GREEN_CORRECTION, BLUE_CORRECTION));
  
  for (byte x = 0; x < NUM_LEDS; x++)
  {
    leds[x] = CRGB(STARTUP_RED, STARTUP_GREEN, STARTUP_BLUE);
  }
  FastLED.show();
  
  #if RC_SWITCH == 1
    mySwitch.enableTransmit(8);
  #endif

  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    #if SERIAL_DEBUG == 1
      Serial.print(F("."));
    #endif
  }
  #if SERIAL_DEBUG == 1
    Serial.println("");
    Serial.print(F("Connected to "));
    Serial.println(ssid);
    Serial.print(F("IP address: "));
    Serial.println(WiFi.localIP());
  #endif
  Udp.beginMulticast(WiFi.localIP(), multicastIP, multicastPort);
}

void loop()
{
  if (Udp.parsePacket())
  {
    byte len = Udp.available();
    byte rcvd[len];
    Udp.read(rcvd, len);
    
    #if SERIAL_DEBUG == 1
      Serial.print(F("UDP Packet from "));
      Serial.print(Udp.remoteIP());
      Serial.print(F(" to "));
      Serial.println(Udp.destinationIP());
      for (byte i = 0; i < len; i++)
      {
        Serial.print(rcvd[i]);
        Serial.print(F(" "));
      }
      Serial.println("");
    #endif
    if (len >= 8 && rcvd[0] == 0xC0 && rcvd[1] == 0xFF && rcvd[2] == 0xEE && (rcvd[4] == ID || rcvd[4] == 0))
    {
      switch (rcvd[3])
      {
        case 1:
          FastLED.clear();
          FastLED.show();
          clearSmoothColors();
          break;
        case 2:
        default:
          setSmoothColor(rcvd[5], rcvd[6], rcvd[7]);
          break;
        case 4:
          setColor(rcvd[5], rcvd[6], rcvd[7]);
          break;
        case 8:
          Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
          Udp.write(ID);
          Udp.endPacket();
          break;
      }
    }
  }
  if (smoothStep < SMOOTH_STEPS && millis() >= (smoothMillis + (SMOOTH_DELAY * (smoothStep + 1))))
  {
    smoothColor();
  }
  #if RC_SWITCH == 1
    if (remoteControlled && currentColor[0] == 0 && currentColor[1] == 0 && currentColor[2] == 0 && millis() >= smoothMillis + RC_SLEEP_DELAY)
    {
      // Send this signal only once every seconds
      smoothMillis += 1000;
      mySwitch.switchOff(RC_CODE_0, RC_CODE_1);
    }
  #endif
}

// Display color on leds
void setColor(byte red, byte green, byte blue)
{
  currentColor[0] = red;
  currentColor[1] = green;
  currentColor[2] = blue;
  
  for (byte i = 0; i < NUM_LEDS; i++)
  {
    leds[i] = CRGB(red, green, blue);
  }
  FastLED.show();
}

// Set a new color to smooth to
void setSmoothColor(byte red, byte green, byte blue)
{
  if (smoothStep == SMOOTH_STEPS || SMOOTH_BLOCK == 0)
  {
    if (nextColor[0] == red && nextColor[1] == green && nextColor[2] == blue)
    {
      return;
    }
    
    prevColor[0] = currentColor[0];
    prevColor[1] = currentColor[1];
    prevColor[2] = currentColor[2];
    
    nextColor[0] = red;
    nextColor[1] = green;
    nextColor[2] = blue;
    
    smoothMillis = millis();
    smoothStep = 0;

    #if RC_SWITCH == 1
      if (!remoteControlled)
      {
        remoteControlled = true;
      }
    #endif
  }
}

// Display one step to the next color
void smoothColor()
{ 
  smoothStep++;
  
  byte red = prevColor[0] + (((nextColor[0] - prevColor[0]) * smoothStep) / SMOOTH_STEPS);
  byte green = prevColor[1] + (((nextColor[1] - prevColor[1]) * smoothStep) / SMOOTH_STEPS);
  byte blue = prevColor[2] + (((nextColor[2] - prevColor[2]) * smoothStep) / SMOOTH_STEPS);   

  setColor(red, green, blue);
}

// Clear smooth color byte arrays
void clearSmoothColors()
{
  memset(prevColor, 0, sizeof(prevColor));
  memset(currentColor, 0, sizeof(nextColor));
  memset(nextColor, 0, sizeof(nextColor));
}
