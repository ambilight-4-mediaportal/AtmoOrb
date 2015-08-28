// This #include statement was automatically added by the Particle IDE.
#include "neopixel/neopixel.h"

// telnet defaults to port 23
TCPServer server = TCPServer(49692);
TCPClient client;

// Cloud status
bool cloudy = true;

// LEDS
#define PIXEL_PIN D6
#define PIXEL_COUNT 24
#define PIXEL_TYPE WS2812B
Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);

// TCP buffers
#define BUFFER_SIZE  3 + 3 * PIXEL_COUNT
#define TIMEOUT_MS   500
uint8_t buffer[BUFFER_SIZE];

// Smoothing
#define SMOOTH_STEPS 50 // Steps to take for smoothing colors
#define SMOOTH_DELAY 4 // Delay between smoothing steps
#define SMOOTH_BLOCK 0 // Block incoming colors while smoothing

byte nextColor[3];
byte prevColor[3];
byte currentColor[3];
byte smoothStep = SMOOTH_STEPS;
unsigned long smoothMillis;
unsigned int forceOff;

// White adjustment

#define RED_CORRECTION 210
#define GREEN_CORRECTION 255
#define BLUE_CORRECTION 180

void setup()
{
  // start listening for clients
  server.begin();
  
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

 
  // Make sure your Serial Terminal app is closed before powering your device
  /*
  
  // Now open your Serial Terminal, and hit any key to continue!
  //while(!Serial.available()) SPARK_WLAN_Loop();
  
  Serial.begin(115200);
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.subnetMask());
  Serial.println(WiFi.gatewayIP());
  Serial.println(WiFi.SSID());*/
}

void loop()
{
    if (client.connected()) {
        
        if(cloudy)
        {
            Spark.disconnect();
            cloudy = false;
        }
        
        while (client.available()) {
            unsigned int i = 0;
            unsigned long endtime = millis() + TIMEOUT_MS;

            while ((millis() < endtime) && (i < BUFFER_SIZE)) {
                if (client.available()) {
                    buffer[i++] = client.read();
                }
            }

            if(i == BUFFER_SIZE){
                i = 0;

                // Look for 0xC0FFEE
                if(buffer[i++] == 0xC0 && buffer[i++] == 0xFF && buffer[i++] == 0xEE){
                    
                    //unsigned int pixels = buffer[i++];
                    forceOff = buffer[i++];
                    byte red =  buffer[i++];
                    byte green =  buffer[i++];
                    byte blue =  buffer[i++];
                    
                    setSmoothColor(red, green, blue);
                }
            }
        }
             
        if(forceOff > 0)
        {
            forceLedsOFF();
        }
        else if (smoothStep < SMOOTH_STEPS && millis() >= (smoothMillis + (SMOOTH_DELAY * (smoothStep + 1))))
        {
            smoothColor();
        }
        
    } 
    else {
        
        if(!cloudy)
        {
            Spark.connect();
            cloudy = true;
        }
        
        // if no client is yet connected, check for a new connection
        client = server.available();
    }
}

// Force all leds OFF
void forceLedsOFF()
{
    for (byte i = 0; i < PIXEL_COUNT; i++)
    {
        strip.setPixelColor(i, 0, 0, 0);
    }
    
    strip.show();
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