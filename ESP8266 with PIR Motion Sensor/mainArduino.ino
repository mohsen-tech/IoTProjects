#include <Arduino.h>
#include <ESP8266HTTPClient.h>
#include "ArduinoJson.h"
#include <ESP8266WiFi.h>

#ifndef STASSID
#define SSID "mohsen_tech"
#define STAPSK "1223334444"
#endif

const char *ssid = SSID;
const char *psk = STAPSK;
int val;

#define sensorPin D5
#define buzzerPin D2

WiFiClient client;
HTTPClient http;
String host = "http://178.63.245.230:8090/status";

bool postToServer()
{
  bool has_auth = false;
  DynamicJsonDocument doc(200);
  doc["status"] = true;
  doc["chipID"] = ESP.getChipId();
  String json;
  serializeJsonPretty(doc, json);
  Serial.println(json); //
  if (http.begin(client, host))
  {
    http.addHeader("Content-Type", "application/json"); //
    int httpCode = http.POST(json);
    Serial.println(httpCode); //
    if (httpCode > 0)
    {
      Serial.printf("[HTTP] POST... code: %d\n", httpCode);
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
      {
        has_auth = true;
        DynamicJsonDocument payload(800);
        deserializeJson(payload, http.getStream());
        String res = payload.as<String>();
        Serial.print("result: ");
        Serial.println(res);
      }
    }
    else
      Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
    http.end();
  }
  else
    Serial.printf("[HTTP] Unable to connect\n");
  return has_auth;
}
// void ICACHE_RAM_ATTR interruptService()
// {
//   while (postToServer())
//   {
//   }
// }
void setup()
{
  pinMode(sensorPin, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(9600);
  Serial.println();
  Serial.print("Connecting to: ");
  Serial.println(ssid);

  WiFi.begin(ssid, psk);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  //TODO change the interruot to waking the wemos up
  //bring the wemos out from deep sleep
  //attachInterrupt(digitalPinToInterrupt(sensorPin), interruptService, CHANGE);
}
void loop()
{
  val = digitalRead(sensorPin);
  if (val == HIGH)
  {
    postToServer();
    tone(buzzerPin, 600);
    digitalWrite(LED_BUILTIN, LOW);
    delay(1000);
    noTone(buzzerPin);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    tone(buzzerPin, 600);
    digitalWrite(LED_BUILTIN, LOW);
    delay(1000);
    noTone(buzzerPin);
    digitalWrite(LED_BUILTIN, HIGH);
  }
}