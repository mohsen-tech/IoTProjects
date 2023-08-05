#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Ticker.h>
#include <SoftwareSerial.h>
#include "../lib/utils/constants.h"

//--------------------------------------------- Timer for sound buzzer --------------------------------------//
Ticker buzzerAndLedTimer, pirTimer, pirCheckTimer;
#define buzzerAndLedTimer_S 1
#define numberOfBuzzerAndLedTimerRuns 20
byte cntBuzzerAndLedTimer = 0;
#define pirTimer_MS 50
#define numberOfPirTimerRuns 4000 / pirTimer_MS
byte cntPirTimer = 0, sumPirValue = 0;
bool pirVal = LOW;
#define pirCheckTimer_S 600 // 180 // 600
void pirCheckTimerHandler();
void pirTimerHandler();
void buzzerAndLedTimerHandler();
void resetPirSetting()
{
    cntPirTimer = 0;
    sumPirValue = 0;
    pirCheckTimer.detach();
}
//---------------------------------------------------------------------------------------------------------------//

//--------------------------------------------- config + read + write in EEPROM --------------------------------------//
#define magicIdx 0
#define magicNum 13
#define stateIdx 1 // pair or not
#define isPair 1
#define notPair 0
#define phoneNumberSizeIdx 2
#define ssidSizeIdx 3
#define passSizeIdx 4
void writeToEEPROM(const String &_src, const byte &_idx, const byte &_start, const byte &_size);
String readFromEEPROM(const byte &_start, const byte &_size);
void initEEPROM();
void reseteFactory();
//---------------------------------------------------------------------------------------------------------------//

//--------------------------------------------- config wifi --------------------------------------//
// Network
#define SSID "ESP8266"
#define PASS "12345678"
#define PARAM_INPUT1 "phoneNumber"
#define PARAM_INPUT2 "ssid"
#define PARAM_INPUT3 "pass"
#define PARAM_INPUT4 "code"
#define PARAM_INPUT5 "pir"
ESP8266WebServer server(80);
String mySSID, myPass, myPhoneNumber, validationCode;
String generateCode(String &_str, const byte &_codeSize = 6)
{
    _str = "";
    for (byte i = 0; i < _codeSize; i++)
        _str += random(9);
    ///
    Serial.print("code: ");
    Serial.println(_str);
    return _str;
}
void handleRoot();
void handleGet();
void handleValid();
void handleNotFound();
void serverConfig();

// GSM
SoftwareSerial sim800(D2, D1);
void initSim800();
void updateSerial();
String receiveFromSim();
void sendSMS(const String &number, const String &text);
bool checkSenderValid(const String &command);
String mySMS = "";
#define reset_command "reset"
#define status_command "status"
#define active_command "active"
#define deactive_command "deactive"
void runCommand(const String &command, const bool &valid);

#define BUZZER_PIN D3
#define LED_PIN D4
#define PIR_PIN D5
bool pairState = notPair, pirCheck = false;

void setup()
{
    Serial.begin(9600);
    Serial.println();
    initSim800();

    // sendSMS("+989308737667", "test");

    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(LED_PIN, OUTPUT);
    pinMode(PIR_PIN, INPUT);
    digitalWrite(LED_PIN, LOW);
    WiFi.mode(WIFI_AP_STA);
    initEEPROM();
    if (pairState == notPair)
    {
        if (WiFi.softAP(SSID, PASS) == true)
        {
            Serial.println(WiFi.softAPIP());
        }
    }
    else
    {
        // reseteFactory();
        WiFi.begin(mySSID, myPass);
        if (WiFi.waitForConnectResult(10000) == WL_CONNECTED)
        {
            Serial.println(WiFi.localIP());
            MDNS.begin("ESP8266");
            // TODO: Add Server info
        }
        else
        {
            Serial.println("bad shod k!");
        }
    }
    serverConfig();
}
void loop()
{
    mySMS = receiveFromSim();
    if (pairState == isPair)
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
    MDNS.update();
    server.handleClient();
}

