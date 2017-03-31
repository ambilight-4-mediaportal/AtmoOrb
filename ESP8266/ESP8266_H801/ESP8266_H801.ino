// AtmoOrb H801 by Wolph, Lightning303 & Rick164
//
// H801 (ESP8266 based) Version
//
//
// You may change the settings that are commented

#include <ESP8266WiFi.h>
#include <WiFiUDP.h>

#define NUM_LEDS 24 // Number of leds
#define DATA_PIN 7 // Data pin for leds (the default pin 7 might correspond to pin 13 on some boards)
#define SERIAL_DEBUG 1 // Serial debugging (0=Off, 1=On)

#define ID 1 // Id of this lamp

// Smoothing
#define SMOOTH_STEPS 20 // Steps to take for smoothing colors
#define SMOOTH_DELAY 10 // Delay between smoothing steps
#define SMOOTH_BLOCK 0 // Block incoming colors while smoothing

// Startup color
#define STARTUP_RED 255 // Color shown directly after power on
#define STARTUP_GREEN 175 // Color shown directly after power on
#define STARTUP_BLUE 100 // Color shown directly after power on

// White adjustment
#define RED_CORRECTION 220 // Color Correction
#define GREEN_CORRECTION 255 // Color Correction
#define BLUE_CORRECTION 180 // Color Correction

// LED pins
#define PIN_RED 15
#define PIN_GREEN 13
#define PIN_BLUE 12
#define PIN_WHITE1 14
#define PIN_WHITE2 4
#define PIN_STATUS_GREEN 1
#define PIN_STATUS_RED 5

#define ON LOW
#define OFF HIGH

// Network settings
const char* ssid = "ssid"; // WiFi SSID
const char* password = "password"; // WiFi password
const IPAddress multicastIP(239, 15, 18, 2); // Multicast IP address
const int multicastPort = 49692; // Multicast port number

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

void wifiConnect() {
  digitalWrite(PIN_STATUS_GREEN, OFF);
  digitalWrite(PIN_STATUS_RED, OFF);

  int i = 0;
  Serial1.print(F("Connecting to "));
  Serial1.println(ssid);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(250);
    if (i++ % 2) {
      /* Enable status led */
      digitalWrite(PIN_STATUS_RED, ON);

      /* Write to the console if needed */
#if SERIAL_DEBUG == 1
      Serial1.print(F("."));
#endif
    } else {
      /* Disable status led */
      digitalWrite(PIN_STATUS_RED, OFF);
    }
  }

  /* Enable the green led when connected to Wifi */
  digitalWrite(PIN_STATUS_RED, OFF);
  digitalWrite(PIN_STATUS_GREEN, ON);

#if SERIAL_DEBUG == 1
  Serial1.println("");
  Serial1.print(F("Connected to "));
  Serial1.println(ssid);
  Serial1.print(F("IP address: "));
  Serial1.println(WiFi.localIP());
#endif
}

void setup()
{
  pinMode(PIN_RED, OUTPUT);
  pinMode(PIN_GREEN, OUTPUT);
  pinMode(PIN_BLUE, OUTPUT);
  pinMode(PIN_WHITE1, OUTPUT);
  pinMode(PIN_WHITE2, OUTPUT);
  pinMode(PIN_STATUS_GREEN, OUTPUT);
  pinMode(PIN_STATUS_RED, OUTPUT);

#if SERIAL_DEBUG == 1
  Serial1.begin(115200);
#endif

  WiFi.begin(ssid, password);
  wifiConnect();

  Udp.beginMulticast(WiFi.localIP(), multicastIP, multicastPort);
}

void loop()
{
  if (WiFi.status() != WL_CONNECTED)
  {
#if SERIAL_DEBUG == 1
    Serial1.print(F("Lost connection to "));
    Serial1.print(ssid);
    Serial1.println(F("."));
    Serial1.println(F("Trying to reconnect."));
#endif

    wifiConnect();
  }

  if (Udp.parsePacket())
  {
    digitalWrite(PIN_STATUS_RED, ON);
    
    byte len = Udp.available();
    byte rcvd[len];
    Udp.read(rcvd, len);

#if SERIAL_DEBUG == 1
    Serial1.print(F("UDP Packet from "));
    Serial1.print(Udp.remoteIP());
    Serial1.print(F(" to "));
    Serial1.println(Udp.destinationIP());
    for (byte i = 0; i < len; i++)
    {
      Serial1.print(rcvd[i]);
      Serial1.print(F(" "));
    }
    Serial1.println("");
#endif
    if (len >= 8 && rcvd[0] == 0xC0 && rcvd[1] == 0xFF && rcvd[2] == 0xEE && (rcvd[4] == ID || rcvd[4] == 0))
    {
      switch (rcvd[3])
      {
        case 1:
          setColor(0, 0, 0);
          smoothStep = SMOOTH_STEPS;
          break;
        case 2:
        default:
          setSmoothColor(rcvd[5], rcvd[6], rcvd[7]);
          break;
        case 4:
          setColor(rcvd[5], rcvd[6], rcvd[7]);
          smoothStep = SMOOTH_STEPS;
          break;
        case 8:
          Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
          Udp.write(ID);
          Udp.endPacket();
          break;
      }
    }
    digitalWrite(PIN_STATUS_RED, OFF);
  }
  if (smoothStep < SMOOTH_STEPS && millis() >= (smoothMillis + (SMOOTH_DELAY * (smoothStep + 1))))
  {
    smoothColor();
  }
}

// Display color on leds
void setColor(byte red, byte green, byte blue)
{
  // Is the new color already active?
  if (currentColor[0] == red && currentColor[1] == green && currentColor[2] == blue)
  {
    return;
  }
  currentColor[0] = red;
  currentColor[1] = green;
  currentColor[2] = blue;

  analogWrite(PIN_RED, red * 4);
  analogWrite(PIN_GREEN, green * 4);
  analogWrite(PIN_BLUE, blue * 4);
}

// Set a new color to smooth to
void setSmoothColor(byte red, byte green, byte blue)
{
  if (smoothStep == SMOOTH_STEPS || SMOOTH_BLOCK == 0)
  {
    // Is the new color the same as the one we already are smoothing towards?
    // If so dont do anything.
    if (nextColor[0] == red && nextColor[1] == green && nextColor[2] == blue)
    {
      return;
    }
    // Is the new color the same as we have right now?
    // If so stop smoothing and keep the current color.
    else if (currentColor[0] == red && currentColor[1] == green && currentColor[2] == blue)
    {
      smoothStep = SMOOTH_STEPS;
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
