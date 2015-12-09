#include "FastLED/FastLED.h"
FASTLED_USING_NAMESPACE;

#if FASTLED_VERSION < 3001000
#error "Requires FastLED 3.1 or later; check github for latest code."
#endif

// UDP SETTINGS
#define SERVER_PORT 49692
#define DISCOVERY_PORT 49692
UDP client;
IPAddress multicastIP(239, 15, 18, 2);

// ORB SETTINGS
unsigned int orbID = 1;
bool useSmoothColor  = false;

// LED settings
#define DATA_PIN    6
#define NUM_LEDS    24
CRGB leds[NUM_LEDS];

// UDP BUFFERS
#define BUFFER_SIZE  5 + 3 * NUM_LEDS
#define BUFFER_SIZE_DISCOVERY 5
#define TIMEOUT_MS   500
uint8_t buffer[BUFFER_SIZE];
uint8_t bufferDiscovery[BUFFER_SIZE_DISCOVERY];

// SMOOTHING SETTINGS
#define SMOOTH_STEPS 50 // Steps to take for smoothing colors
#define SMOOTH_DELAY 4 // Delay between smoothing steps
#define SMOOTH_BLOCK 0 // Block incoming colors while smoothing

byte nextColor[3];
byte prevColor[3];
byte currentColor[3];
byte smoothStep = SMOOTH_STEPS;
unsigned long smoothMillis;

// WHITE ADJUSTMENT
#define RED_CORRECTION 255
#define GREEN_CORRECTION 255
#define BLUE_CORRECTION 255

void setup()
{
    //  Client
    client.begin(SERVER_PORT);
    client.setBuffer(BUFFER_SIZE);
    
    // Multicast group
    client.joinMulticast(multicastIP);
    
    // Leds
    FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
}

void loop(){
    int packetSize = client.parsePacket();
    
    if(packetSize == BUFFER_SIZE){
        client.read(buffer, BUFFER_SIZE);
        unsigned int i = 0;
        
        // Look for 0xC0FFEE
        if(buffer[i++] == 0xC0 && buffer[i++] == 0xFF && buffer[i++] == 0xEE)
        {
            byte commandOptions = buffer[i++];
            byte rcvOrbID = buffer[i++];
            
            // Command option: 1 = force off | 2 = use lamp smoothing and validate by Orb ID | 4 = validate by Orb ID | 8 = discovery
            if(commandOptions == 1)
            {
                // Orb ID 0 = turn off all lights
                // Otherwise turn off selectively
                if(rcvOrbID == 0)
                {
                    setColor(0, 0, 0);
                    //setSmoothColor(0, 0, 0);
                    //forceLedsOFF();
                }
                else if(rcvOrbID == orbID)
                {
                    setColor(0, 0, 0);
                    //setSmoothColor(0, 0, 0);
                    //forceLedsOFF();
                }
                return;
            }
            else if(commandOptions == 2)
            {
                if(rcvOrbID != orbID)
                {
                    return;
                }
                
                useSmoothColor = true;
            }
            else if(commandOptions == 4)
            {
                if(rcvOrbID != orbID)
                {
                    return;
                }
                
                useSmoothColor = false;
            }
            else if(commandOptions == 8)
            {
                // Respond to remote IP address with Orb ID
                IPAddress remoteIP = client.remoteIP();
                bufferDiscovery[0] = orbID;
                
                client.sendPacket(bufferDiscovery, BUFFER_SIZE_DISCOVERY, remoteIP, DISCOVERY_PORT);
                
                // Clear buffer
                memset(bufferDiscovery, 0, sizeof(bufferDiscovery));
                return;
            }

            byte red =  buffer[i++];
            byte green =  buffer[i++];
            byte blue =  buffer[i++];
        
            if(useSmoothColor)
            {
                setSmoothColor(red, green, blue);
            }
            else
            {
                // Apply color corrections
                red = (red * RED_CORRECTION) / 255;
                green = (green * GREEN_CORRECTION) / 255;
                blue = (blue * BLUE_CORRECTION) / 255;
            
                setColor(red, green, blue);
            }
        }
        
    }else if(packetSize > 0){
        // Got malformed packet
    }
    
    if (useSmoothColor)
    {
        if (smoothStep < SMOOTH_STEPS && millis() >= (smoothMillis + (SMOOTH_DELAY * (smoothStep + 1))))
        { 
            smoothColor();
        }
    }
}

// Set color
void setColor(byte red, byte green, byte blue)
{
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
    
    setColor(currentColor[0], currentColor[1], currentColor[2]);
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
