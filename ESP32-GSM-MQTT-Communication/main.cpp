// [env:nodemcu-32s]
// platform = espressif32
// board = nodemcu-32s
// framework = arduino
// lib_deps = 
// 	knolleary/PubSubClient@^2.8
// 	vshymanskyy/TinyGSM@^0.11.7
// 	adafruit/Adafruit Unified Sensor@^1.1.13
// 	adafruit/DHT sensor library@^1.4.4

/*----------------------------------------------------------\
|                                                           |
|   remote-asiatech.runflare.com:30705                      |
|   iot-project                                             |
|   wBbnOox6t6WkqM4vNFKc                                    |
|                                                           |
\----------------------------------------------------------*/
#define TINY_GSM_MODEM_SIM800 // Modem is SIM800L
#define GSM_PIN ""
const char apn[] = "Irancell-GPRS";
const char gprsUser[] = "";
const char gprsPass[] = "";
const char simPIN[] = "";

#include <TinyGsmClient.h>
#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm modem(debugger);
#else
TinyGsm modem(Serial2);
#endif

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <PubSubClient.h>
#include <EEPROM.h>
#include <Ticker.h>

// Configuration
#define BAUD_RATE 9600
#define STA_SIZE 15
#define APP_VERSION "0.0.1"
String uniqueID = "", device2UniqueID = "";
byte currentState = 0x00; // 0x00=>NotPair || 0x01=>Pair
byte withModules = 0x00;  // 0x00=>without Modules || 0x01=>with Modules
byte numberModules = 0;
bool arrOfStatus[STA_SIZE] = {0};

// GSM and GPRS side
#define MODEM_TX 17
#define MODEM_RX 16
TinyGsmClient client(modem);
void initGSM();
void initGPRS();

// Network side
#define defaultSSID "ESP-WiFi"
#define defaultPASSWORD "12345678"
#define ADMIN_USER "admin"
#define ADMIN_PASSWORD "admin"
String userName, userPassword, userWiFiSSID, userWiFiPassword;
WebServer server(80);
const char login_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Login Form</title>
</head>
<body>
    <h1>Login</h1>
    <form id="loginForm" action="/authenticate" method="POST">
        <label for="username">UserName:</label>
        <input type="text" id="username" name="username" required><br><br>
        <label for="password">Password:</label>
        <input type="password" id="password" name="password" required><br><br>
        <input type="submit" value="Login">
    </form>
    <script>
        document.getElementById('loginForm').addEventListener('submit', function (event) {
            event.preventDefault();
            var username = document.getElementById('username').value;
            var password = document.getElementById('password').value;
            var formData = new FormData();
            formData.append('username', username);
            formData.append('password', password);
            var xhr = new XMLHttpRequest();
            xhr.open('POST', '/authenticate', true);
            xhr.onreadystatechange = function () {
                if (xhr.readyState === XMLHttpRequest.DONE) {
                    if (xhr.status === 200) {
                        window.location.href = '/adminForm';
                    }
                    else {
                        window.location.href = '/error';
                    }
                }
            };
            xhr.send(formData);
        });
    </script>
