#include <Adafruit_CC3000.h>
#include <SPI.h>
#include <Adafruit_NeoPixel.h>


// Wi-Fi
#define ADAFRUIT_CC3000_IRQ   3
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT,
                                         SPI_CLOCK_DIVIDER);
                                         
#define WLAN_SSID "Your SSID"
#define WLAN_PASS "Your WiFi Password"

#define WLAN_SECURITY   WLAN_SEC_WPA2

#define LISTEN_PORT           1879

Adafruit_CC3000_Server lightServer(LISTEN_PORT);

#define BUFFER_SIZE  100
#define TIMEOUT_MS   500
char message[100];
uint8_t buffer[BUFFER_SIZE+1];
int bufindex = 0;

// LEDS
#define PIN 6
#define NUM_LEDS 24
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, PIN, NEO_GRB + NEO_KHZ800);

void setup(void)
{  
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  
  Serial.begin(115200);
  while (!Serial);
  Serial.println(F("Hello, CC3000!\n")); 
  /* Initialise the module */
  Serial.println(F("\nInit..."));
  
  if (!cc3000.begin())
  {
    //Error
  }
  
  Serial.print(F("\nConnecting to ")); Serial.println(WLAN_SSID);
  delay(1000);
  if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
    Serial.println(F("Failed!"));
    while(1);
  }
   
  Serial.println(F("Connected!"));
  
  while (!cc3000.checkDHCP())
  {
    delay(100);
  }
  
  lightServer.begin();
  
  Serial.println(F("Listening..."));
}

void loop(void)
{
  Adafruit_CC3000_ClientRef client = lightServer.available();
  if (client) {
     if (client.available() > 0) {
      
     int x = 0;
     bufindex = 0;
     memset(&buffer, 0, sizeof(buffer));
     memset(&message, 0, sizeof(message));
     unsigned long endtime = millis() + TIMEOUT_MS;

     while ((millis() < endtime) && (bufindex < BUFFER_SIZE)) {
       if (client.available()) {
         buffer[bufindex++] = client.read();
       }
     }

     String message = (char*)buffer;
     message.trim();
     
     if(message.length() > 0)
     {
       if (message.indexOf("setcolors:") > -1)
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
            
             int red = hexToDec(message.substring(startPos, startPos + 2));
             int green = hexToDec(message.substring(startPos + 2, startPos + 4));
             int blue = hexToDec(message.substring(startPos + 4, startPos + 6));
             
             uint32_t color = strip.Color(red, green, blue);
             uint16_t i;
            
             for(i=0; i< NUM_LEDS; i++) {
              strip.setPixelColor(i, color);
             }
           }
           startPos = endPos + 1;
         }
         if (success)
         {
           strip.show();
         }
       }
       Serial.println(message);
     }
   }
  }
}

int hexToDec(String hex)
{
  char hexChar[hex.length()];
  hex.toCharArray(hexChar, hex.length() + 1);
  char* hexPos = hexChar;
  return strtol(hexPos, &hexPos, 16);
}
