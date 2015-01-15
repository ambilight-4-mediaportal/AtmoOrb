#include "application.h"
#include "neopixel__spark_internet_button/neopixel__spark_internet_button.h"

// IMPORTANT: Set pixel COUNT, PIN and TYPE
#define PIXEL_PIN A7
#define PIXEL_COUNT 24
#define PIXEL_TYPE WS2812B

#define SMOOTH_STEPS 10 // Steps to take for smoothing colors
#define SMOOTH_DELAY 5 // Delay between smoothing steps

byte nextColor[3];
byte prevColor[3];
byte smoothStep = SMOOTH_STEPS;
unsigned long smoothMillis;


// Define AtmoOrb ID
#define ID "1"

// Define AtmoOrb PORT
unsigned int PORT = 49692;

char recbuf[500];

char myIP[24];
char broadcastIP[24];
IPAddress myIp;

// An UDP instance to let us send and receive packets over UDP
UDP Udp; 

// Define LEDS
Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);

void setup() {
    
    strip.begin();
    strip.show(); // Initialize all pixels to 'off'
    
    // Initialise UDP
    Udp.begin(PORT);
    Serial.begin(9600);
    Serial.println(WiFi.localIP()); 
    
    myIp = WiFi.localIP();
    
    //Set current IP
    sprintf(myIP, "%d.%d.%d.%d", myIp[0], myIp[1], myIp[2], myIp[3]);
    
    //Set broadcast IP
    sprintf(broadcastIP, "%d.%d.%d.%d", myIp[0], myIp[1], myIp[2], 255);
}

void loop() {
    
    //Display rainbox led for testing LEDs
    rainbow(20);
    
    int32_t packetLen = Udp.parsePacket();
     
    while (packetLen > 0) {
        Udp.read(recbuf, packetLen);
        packetLen = Udp.parsePacket();
    }
    
    String message = String(recbuf);
    
    if (message.length() > 0)
    {
        // Debug incoming messages
        Serial.println(message); 
        //Serial.println(myIP);
        //Serial.println(broadcastIP);
        
        if (message.indexOf(F("setcolor:")) > -1)
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
            for (byte i = 0; i < PIXEL_COUNT; i++)
            {
              //setSmoothColor(red, green, blue);
              strip.setPixelColor(i, red, green, blue);
            }
            strip.show();
            //setSmoothColor(red, green, blue);
          }
        }
    }
    
    //Clear receive buffer
    memset(recbuf, 0, sizeof(recbuf));

    // for now just always broadcast
    ipReceived();
}

// Broadcast ip
void ipReceived()
{
    String tempString = F("AtmoOrb:");
    tempString += ID;
    tempString += F(":address:");
    tempString += myIP;
    tempString += F(",");
    tempString += PORT;
    tempString += F(";");
    
    broadcast(tempString);
}

void broadcast(String message)
{
    int beginReady = Udp.beginPacket(IPAddress(myIp[0],myIp[1],myIp[2],255), PORT);  //works-change 255 to 10 to fail
    
    unsigned char ReplyBuffer[500];
    message.toCharArray((char*)ReplyBuffer, 500);
                
    if (beginReady!=0) {
        //Serial.print(F("IP: "));
        //Serial.println(myIP);
        
        Udp.write(ReplyBuffer, 500);
        
        Udp.endPacket();
    }
}

void rainbow(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i+j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Convert a hex string into decimal
byte hexToDec(String hex)
{
    char hexChar[hex.length()];
    hex.toCharArray(hexChar, hex.length() + 1);
    char* hexPos = hexChar;
    return strtol(hexPos, &hexPos, 16);
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
  
  for (byte i = 0; i < PIXEL_COUNT; i++)
  {
    strip.setPixelColor(i, red, green, blue);
  }
  strip.show();
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
   return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170;
   return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}