</body>
</html>
)rawliteral";
const char adminForm_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Admin Form</title>
</head>
<body>
    <h1>Enter Server, WiFi, Device details</h1>
    <form id="authForm" action="/checkAdminForm" method="POST">
        <label for="username">Username (for Device):</label>
        <input type="text" id="username" name="username" required><br><br>
        <label for="userPassword">Password (for Device):</label>
        <input type="password" id="userPassword" name="userPassword" required><br><br>
        <label for="ssid">WiFi SSID:</label>
        <input type="text" id="ssid" name="ssid" required><br><br>
        <label for="ssidPassword">WiFi Password:</label>
        <input type="password" id="ssidPassword" name="ssidPassword" required><br><br>
        <label for="serverAddress">Server Address:</label>
        <input type="text" id="serverAddress" name="serverAddress" required><br><br>
        <label for="serverPort">Server Port:</label>
        <input type="number" id="serverPort" name="serverPort" required><br><br>
        <label for="serverUserName">Server Username:</label>
        <input type="text" id="serverUserName" name="serverUserName" required><br><br>
        <label for="serverPassword">Server Password:</label>
        <input type="password" id="serverPassword" name="serverPassword" required><br><br>
        <label for="NumberModules">Number of modules connected to the device (min 1, max 15):</label>
        <input type="number" id="NumberModules" name="NumberModules" min='1' max='15' required><br><br>
        <label for="WithModules">With connected modules (if it is not connected to a module enter 0 or 1):</label>
        <input type="number" id="WithModules" name="WithModules" min='0' max='1' required><br><br>
        <label for="Device1UniqueID">UniqueID of this device:</label>
        <input type="text" id="Device1UniqueID" name="Device1UniqueID" required><br><br>
        <label for="Device2UniqueID">UniqueID of another device:</label>
        <input type="text" id="Device2UniqueID" name="Device2UniqueID" required><br><br>
        <input type="submit" value="Submit">
    </form>
    <script>
        document.getElementById('authForm').addEventListener('submit', function (event) {
            event.preventDefault();
            var username = document.getElementById('username').value;
            var userPassword = document.getElementById('userPassword').value;
            var ssid = document.getElementById('ssid').value;
            var ssidPassword = document.getElementById('ssidPassword').value;
            var serverAddress = document.getElementById('serverAddress').value;
            var serverPort = document.getElementById('serverPort').value;
            var serverUserName = document.getElementById('serverUserName').value;
            var serverPassword = document.getElementById('serverPassword').value;
            var NumberModules = document.getElementById('NumberModules').value;
            var WithModules = document.getElementById('WithModules').value;
            var Device1UniqueID = document.getElementById('Device1UniqueID').value;
            var Device2UniqueID = document.getElementById('Device2UniqueID').value;
            window.location.href = '/checkAdminForm?username=' + encodeURIComponent(username) +
                '&userPassword=' + encodeURIComponent(userPassword) +
                '&ssid=' + encodeURIComponent(ssid) +
                '&ssidPassword=' + encodeURIComponent(ssidPassword) +
                '&serverAddress=' + encodeURIComponent(serverAddress) +
                '&serverPort=' + encodeURIComponent(serverPort) +
                '&serverUserName=' + encodeURIComponent(serverUserName) +
                '&serverPassword=' + encodeURIComponent(serverPassword) +
                '&NumberModules=' + encodeURIComponent(NumberModules) +
                '&WithModules=' + encodeURIComponent(WithModules) +
                '&Device1UniqueID=' + encodeURIComponent(Device1UniqueID) +
                '&Device2UniqueID=' + encodeURIComponent(Device2UniqueID);
        });
    </script>
</body>
</html>
)rawliteral";
const char status_html1[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
        body {
            font-family: Arial, sans-serif;
        }
        table {
            border-collapse: collapse;
            width: 50%;
            margin: 20px auto;
        }
        th,
        td {
            border: 1px solid #ddd;
            padding: 8px;
            text-align: center;
        }
        th {
            background-color: #f2f2f2;
        }
        #submitButton {
            display: block;
            margin: 20px auto;
            padding: 10px;
            background-color: #4CAF50;
            color: white;
            border: none;
            border-radius: 5px;
            cursor: pointer;
        }
    </style>
    <title>Status</title>
</head>
<body>
    <form action="/newOrder" method="get">
        <table>
            <tr>
                <th>Number of Modules</th>
                <th>Status</th>
                <th>Enter new Status (0 or 1)</th>
            </tr>
)rawliteral";
const char status_html2[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
        body {
            font-family: Arial, sans-serif;
        }
        table {
            border-collapse: collapse;
            width: 50%;
            margin: 20px auto;
        }
        th,
        td {
            border: 1px solid #ddd;
            padding: 8px;
            text-align: center;
        }
        th {
            background-color: #f2f2f2;
        }
        #formContainer {
            text-align: center;
            margin-top: 20px;
        }
        #submitButton {
            display: inline-block;
            margin-top: 10px;
            padding: 10px;
            background-color: #4CAF50;
            color: white;
            border: none;
            border-radius: 5px;
            cursor: pointer;
        }
    </style>
    <title>Status</title>
</head>
<body>
    <table>
        <tr>
            <th>Number of Modules</th>
            <th>Status</th>
        </tr>
)rawliteral";
void initAP();
void initWebServer();
void handleRoot();
void handleAuthenticate();
void handleAdminForm();
void handleCheckAdminForm();
void handleReset();
String createStatusWebPageV1();
String createStatusWebPageV2();
void handleStatus();
void handleNewOrder();
void handleError();
void handleNotFound();