// ###############################################################################################################################//
void pirCheckTimerHandler()
{
    pirCheckTimer.detach();
    pirTimer.attach_ms(pirTimer_MS, pirTimerHandler);
}
void pirTimerHandler()
{
    sumPirValue += digitalRead(PIR_PIN);
    cntPirTimer++;
    if (cntPirTimer >= numberOfPirTimerRuns)
    {
        if (sumPirValue > numberOfPirTimerRuns - 5)
            pirVal = HIGH;
        else
            pirVal = LOW;

        if (pirVal == HIGH)
        {
            ///
            sendSMS(myPhoneNumber, "Movement Detected!");
            pirTimer.detach();
            pirCheckTimer.attach(pirCheckTimer_S, pirCheckTimerHandler);
            buzzerAndLedTimer.attach(buzzerAndLedTimer_S, buzzerAndLedTimerHandler);
        }
        ///
        Serial.println(pirVal);
        cntPirTimer = 0;
        sumPirValue = 0;
    }
}
void buzzerAndLedTimerHandler()
{
    cntBuzzerAndLedTimer++;
    if (digitalRead(LED_PIN) == LOW)
    {
        tone(BUZZER_PIN, 600);
    }
    else
    {
        noTone(BUZZER_PIN);
    }
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    if (cntBuzzerAndLedTimer >= numberOfBuzzerAndLedTimerRuns)
    {
        noTone(BUZZER_PIN);
        digitalWrite(LED_PIN, LOW);
        cntBuzzerAndLedTimer = 0;
        buzzerAndLedTimer.detach();
    }
}
// ###############################################################################################################################//
void writeToEEPROM(const String &_src, const byte &_idx, const byte &_start, const byte &_size)
{
    byte j = 0;
    EEPROM.write(_idx, _size);
    for (byte i = _start; i < _start + _size; i++)
    {
        EEPROM.write(i, _src[j++]);
        delay(1);
    }
    EEPROM.commit();
    delay(1);
}
String readFromEEPROM(const byte &_start, const byte &_size)
{
    String temp = "";
    for (byte i = _start; i < _start + _size; i++)
    {
        temp += (char)EEPROM.read(i);
        delay(1);
    }
    return temp;
}
void initEEPROM()
{
    EEPROM.begin(100);
    delay(1);
    if (EEPROM.read(magicIdx) != magicNum)
    {
        delay(1);
        EEPROM.write(magicIdx, magicNum);
        delay(1);
        EEPROM.write(stateIdx, notPair);
        delay(1);
        EEPROM.commit();
        delay(1);
        return;
    }
    pairState = EEPROM.read(stateIdx);
    delay(1);
    if (pairState == isPair)
    {
        byte start = passSizeIdx + 1, size = EEPROM.read(phoneNumberSizeIdx);
        delay(1);
        myPhoneNumber = readFromEEPROM(start, size);
        Serial.println(myPhoneNumber);

        start = passSizeIdx + size + 1;
        size = EEPROM.read(ssidSizeIdx);
        mySSID = readFromEEPROM(start, size);
        Serial.println(mySSID);

        start += size;
        size = EEPROM.read(passSizeIdx);
        myPass = readFromEEPROM(start, size);
        Serial.println(myPass);
    }
}
void reseteFactory()
{
    EEPROM.write(stateIdx, notPair);
    delay(1);
    EEPROM.commit();
    delay(1);
    ESP.restart();
}
// ###############################################################################################################################//

