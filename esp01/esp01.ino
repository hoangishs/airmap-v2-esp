#include "./Communication.h"
#include "./Device.h"

uint8_t dataBuffer[PACKET_SIZE] = {0};
uint8_t dataByteCount = 0;

bool isGetTime = true;
bool isPublish = false;

uint32_t lastGetTime = 0;
uint32_t lastMqttReconnect = 0;

char topic[25];
char espID[10];

WiFiClient espClient;
PubSubClient mqttClient(espClient);

NTPtime NTPch("ch.pool.ntp.org");///???
strDateTime dateTime;

void setup()
{
  DEBUG.begin(9600);
  DEBUG.setDebugOutput(true);
  Serial.begin(9600);
  pinMode(PIN_BUTTON, INPUT);
  WiFi.mode(WIFI_STA);
  initMqttClient(topic, espID, mqttClient);
  DEBUG.println(" - setup done");
}

void loop()
{
  if (longPressButton())
  {
    DEBUG.println(" - long press!");
    if (WiFi.beginSmartConfig())
    {
      DEBUG.println(" - start config wifi");
    }
  }
  if (Serial.available() > 0)
  {
    dataBuffer[dataByteCount] = Serial.read();
    //Serial.print(dataByteCount);
    //Serial.print(" ");
    //Serial.println(dataBuffer[dataByteCount]);
    if (dataBuffer[0] == 8)
      dataByteCount++;
    if (dataByteCount == 2)
    {
      if (dataBuffer[1] == 8)
      {
        dataByteCount = 0;
        uint8_t espMacAddress[6];
        WiFi.macAddress(espMacAddress);
        Serial.write(8);
        Serial.write(8);
        Serial.write(0);
        Serial.write(espMacAddress[3]);
        Serial.write(espMacAddress[4]);
        Serial.write(espMacAddress[5]);
        //Serial.println("id");
      }
    }
    if (dataByteCount == PACKET_SIZE)
    {
      dataByteCount = 0;
      if (dataBuffer[1] == 11)
      {
        uint16_t check = 0;
        for (uint8_t i = 0; i < PACKET_SIZE - 2; i++)
        {
          check += dataBuffer[i];
        }
        uint16_t check2 = dataBuffer[PACKET_SIZE - 2] * 256 + dataBuffer[PACKET_SIZE - 1];
        if (check == check2)
        {
          isPublish = true;
        }
      }
    }
  }
  else if (WiFi.status() == WL_CONNECTED)
  {
    if (isGetTime)
    {
      dateTime = NTPch.getNTPtime(7.0, 0);
      if (dateTime.valid)
      {
        //if got time
        lastGetTime = millis();
        isGetTime = false;

        uint8_t time2arduino_buff[11] = {8, 11, dateTime.hour, dateTime.minute, dateTime.second, dateTime.year - 2000, dateTime.month, dateTime.day, dateTime.dayofWeek, 0, 0};
        uint16_t a = 0;
        for (uint8_t i = 0; i < 9; i++)
        {
          a += time2arduino_buff[i];
        }
        time2arduino_buff[9] = a / 256;
        time2arduino_buff[10] = a % 256;
        for (uint8_t i = 0; i < 11; i++)
        {
          Serial.write(time2arduino_buff[i]);
          //Serial.println("time");
        }

      }
    }
    else if (isPublish)
    {
      //if queue is not empty, publish data to server
      if (mqttClient.connected())
      {
        DEBUG.print(" - publish:.. ");
        uint32_t epochTime = 0;

        struct tm t;
        time_t epoch;

        t.tm_hour = dataBuffer[16];
        t.tm_min = dataBuffer[17];
        t.tm_sec = dataBuffer[0];
        t.tm_mday = dataBuffer[18];          // Day of the month
        t.tm_mon = dataBuffer[19] - 1;         // Month, 0 - jan
        t.tm_year = dataBuffer[20] + 100;
        t.tm_isdst = -1;        // Is DST on? 1 = yes, 0 = no, -1 = unknown

        epoch = mktime(&t);

        epochTime = epoch;

        char mes[256] = {0};
        sprintf(mes, "{\"data\":{\"tem\":\"%d\",\"humi\":\"%d\",\"pm1\":\"%d\",\"pm2p5\":\"%d\",\"pm10\":\"%d\",\"time\":\"%d\"}}", dataBuffer[2], dataBuffer[3], ((dataBuffer[4] << 8) + dataBuffer[5]), ((dataBuffer[6] << 8) + dataBuffer[7]), ((dataBuffer[8] << 8) + dataBuffer[9]), epochTime);
        if (mqttClient.publish(topic, mes, true))
        {
          DEBUG.println(mes);
          isPublish = false;
        }
        mqttClient.loop();
      }
      else if (millis() - lastMqttReconnect > 1000)
      {
        lastMqttReconnect = millis();
        DEBUG.println(" - mqtt reconnect ");
        mqttClient.connect(espID);
      }
      else if (millis() - lastGetTime > 300000)
      {
        isGetTime = true;
      }
    }
  }
}