// MQTT Broker side
#define MQTT_TIMEOUT 5000
uint32_t lastReconnectAttempt;
String mqttServer;
int mqttPort;
String MQTT_Username, MQTT_Password;
const char *registerTopic = "Register";
bool flagConnected = false;
PubSubClient mqttClient(client);
void initMQTT();
bool mqttConnect(const String &_clientId);
void mqttPublish(const String &_topic, const String &_command);
void subscribeReceive(char *topic, byte *payload, unsigned int length);

// Implementation of work with EEPROM
#define SIZE_EEPROM 400
#define MAGIC_IDX 0
#define MAGIC_NUM 13
#define CSidx_EEPROM 1 // index of CurrentState in EEPROM
#define WithModules_IDX 2
#define NumberModules_IDX 3 // Default Value in EEPROM = 0
#define PORT_IDX 4
#define DATA_IDX PORT_IDX + sizeof(int) + 1 // Server Address + UserName + Password
#define IsConfig 0x01
#define IsNotConfig 0x00
#define separator '#'
String getStrWithSeparatorFromEEPROM(byte &_idx);
void initEEPROM();
void resetFactoryFunc(); // Reset and set default
String createStrForEEPROM(const String &, const String &, const String &, const String &, const String &, const String &, const String &, const String &, const String &);
void saveToEEPROM(const byte &_idx, const String &_str);

// other Implementation
Ticker timer;
int TIMER_S = 60;
void timerHandler();
String createCommand(const int &, const bool &);
void fetchCommand(const String &);

