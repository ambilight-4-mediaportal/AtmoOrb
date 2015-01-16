// This #include statement was automatically added by the Spark IDE.
#include "neopixel/neopixel.h"

// This #include statement was automatically added by the Spark IDE.
#include "mdns/mdns.h"

// TCP client
int port = 49692;
TCPServer server = TCPServer(port);

// TCP buffers
#define BUFFER_SIZE  100
#define TIMEOUT_MS   500
uint8_t buffer[BUFFER_SIZE + 1];
int bufindex = 0;

// mDNS
MDNSResponder mdns;
bool isMDNS;
char* hostname = "ORB001";

// LEDS
#define PIXEL_PIN D2
#define PIXEL_COUNT 24
#define PIXEL_TYPE WS2812B
Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);

// Orb LED handling, smoothing etc..
#define SMOOTH_STEPS 50 // Steps to take for smoothing colors
#define SMOOTH_DELAY 4 // Delay between smoothing steps
#define SMOOTH_BLOCK 0 // Block incoming colors while smoothing

byte nextColor[3];
byte prevColor[3];
byte currentColor[3];
byte smoothStep = SMOOTH_STEPS;
unsigned long smoothMillis;

// White adjustment
#define RED_CORRECTION 210
#define GREEN_CORRECTION 255
#define BLUE_CORRECTION 180

void setup() {
    
    IPAddress addr = WiFi.localIP();
    int32_t ip = (addr[0] * 16777216) + (addr[1] * 65536) + (addr[2] * 256) + (addr[3]);
    
    // Setup MDNS
    isMDNS = mdns.begin(hostname, ip);
    
    Serial.begin(115200);
    server.begin();
}

void loop() {
    
// Update mDNS record if it has started successfully
if(isMDNS)
{ 
    mdns.update();
}

// Cloud connection status
bool cloudy = true;

TCPClient client = server.available();

    if(client)
    {
      //Serial.println("Connected clients:" + (String)client.connected());
      //Serial.println("Connected avail:" + (String)client.available());
      while(client.connected())
      {
          // Disconnect from cloud
          Spark.disconnect();
          cloudy = false;
          
          bufindex = 0;
          memset(&buffer, 0, sizeof(buffer));
          unsigned long endtime = millis() + TIMEOUT_MS;
    
          while ((millis() < endtime) && (bufindex < BUFFER_SIZE)) {
            if (client.available()) {
              buffer[bufindex++] = client.read();
            }
          }
          
          String message = (char*)buffer;
          if (message.length() > 0)
          {
            if (message.indexOf(F("setcolor:")) > -1)
            {
              Serial.println(message);
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
          }
          
          if (smoothStep < SMOOTH_STEPS && millis() >= (smoothMillis + (SMOOTH_DELAY * (smoothStep + 1))))
          {
            smoothColor();
          }
          
          client.flush();
      }
              
      // Reconnect to cloud if needed and flush client
      if(!cloudy)
      {
        Spark.connect();
      }
    
      client.flush();  //for safety
      delay(400);
      client.stop();
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


// Set a new color to smooth to
void setSmoothColor(byte red, byte green, byte blue)
{
    if (smoothStep == SMOOTH_STEPS || SMOOTH_BLOCK == 0)
    {
        red = (red * RED_CORRECTION) / 255;
        green = (green * GREEN_CORRECTION) / 255;
        blue = (blue * BLUE_CORRECTION) / 255;
        
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
    }
}
// Display one step to the next color
void smoothColor()
{
    smoothStep++;
    currentColor[0] = prevColor[0] + (((nextColor[0] - prevColor[0]) * smoothStep) / SMOOTH_STEPS);
    currentColor[1] = prevColor[1] + (((nextColor[1] - prevColor[1]) * smoothStep) / SMOOTH_STEPS);
    currentColor[2] = prevColor[2] + (((nextColor[2] - prevColor[2]) * smoothStep) / SMOOTH_STEPS);
    
    for (byte i = 0; i < PIXEL_COUNT; i++)
    {
        strip.setPixelColor(i, currentColor[0], currentColor[1], currentColor[2]);
    }
    
    strip.show();
}