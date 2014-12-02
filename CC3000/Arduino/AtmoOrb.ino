#include <Adafruit_CC3000.h>
#include <SPI.h>
#include <Adafruit_NeoPixel.h>

#define ADAFRUIT_CC3000_IRQ   3
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT,
                                         SPI_CLOCK_DIVIDER);

#define WLAN_SSID       "Your SSID"
#define WLAN_PASS       "Your WiFi Password"

#define WLAN_SECURITY   WLAN_SEC_WPA2

#define LISTEN_PORT           1879

Adafruit_CC3000_Server lightServer(LISTEN_PORT);

#define PIN 6
#define NUM_PIXELS 24
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_PIXELS, PIN, NEO_GRB + NEO_KHZ800);

void setup(void)
{  
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  
  Serial.begin(115200);
  Serial.println(F("Hello, CC3000!\n")); 
  
  /* Initialise the module */
  Serial.println(F("\nInitializing..."));
  if (!cc3000.begin())
  {
    Serial.println(F("Couldn't begin()! Check your wiring?"));
    while(1);
  }
  
  Serial.print(F("\nAttempting to connect to ")); Serial.println(WLAN_SSID);
  if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
    Serial.println(F("Failed!"));
    while(1);
  }
   
  Serial.println(F("Connected!"));
  
  Serial.println(F("Request DHCP"));
  while (!cc3000.checkDHCP())
  {
    delay(100);
  }
  
  lightServer.begin();
  
  Serial.println(F("Listening for connections..."));
}

void loop(void)
{
  Adafruit_CC3000_ClientRef client = lightServer.available();
  if (client) {
     if (client.available() > 0) {
      
       int x = 0;
       
       uint8_t ch = client.read();
       
       int len = sizeof(ch);
       String message = String(ch).substring(0,len-1);
       Serial.println(F("Message:\r\n"));
       Serial.println(message);
       
       int red = -1;
       int green = -1;
       int blue = -1;
       
       if (message.indexOf("setcolor:") > -1)
       {         
         int red = -1;
         int green = -1;
         int blue = -1;
         
         while (x < message.length())
         {
           int start = message.indexOf("setcolor:", x);
           int endValue1 = message.indexOf(',', start + 9);
           int endValue2 = message.indexOf(',', endValue1 + 1);
           int endValue3 = message.indexOf(';', endValue2 + 1);
           
           if (start == -1 || endValue1 == -1 || endValue2 == -1 || endValue3 == -1)
           {
             break;
           }
          
           x = endValue3;
           red = message.substring(start + 9, endValue1).toInt();
           green = message.substring(endValue1 + 1, endValue2).toInt();
           blue = message.substring(endValue2 + 1, endValue3).toInt();
         }
         
         if (red != -1 && green != -1 && blue != -1)
         {
           
           Serial.print("Red: "+ red);
           Serial.print("Green: "+ green);
           Serial.print("Blue: "+ blue);
            
           uint32_t color = strip.Color(red, green, blue);
           uint16_t i;
           for(i=0; i< strip.numPixels(); i++) {
             strip.setPixelColor(i, color);
           }
           strip.show();
       }
     }
    }
  }
}
