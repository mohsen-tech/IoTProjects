#include <Arduino.h>
#include <EEPROM.h>
#include <SoftwareSerial.h>
#include <Ticker.h>

#define SIM800_TX_PIN D2 // Connect SIM800 TX to ESP8266 D4 pin
#define SIM800_RX_PIN D1 // Connect SIM800 RX to ESP8266 D5 pin
#define BAUD_RATE 9600
SoftwareSerial gsmSerial(SIM800_RX_PIN, SIM800_TX_PIN); // rx, tx

#define IsPair 1
#define IsNotPair 0
byte isPair = IsNotPair;
bool isSmart = 0; // 1 == smart mode in ON, 0 == smart mode is OFF
String ownersPhoneNumber = "";

#define humidity_PIN A0
#define ElectricalValve_PIN D4 // ACTIVE LOW
#define LOW_threshold 900
#define HIGH_threshold 500
#define LOW_VAL "LOW"
#define MEDIUM_VAL "MEDIUM"
#define HIGH_VAL "HIGH"
String status = "";
#define samplingTimer_S 6
#define wateringWarning_S 300 // 86400s == 24h
#define samplingRate 10
int samplingData[samplingRate] = {0};
int samplingCounter = 0;
Ticker samplingTimer, wateringWarningTimer;
void wateringWarningTimerHandler();
void samplingTimerHandler();

#define EEPROM_SIZE 50
#define MAGIC_NUMBER 26
#define MAGIC_INDEX 0
#define PAIR_INDEX 1
#define PHONENUMBER_INDEX 2 // End phoneNumber with '#'
void initEEPROM()
{
  EEPROM.begin(EEPROM_SIZE);
  delay(1);
  if (EEPROM.read(MAGIC_INDEX) == MAGIC_NUMBER)
  {
    isPair = EEPROM.read(PAIR_INDEX);
    if (isPair == IsPair)
    {
      byte i = PHONENUMBER_INDEX;
      while (1)
      {
        char ch = EEPROM.read(i);
        delay(1);
        if (ch == '#')
          break;
        ownersPhoneNumber += ch;
        i++;
      }
      ///
      Serial.println(ownersPhoneNumber);
      wateringWarningTimer.attach(wateringWarning_S, wateringWarningTimerHandler);
    }
  }
  else
  {
    EEPROM.write(MAGIC_INDEX, MAGIC_NUMBER);
    delay(1);
    EEPROM.write(PAIR_INDEX, IsNotPair);
    delay(1);
    EEPROM.commit();
    delay(1);
  }
}
void saveToEEPROM(const String &_str)
{
  byte j = PHONENUMBER_INDEX;
  for (byte i = 0; i < _str.length(); i++)
  {
    EEPROM.write(j++, _str[i]);
    delay(1);
  }
  EEPROM.write(j, '#');
  delay(1);
  EEPROM.write(PAIR_INDEX, IsPair);
  delay(1);
  EEPROM.commit();
  delay(1);
}
void reseteFactory()
{
  EEPROM.write(PAIR_INDEX, IsNotPair);
  delay(1);
  EEPROM.commit();
  delay(1);
  ESP.restart();
}

String receiveFromSim()
{
  String temp = "";
  if (gsmSerial.available())
  {
    temp += gsmSerial.readString();
  }
  return temp;
}

void updateSerial()
{
  delay(500);
  while (Serial.available())
  {
    gsmSerial.write(Serial.read()); // Forward what Serial received to Software Serial Port
  }
  while (gsmSerial.available())
  {
    Serial.write(gsmSerial.read()); // Forward what Software Serial received to Serial Port
  }
}

