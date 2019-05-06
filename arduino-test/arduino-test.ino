#include <SoftwareSerial.h>

#define debug true

SoftwareSerial espSerial(3, 2);

uint32_t last = 0;
uint32_t lastT = 0;
bool checkSum();

void setup()
{
  Serial.begin(9600);
  espSerial.begin(9600);
  if (debug) Serial.println("setup done");
}
void loop()
{
  if (millis() - last > 10000)
  {
    last = millis();
    uint8_t data2esp_buff[23] = {8, 11, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 0, 0};
    uint16_t a = 0;
    for (uint8_t i = 0; i < 21; i++)
    {
      a += data2esp_buff[i];
    }
    data2esp_buff[21] = a / 256;
    data2esp_buff[22] = a % 256;
    if (debug) Serial.print(" - data: ");
    for (uint8_t i = 0; i < 23; i++)
    {
      espSerial.write(data2esp_buff[i]);
      if (debug) Serial.print(data2esp_buff[i]);
      if (debug) Serial.print(" ");
    }
    if (debug) Serial.println();
  }
  else if (millis() - lastT > 3000)
  {
    lastT = millis();
    espSerial.write(8);
    if (debug) Serial.print(8);
    if (debug) Serial.print(" ");
    espSerial.write(8);
    if (debug) Serial.println(8);
  }
}