void handleRoot()
{
    if (pairState == isPair)
    {
        if (server.hasArg(PARAM_INPUT5))
        {
            String pir = server.arg(PARAM_INPUT5);
            if (pir == "false")
            {
                Serial.println("pir == false");
                pirCheck = false;
                resetPirSetting();
                pirTimer.detach();
            }
            else if (pir == "true")
            {
                Serial.println("pir == true");
                pirCheck = true;
                resetPirSetting();
                pirTimer.attach_ms(pirTimer_MS, pirTimerHandler);
            }
        }
        String html = home_html;
        if (pirCheck == true)
            html += "<body><h1>Status: <span id=\"status\">Active</span></h1>";
        else
            html += "<body><h1>Status: <span id=\"status\">Inactive</span></h1>";
        html += "<button onclick=\"togglePirValue()\">Toggle PIR Value</button></body></html>";
        server.send(200, "text/html", html);
    }
    else
        server.send(200, "text/html", htmlmsg);
}
void handleGet()
{
    String phoneNumber = server.arg(PARAM_INPUT1);
    String ssid = server.arg(PARAM_INPUT2);
    String pass = server.arg(PARAM_INPUT3);

    if (phoneNumber != "" && ssid != "" && pass != "")
    {
        WiFi.begin(ssid, pass);
        if (WiFi.waitForConnectResult(10000) == WL_CONNECTED)
        {
            WiFi.disconnect();
            myPhoneNumber = phoneNumber;
            mySSID = ssid;
            myPass = pass;

            sendSMS(myPhoneNumber, generateCode(validationCode));
            ///
            server.send(200, "text/html", valid_html);
            return;
        }
    }
    server.send(200, "text/html", "The entered values are not correct!<br><a href=\"/\">Return to Home Page</a>");
    // server.send(200, "text/html", "HTTP GET request sent to your ESP on input field (" + inputParam + ") with value: " + inputMessage + "<br><a href=\"/\">Return to Home Page</a>");
}
void handleValid()
{
    if (server.hasArg(PARAM_INPUT4))
    {
        String inputMessage = server.arg(PARAM_INPUT4);
        if (inputMessage != "" && inputMessage == validationCode)
        {
            byte end0 = passSizeIdx + 1;
            byte end1 = end0 + myPhoneNumber.length();
            byte end2 = end1 + mySSID.length();

            Serial.print("end0: ");
            Serial.print(end0);
            Serial.print(" size0: ");
            Serial.println(myPhoneNumber.length());

            Serial.print("end1: ");
            Serial.print(end1);
            Serial.print(" size1: ");
            Serial.println(mySSID.length());

            Serial.print("end2: ");
            Serial.print(end2);
            Serial.print(" size2: ");
            Serial.println(myPass.length());

            writeToEEPROM(myPhoneNumber, phoneNumberSizeIdx, end0, myPhoneNumber.length());
            writeToEEPROM(mySSID, ssidSizeIdx, end1, mySSID.length());
            writeToEEPROM(myPass, passSizeIdx, end2, myPass.length());

            EEPROM.write(stateIdx, isPair);
            delay(1);
            EEPROM.commit();
            delay(1);

            Serial.println(myPhoneNumber);
            server.send(200, "text/html", "Validation was successful");
            delay(1000);
            ESP.restart();
        }
    }
    server.send(200, "text/html", "Error in validation<br><a href=\"/\">Return to Home Page</a>");
}
void handleNotFound()
{
    server.send(404, "text/plain", "NOT FOUND - ERROR 404");
}
void serverConfig()
{
    server.on("/", HTTP_GET, handleRoot);
    server.on("/get", HTTP_GET, handleGet);
    server.on("/valid", HTTP_GET, handleValid);
    server.onNotFound(handleNotFound);
    server.begin();
}
// ###############################################################################################################################//
void initSim800()
{
    sim800.begin(115200);
    delay(1000);
    sim800.setTimeout(10000);
    delay(1000);
    sim800.println("AT"); // Once the handshake test is successful, it will back to OK
    updateSerial();
    sim800.println("AT+CMGF=1"); // Configuring TEXT mode
    updateSerial();
    sim800.println("AT+CNMI=2,2,0,0,0"); // Forward received SMS to serial port using interrupt
    updateSerial();
    sim800.println("AT+CSMP=17,167,2,25"); // for Irancell
    updateSerial();
    sim800.print("AT+CMGDA=\"DEL ALL\"\r\n"); // delete sms
    updateSerial();
}
void updateSerial()
{
    delay(500);
    while (Serial.available())
    {
        sim800.write(Serial.read()); // Forward what Serial received to Software Serial Port
    }
    while (sim800.available())
    {
        Serial.write(sim800.read()); // Forward what Software Serial received to Serial Port
    }
}
String receiveFromSim()
{
    String temp = "";
    if (sim800.available())
        temp += sim800.readString();
    ///
    if (temp != "")
        Serial.println(temp);
    return temp;
}
void sendSMS(const String &number, const String &text)
{
    sim800.print(F("AT+CMGS=\"")); // command to send sms
    sim800.print(number);
    sim800.print(F("\"\r"));
    updateSerial();
    sim800.print(text); // text content
    updateSerial();
    sim800.write(26);
}
bool checkSenderValid(const String &command)
{
    Serial.println(command.indexOf("+98"));
    ///
    int index = command.indexOf(myPhoneNumber);
    if (index > 0 && index < 20)
        return true;
    return false;
}
void runCommand(const String &command, const bool &valid)
{
    if (command.indexOf(reset_command) > 0 && valid == true)
    {
        sendSMS(myPhoneNumber, "Factory reset command");
        delay(1000);
        reseteFactory();
    }
    else if (command.indexOf(status_command) > 0 && valid == true)
    {
        String str = "";
        if (pirCheck == true)
            str += "The PIR sensor is active";
        else
            str += "The PIR sensor is deactive";
        sendSMS(myPhoneNumber, str);
    }
    else if (command.indexOf(active_command) > 0 && valid == true)
    {
        pirCheck = true;
        resetPirSetting();
        pirTimer.attach_ms(pirTimer_MS, pirTimerHandler);
        sendSMS(myPhoneNumber, "PIR activation command");
    }
    else if (command.indexOf(deactive_command) > 0 && valid == true)
    {
        pirCheck = false;
        resetPirSetting();
        pirTimer.detach();
        sendSMS(myPhoneNumber, "PIR deactivation command");
    }
    else if (valid == true)
    {
        sendSMS(myPhoneNumber, "Your command is wrong");
    }
    else if (command.indexOf("+98") > 0)
    {
        int index = command.indexOf("+98");
        String tempPhoneNumber = command.substring(index, index + 13);
        sendSMS(tempPhoneNumber, "Access is impossible");
    }
}