void sendSMS(const String &number, const String &text)
{
  // gsmSerial.print("AT+CMGDA=\"DEL ALL\"\r\n"); // delete sms
  // updateSerial();
  gsmSerial.print(F("AT+CMGS=\"")); // command to send sms
  gsmSerial.print(number);
  gsmSerial.print(F("\"\r"));
  updateSerial();
  gsmSerial.print(text); // text content
  updateSerial();
  gsmSerial.write(26);
}
#define register_command "register"
#define delete_command "delete"
#define status_command "status"
#define smartOn_command "smart-on"
#define smartOff_command "smart-off"
#define start_command "start"
#define stop_command "stop"
// if has owner == true
void runCommand(const String &command, const bool &owner)
{
  if (command.indexOf(register_command) > 0 && owner == false)
  {
    int index = command.indexOf("+98");
    String tempPhoneNumber = command.substring(index, index + 13);
    ///
    wateringWarningTimer.attach(2 * samplingRate * samplingTimer_S, wateringWarningTimerHandler); // for first one aftre pair
    ///
    ownersPhoneNumber = tempPhoneNumber;
    isPair = IsPair;
    saveToEEPROM(ownersPhoneNumber);
    sendSMS(ownersPhoneNumber, "You are the owner now!");
  }
  else if (command.indexOf(delete_command) > 0 && owner == true)
  {
    sendSMS(ownersPhoneNumber, "Factory reset command received.");
    delay(1000);
    reseteFactory();
  }
  else if (command.indexOf(status_command) > 0 && owner == true)
  {
    String str = "";
    if (status != "")
      str += "humidity level is " + status + ".\n";
    else
      str += "humidity level is not ready!\n";
    if (isSmart == 1)
      str += "smart mode is ON.\n";
    else
      str += "smart mode is OFF.\n";
    if (digitalRead(ElectricalValve_PIN) == LOW)
      str += "Watering.\n";
    else
      str += "not Watering.\n";
    sendSMS(ownersPhoneNumber, str);
  }
  else if (command.indexOf(smartOn_command) > 0 && owner == true)
  {
    isSmart = 1;
    sendSMS(ownersPhoneNumber, "Your command is run");
  }
  else if (command.indexOf(smartOff_command) > 0 && owner == true)
  {
    isSmart = 0;
    sendSMS(ownersPhoneNumber, "Your command is run");
  }
  else if (command.indexOf(start_command) > 0 && owner == true)
  {
    digitalWrite(ElectricalValve_PIN, LOW);
    sendSMS(ownersPhoneNumber, "Start watering...");
  }
  else if (command.indexOf(stop_command) > 0 && owner == true)
  {
    digitalWrite(ElectricalValve_PIN, HIGH);
    sendSMS(ownersPhoneNumber, "Stop watering...");
  }
  else if (owner == true)
  {
    sendSMS(ownersPhoneNumber, "Your command is wrong");
  }
  else if (command.indexOf("+98") > 0)
  {
    int index = command.indexOf("+98");
    String tempPhoneNumber = command.substring(index, index + 13);
    sendSMS(tempPhoneNumber, "Access is impossible");
  }
}

bool checkSenderValid(const String &command)
{
  if (command.indexOf(ownersPhoneNumber) > 0)
    return true;
  return false;
}

String mySMS = "";
bool checkNewSMS()
{
  gsmSerial.println("AT+CMGL=\"REC UNREAD\"");
  delay(1000);
  while (gsmSerial.available())
  {
    if (gsmSerial.find("+CMGL:"))
    {
      return true;
    }
  }
  return false;
}
void setup()
{
  Serial.begin(BAUD_RATE);
  Serial.println();
  pinMode(ElectricalValve_PIN, OUTPUT);
  ///
  digitalWrite(ElectricalValve_PIN, HIGH);

  gsmSerial.begin(BAUD_RATE);
  delay(1000);
  gsmSerial.setTimeout(10000);
  delay(1000);
  gsmSerial.println("AT"); // Once the handshake test is successful, it will back to OK
  updateSerial();
  gsmSerial.println("AT+CMGF=1"); // Configuring TEXT mode
  updateSerial();
  gsmSerial.println("AT+CNMI=2,2,0,0,0"); // Forward received SMS to serial port using interrupt
  updateSerial();
  gsmSerial.println("AT+CSMP=17,167,2,25"); // for Irancell
  updateSerial();
  gsmSerial.print("AT+CMGDA=\"DEL ALL\"\r\n"); // delete sms
  updateSerial();

  initEEPROM();
  if (isPair == IsPair)
    samplingTimer.attach(samplingTimer_S, samplingTimerHandler);
}
void loop()
{
  mySMS = receiveFromSim();
  if (isPair == IsPair)
  {
    if (mySMS != "" && checkSenderValid(mySMS) == true)
    {
      runCommand(mySMS, true);
    }
    else if (mySMS != "")
    {
      runCommand(mySMS, false);
    }
  }
  else
  {
    if (mySMS != "")
    {
      runCommand(mySMS, false);
    }
  }
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
void wateringWarningTimerHandler()
{
  wateringWarningTimer.detach();
  wateringWarningTimer.attach(wateringWarning_S, wateringWarningTimerHandler);
  if (status == LOW_VAL)
  {
    sendSMS(ownersPhoneNumber, "Watering warning!!!");
    delay(500);
  }
  if (isSmart == 1)
  {
    digitalWrite(ElectricalValve_PIN, LOW);
    sendSMS(ownersPhoneNumber, "Start watering...");
    delay(500);
  }
}
void samplingTimerHandler()
{
  if (samplingCounter >= samplingRate)
  {
    int sum = 0;
    for (byte i = 0; i < samplingRate; i++)
      sum += samplingData[i];
    sum /= samplingRate;
    if (sum > LOW_threshold)
      status = LOW_VAL;
    else if (sum < HIGH_threshold)
      status = HIGH_VAL;
    else
      status = MEDIUM_VAL;
    samplingCounter = 0;

    ///
    if (status == HIGH_VAL)
      digitalWrite(ElectricalValve_PIN, HIGH);
  }
  samplingData[samplingCounter++] = analogRead(humidity_PIN);
}