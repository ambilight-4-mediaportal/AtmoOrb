// This #include statement was automatically added by the Particle IDE.
#include "neopixel/neopixel.h"

// UDP SETTINGS
#define SERVER_PORT 49692
#define HOSTNAME "ORB002"

UDP client;

// LED SETTINGS
#define PIXEL_PIN D6
#define PIXEL_COUNT 24
#define PIXEL_TYPE WS2812B
Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);

// UDP buffers
#define BUFFER_SIZE  3 + 3 * PIXEL_COUNT
#define TIMEOUT_MS   500
uint8_t buffer[BUFFER_SIZE];

// SMOOTHING SETTINGS
#define SMOOTH_STEPS 50 // Steps to take for smoothing colors
#define SMOOTH_DELAY 4 // Delay between smoothing steps
#define SMOOTH_BLOCK 0 // Block incoming colors while smoothing

byte nextColor[3];
byte prevColor[3];
byte currentColor[3];
byte smoothStep = SMOOTH_STEPS;
unsigned long smoothMillis;
unsigned int forceOff;

// WHITE ADJUSTMENT
#define RED_CORRECTION 255
#define GREEN_CORRECTION 255
#define BLUE_CORRECTION 255

void setup()
{
    // Init UDP
    client.begin(SERVER_PORT);
    
    // Join Orb multicast group
    client.joinMulticast(IPAddress(224, 15, 18, 2));
    
    // Init leds
    strip.begin();
    strip.show(); // Initialize all pixels to 'off'
}

void loop(){
    
    int packetSize = client.parsePacket();
    
    if(packetSize == BUFFER_SIZE){
        client.read(buffer, BUFFER_SIZE);
        unsigned int i = 0;
        
        // Look for 0xC0FFEE
        if(buffer[i++] == 0xC0 && buffer[i++] == 0xFF && buffer[i++] == 0xEE){
            
            //unsigned int pixels = buffer[i++];
            forceOff = buffer[i++];
            byte red =  buffer[i++];
            byte green =  buffer[i++];
            byte blue =  buffer[i++];
            setSmoothColor(red, green, blue);
        }
        
    }else if(packetSize > 0){
        //Serial.println("Got malformed packet");
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

// Set color manually
void setColor(byte red, byte green, byte blue)
{
    for (byte i = 0; i < PIXEL_COUNT; i++)
    {
        strip.setPixelColor(i, red, green, blue);
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
    
    int i = 0;
    setColor(currentColor[i++], currentColor[i++], currentColor[i++]);
    strip.show();
}

// Force all leds OFF
void forceLedsOFF()
{
    setColor(0,0,0);
    clearSmoothColors();
}

// Clear smooth color byte arrays
void clearSmoothColors()
{
    memset(prevColor, 0, sizeof(prevColor));
    memset(currentColor, 0, sizeof(nextColor));
    memset(nextColor, 0, sizeof(nextColor));
}
