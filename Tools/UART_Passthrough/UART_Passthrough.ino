// Sketch to send serial data through the Arduino to a second device.
// Usefull to flash ESP8266 through an Arduino.

void setup()
{
  Serial.begin(115200);
  Serial1.begin(115200);
}

void loop()
{
  if (Serial.available() > 0)
  {
    Serial1.write(Serial.read());
  }
  if (Serial1.available() > 0)
  {
    Serial.write(Serial1.read());
  }
}

