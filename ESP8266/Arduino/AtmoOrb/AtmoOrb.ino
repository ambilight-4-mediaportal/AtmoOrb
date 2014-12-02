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


#define wifiSSID "Your SSID"
#define wifiPassword "Your WiFi Password"
#define disableDHCP 1
#define staticIP "192.168.1.42"
#define serverPort 30003
#define broadcastIP "192.168.1.255"
#define broadcastPort 30003
#define ledPin 15

char serialBuffer[1000];
String ip;
boolean setupDone = false;
long previousCheckIPMillis = 0; 
long checkIPInterval = 5000;


void setup()
{
  String setupMessage = "";
  pinMode(9, OUTPUT);
  
  Serial.begin(115200);
  Serial.setTimeout(5);
  Serial1.begin(115200);
  Serial1.setTimeout(5);
  
  delay(1000);
  Serial1.println("AT+RST");
  Serial1.println("AT+CWMODE=3");
  delay(100);
  if (disableDHCP == 1)
  {
    Serial1.println("AT+CWDHCP=2,1");
  }
  delay(100);
  setupMessage = "AT+CWJAP=\"";
  setupMessage += wifiSSID;
  setupMessage += "\",\"";
  setupMessage += wifiPassword;
  setupMessage += "\"";
  Serial1.println(setupMessage);
  delay(5000);
  if (disableDHCP == 1)
  {
    setupMessage = "AT+CIPSTA=\"";
    setupMessage += staticIP;
    setupMessage += "\"";
    Serial1.println(setupMessage);
  }
  delay(100);
  Serial1.println("AT+CIPMUX=1");
  delay(100);
  setupMessage = "AT+CIPSERVER=1,";
  setupMessage += serverPort;
  Serial1.println(setupMessage);
  setupDone = true;
}

void loop()
{
  if (Serial.available() > 0)
  {
     int len = Serial.readBytesUntil('<\n>', serialBuffer, sizeof(serialBuffer));
     String message = String(serialBuffer).substring(0,len-1);
     Serial1.println(message);
  }
  if (Serial1.available() > 0)
  {
    int len = Serial1.readBytesUntil('<\n>', serialBuffer, sizeof(serialBuffer));
    String message = String(serialBuffer).substring(0,len-1);
    int x = 0;
    // AT 0.20 ip syntax
    if (message.indexOf("+CIFSR:") > -1)
    {
      if (message.indexOf("+CIFSR:STAIP") > -1)
      {
        int start = message.indexOf("+CIFSR:STAIP");
        start = message.indexOf("\"", start) + 1; 
        ip = message.substring(start, message.indexOf("\"", start));
        Serial.println("IP: " + ip);
        broadcast();
      }
    }
    // AT pre 0.20 ip syntax
    else if (message.indexOf("AT+CIFSR") > -1)
    {
      int start = message.indexOf("AT+CIFSR");
      start = message.indexOf("\n", start) + 1;
      start = message.indexOf("\n", start) + 1;
      ip = message.substring(start, message.indexOf("\n", start) - 1);
      Serial.println("IP: " + ip);
      broadcast();
    }
    // Receiving boradcast messages not working with 0020000903, but with 0018000902-AI03
    // Not sure why. But static IP is supported.
    else if (message.indexOf("M-SEARCH") > -1)
    {
      Serial1.println("AT+CIFSR");
    }
    else if (message.indexOf("setcolor:") > -1)
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
        // Change LED color
        Serial.print("Red:");
        Serial.print(red);
        Serial.print(" Green:");
        Serial.print(green);
        Serial.print(" Blue:");
        Serial.println(blue);
      }
    }
    Serial.println(message);
  }
  if ((ip == "" || ip == "0.0.0.0") && setupDone && (millis() - previousCheckIPMillis > checkIPInterval))
  {
    Serial1.println("AT+CIFSR");
    previousCheckIPMillis = millis();
  }
}

void broadcast()
{
  if ((ip == "" || ip == "0.0.0.0") && setupDone)
  {
    return;
  }
  String broadcastConnection = "AT+CIPSTART=3,\"UDP\",\"";
  broadcastConnection += broadcastIP;
  broadcastConnection += "\",";
  broadcastConnection += broadcastPort;
  Serial1.println(broadcastConnection);
  delay(100);
  String broadcastReply = "AtmoOrb:";
  broadcastReply += ip;
  broadcastReply += ",";
  broadcastReply += serverPort;
  broadcastReply += ";";
  Serial1.print("AT+CIPSEND=3,");
  Serial1.println(broadcastReply.length());
  delay(5);
  Serial1.println(broadcastReply);
}