//---------------------------------------------------------------------------------------------------------------------------------------
void setup()
{
  Serial.begin(BAUD_RATE);
  Serial.println();
  initEEPROM();
  initAP();
  initWebServer();
  if (currentState == IsConfig)
  {
    initGSM();
    initGPRS();
    initMQTT();
    if (withModules == IsConfig)
      timer.attach(TIMER_S, timerHandler);
  }
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);
  digitalWrite(LED_BUILTIN, LOW);
  delay(1000);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);
  digitalWrite(LED_BUILTIN, LOW);
}
void loop()
{
  server.handleClient();
  if (currentState == IsConfig)
  {
    if (Serial.available() > 0 && withModules == IsConfig)
    {
      String command = Serial.readStringUntil('#');
      fetchCommand(command + '#');
      ///
      mqttPublish(device2UniqueID, createCommand(11, true));
    }
    if (!mqttClient.connected())
    {
      // uint32_t t = millis();
      // if (t - lastReconnectAttempt > MQTT_TIMEOUT)
      // {
      //   lastReconnectAttempt = t;
      //   if (mqttConnect(clientID))
      //   {
      //     lastReconnectAttempt = 0;
      //   }
      // }
      if (lastReconnectAttempt >= MQTT_TIMEOUT)
      {
        mqttConnect(uniqueID);
        lastReconnectAttempt = 0;
      }
      else
      {
        lastReconnectAttempt += 5;
      }
    }
    mqttClient.loop();
  }
  delay(5);
}
//---------------------------------------------------------------------------------------------------------------------------------------
void initGSM()
{
  Serial.println("Wait...");
  // Set GSM module baud rate and UART pins
  Serial2.begin(9600, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(6000);
}
void initGPRS()
{
  // Restart takes quite some time
  // To skip it, call init() instead of restart()
  Serial.println("Initializing modem...");
  modem.restart();
  // modem.init();
  String modemInfo = modem.getModemInfo();
  Serial.print("Modem Info: ");
  Serial.println(modemInfo);
  // Unlock your SIM card with a PIN if needed
  if (GSM_PIN && modem.getSimStatus() != 3)
  {
    modem.simUnlock(GSM_PIN);
  }
  Serial.print("Connecting to APN: ");
  Serial.print(apn);
  if (!modem.gprsConnect(apn, gprsUser, gprsPass))
  {
    Serial.println(" fail");
    ESP.restart();
  }
  else
  {
    Serial.println(" OK");
  }
  if (modem.isGprsConnected())
  {
    Serial.println("GPRS connected");
  }
}
//---------------------------------------------------------------------------------------------------------------------------------------
void initAP()
{
  WiFi.mode(WIFI_AP);
  WiFi.softAP(userWiFiSSID, userWiFiPassword);
  Serial.println(WiFi.softAPIP());
}
void initWebServer()
{
  server.on("/", handleRoot);
  server.on("/authenticate", handleAuthenticate);
  server.on("/adminForm", handleAdminForm);
  server.on("/checkAdminForm", handleCheckAdminForm);
  server.on("/reset", handleReset);
  server.on("/status", handleStatus);
  server.on("/newOrder", handleNewOrder);
  server.on("/error", handleError);
  server.onNotFound(handleNotFound);
  server.begin();
}
void handleRoot()
{
  if (currentState == IsNotConfig)
  {
    server.send(200, "text/html", login_html);
  }
  else if (currentState == IsConfig)
  {
    String str = "<br><a href=\"/status\">Click to See the Status</a>";
    str += "<br><br><a href=\"/reset\">Click to Reset Admin</a>";
    server.send(200, "text/html", str);
  }
}
void handleAuthenticate()
{
  String username_temp = server.arg("username"), pass_temp = server.arg("password");
  if (username_temp == userName && pass_temp == userPassword)
    server.send(200);
  else
    server.send(401);
}
void handleAdminForm()
{
  server.send(200, "text/html", adminForm_html);
}
void handleCheckAdminForm()
{
  String userName_temp = server.arg("username"), userPassword_temp = server.arg("userPassword"),
         wifiSSID_temp = server.arg("ssid"), wifiPassword_temp = server.arg("ssidPassword"),
         serverAddress_temp = server.arg("serverAddress"), serverPort_temp = server.arg("serverPort"),
         serverUserName_temp = server.arg("serverUserName"), serverPassword_temp = server.arg("serverPassword"),
         numberModules_temp = server.arg("NumberModules"), withModules_temp = server.arg("WithModules"),
         device1UniqueID_temp = server.arg("Device1UniqueID"),
         device2UniqueID_temp = server.arg("Device2UniqueID");

  if (userName_temp != "" && userPassword_temp != "" && wifiSSID_temp != "" && wifiPassword_temp != "" &&
      serverAddress_temp != "" && serverPort_temp != "" && serverUserName_temp != "" && serverPassword_temp != "")
  {
    userName = userName_temp;
    userPassword = userPassword_temp;
    userWiFiSSID = wifiSSID_temp;
    userWiFiPassword = wifiPassword_temp;
    mqttServer = serverAddress_temp;
    mqttPort = serverPort_temp.toInt();
    MQTT_Username = serverUserName_temp;
    MQTT_Password = serverPassword_temp;
    uniqueID = device1UniqueID_temp;
    device2UniqueID = device2UniqueID_temp;

    saveToEEPROM(DATA_IDX, createStrForEEPROM(userName, userPassword, userWiFiSSID, userWiFiPassword, mqttServer, MQTT_Username, MQTT_Password, uniqueID, device2UniqueID));
    EEPROM.put(PORT_IDX, mqttPort);
    delay(1);
    currentState = IsConfig;
    EEPROM.write(CSidx_EEPROM, currentState);
    delay(1);
    if (withModules_temp == "0" || withModules_temp == "1")
    {
      EEPROM.write(WithModules_IDX, withModules_temp.toInt());
      delay(1);
    }
    EEPROM.write(NumberModules_IDX, numberModules_temp.toInt());
    delay(1);
    EEPROM.commit();
    delay(1);
    server.send(200, "text/html", "<a>The entered information has been saved!</a><br><br><br><a>After 10 seconds, the device will restart</a>");
    delay(10000);
    ESP.restart();
  }
  else
  {
    server.send(401, "text/html", "The entered values are not correct!<br><a href=\"/\">Return to Home Page</a>");
  }
}
void handleReset()
{
  if (currentState == IsNotConfig)
  {
    server.send(404, "text/plain", "Error 404 - Not Found!");
    return;
  }
  if (!server.authenticate(userName.c_str(), userPassword.c_str()))
  {
    server.requestAuthentication();
    return;
  }
  server.send(200, "text/html", "After 10 seconds, the settings return to the initial state");
  delay(10000);
  resetFactoryFunc();
}
String createStatusWebPageV1()
{
  String html = "";
  html += status_html1;
  for (size_t i = 0; i < numberModules; i++)
  {
    html += "<tr>";
    html += "<td>" + String(i + 1) + "</td>";
    html += "<td>" + String(arrOfStatus[i]) + "</td>";
    html += "<td><input type='number' name='st" + String(i + 1) + "' min='0' max='1' required></td>";
    html += "</tr>";
  }
  html += "</table><input type=\"submit\" id=\"submitButton\" value=\"submit\"></form></body></html>";
  return html;
}
String createStatusWebPageV2()
{
  String html = "";
  html += status_html2;
  for (size_t i = 0; i < numberModules; i++)
  {
    html += "<tr>";
    html += "<td>" + String(i + 1) + "</td>";
    html += "<td>" + String(arrOfStatus[i]) + "</td>";
    html += "</tr>";
  }
  html += "</table><div id=\"formContainer\"><form action=\"/newOrder\" method=\"get\">";
  html += "<label for=\"userInput\">Enter the status capture time: </label>";
  html += "<input type=\"number\" id=\"userInput\" name=\"timer\" min=\"10\" max=\"86400\" required>";
  html += "<button type=\"submit\" id=\"submitButton\">submit</button>";
  html += "</form></div></body></html>";
  return html;
}
void handleStatus()
{
  if (currentState == IsNotConfig)
  {
    server.send(404, "text/plain", "Error 404 - Not Found!");
    return;
  }
  if (!server.authenticate(userName.c_str(), userPassword.c_str()))
  {
    server.requestAuthentication();
    return;
  }
  String html = "";
  if (withModules == IsNotConfig)
  {
    html += createStatusWebPageV1();
  }
  else if (withModules == IsConfig)
  {
    html += createStatusWebPageV2();
  }
  server.send(200, "text/html", html);
}
void handleNewOrder()
{
  if (withModules == IsNotConfig)
  {
    for (size_t i = 0; i < numberModules; i++)
    {
      arrOfStatus[i] = server.arg("st" + String(i + 1)).toInt();
    }
    mqttPublish(device2UniqueID, createCommand(12, true));
  }
  else if (withModules == IsConfig)
  {
    TIMER_S = server.arg("timer").toInt();
    timer.detach();
    timer.attach(TIMER_S, timerHandler);
  }
  delay(500);
  server.sendHeader("Location", "/status", true);
  server.send(302, "text/plane", "");
}
void handleError()
{
  server.send(401, "text/html", "The entered values are not correct!<br><a href=\"/\">Return to Home Page</a>");
}
void handleNotFound()
{
  server.send(404, "text/plain", "Error 404 - Not Found!");
}
//---------------------------------------------------------------------------------------------------------------------------------------
void initMQTT()
{
  mqttClient.setServer(mqttServer.c_str(), mqttPort);
  mqttClient.setCallback(subscribeReceive);
  mqttConnect(uniqueID);
}
bool mqttConnect(const String &_clientId)
{
  if (mqttClient.connect(_clientId.c_str(), MQTT_Username.c_str(), MQTT_Password.c_str()))
  {
    mqttClient.subscribe(uniqueID.c_str());
    String temp = String(uniqueID) + " # " + String(withModules) + " # " + String(APP_VERSION);
    mqttClient.publish((registerTopic + uniqueID).c_str(), temp.c_str());
    return true;
  }
  else
  {
    Serial.println("not connected!\n");
    return false;
  }
}
void mqttPublish(const String &_topic, const String &_command)
{
  Serial.print("publish: ");
  Serial.println(_command);
  if (mqttClient.connected())
  {
    mqttClient.publish(_topic.c_str(), _command.c_str());
  }
  else
  {
    Serial.println("can't publish in timerHandler");
  }
}
void subscribeReceive(char *topic, byte *payload, unsigned int length)
{
  String temp;
  for (int i = 0; i < length; i++)
    temp += (char)payload[i];

  Serial.println("-----------------------------------------------------{");
  Serial.print("topic: ");
  Serial.println(topic);
  Serial.print("payload: ");
  Serial.println(temp);
  Serial.println("-----------------------------------------------------}");

  ///
  fetchCommand(temp);
  if (withModules == IsConfig)
  {
    Serial.println(createCommand(12, true));
  }
}
//---------------------------------------------------------------------------------------------------------------------------------------
String getStrWithSeparatorFromEEPROM(byte &_idx)
{
  String str = "";
  while (1)
  {
    char ch = EEPROM.read(_idx++);
    delay(1);
    if (ch == separator)
      return str;
    str += ch;
  }
}
void initEEPROM()
{
  EEPROM.begin(SIZE_EEPROM);
  delay(1);
  if (EEPROM.read(MAGIC_IDX) != MAGIC_NUM)
  {
    EEPROM.write(MAGIC_IDX, MAGIC_NUM);
    delay(1);
    currentState = IsNotConfig;
    EEPROM.write(CSidx_EEPROM, currentState);
    delay(1);
    EEPROM.write(NumberModules_IDX, currentState);
    delay(1);
    EEPROM.commit();
    delay(1);
  }
  else
  {
    EEPROM.get(CSidx_EEPROM, currentState);
    delay(1);
    if (currentState == IsConfig)
    {
      byte i = DATA_IDX + 1;
      userName = getStrWithSeparatorFromEEPROM(i);
      userPassword = getStrWithSeparatorFromEEPROM(i);
      userWiFiSSID = getStrWithSeparatorFromEEPROM(i);
      userWiFiPassword = getStrWithSeparatorFromEEPROM(i);
      mqttServer = getStrWithSeparatorFromEEPROM(i);
      EEPROM.get(PORT_IDX, mqttPort);
      MQTT_Username = getStrWithSeparatorFromEEPROM(i);
      MQTT_Password = getStrWithSeparatorFromEEPROM(i);
      uniqueID = getStrWithSeparatorFromEEPROM(i);
      device2UniqueID = getStrWithSeparatorFromEEPROM(i);

      EEPROM.get(NumberModules_IDX, numberModules);
      EEPROM.get(WithModules_IDX, withModules);

      Serial.println(userName);
      Serial.println(userPassword);
      Serial.println(userWiFiSSID);
      Serial.println(userWiFiPassword);
      Serial.println(mqttServer);
      Serial.println(mqttPort);
      Serial.println(MQTT_Username);
      Serial.println(MQTT_Password);
      Serial.println(uniqueID);
      Serial.println(device2UniqueID);
      Serial.println(numberModules);
      Serial.println(withModules);
    }
    else
    {
      userName = ADMIN_USER;
      userPassword = ADMIN_PASSWORD;
      userWiFiSSID = defaultSSID;
      userWiFiPassword = defaultPASSWORD;
    }
  }
}
void resetFactoryFunc() // Reset and set default
{
  EEPROM.write(CSidx_EEPROM, IsNotConfig);
  delay(1);
  EEPROM.write(WithModules_IDX, IsNotConfig);
  delay(1);
  EEPROM.write(NumberModules_IDX, IsNotConfig);
  delay(1);
  EEPROM.commit();
  delay(1);
  ESP.restart();
}
String createStrForEEPROM(const String &a, const String &b, const String &c, const String &d, const String &e, const String &f, const String &g, const String &h, const String &i)
{
  return a + separator + b + separator + c + separator + d + separator + e + separator + f + separator + g + separator + h + separator + i + separator;
}
void saveToEEPROM(const byte &_idx, const String &_str)
{
  byte j = _idx + 1;
  for (byte i = 0; i < _str.length(); i++)
  {
    EEPROM.write(j++, _str[i]);
    delay(1);
  }
  EEPROM.commit();
  delay(1);
}
//---------------------------------------------------------------------------------------------------------------------------------------
void timerHandler()
{
  Serial.println(createCommand(11, false));
}
String createCommand(const int &_cmd, const bool &_with)
{
  String str = "*";
  str += String(_cmd);
  if (_with == true)
  {
    for (size_t i = 0; i < numberModules; i++)
    {
      if (i < 9)
        str += String(i + 1) + arrOfStatus[i];
      else if (i == 9)
        str += 'A' + String(arrOfStatus[i]);
      else if (i == 10)
        str += 'B' + String(arrOfStatus[i]);
      else if (i == 11)
        str += 'C' + String(arrOfStatus[i]);
      else if (i == 12)
        str += 'D' + String(arrOfStatus[i]);
      else if (i == 13)
        str += 'E' + String(arrOfStatus[i]);
      else if (i == 14)
        str += 'F' + String(arrOfStatus[i]);
    }
  }
  str += "#";
  return str;
}
void fetchCommand(const String &_cmd)
{
  if (_cmd[0] == '*' && _cmd[_cmd.length() - 1] == '#')
  {
    byte j = 0;
    for (size_t i = 0; i + 4 < _cmd.length() - 1; i += 2)
    {
      arrOfStatus[j++] = _cmd[i + 4] - '0';
    }
  }
}